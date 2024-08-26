#include "server_tcp.h"
#include "thread_pool.h"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>
#include <thread>
#include <iostream>

using namespace boost::asio;

/*

Сервер работает на одном сокете TCP
При создании сервера создается заданное число потоков для обработки клиентов
Нагрузка по потокам распределяется с помощью boost::io_service
В одном соединении может обрабатываться последовательно несколько блоков
После подключения клиент передает хэши
Хэш блоков извлекается из потока TCP по одному
Данные могли бы передаваться в ОС ограниченными по размеру частями, 
    но в задаче отсутствует необходимая функция. Такой функционал мог бы
    пригодиться для чтения файлов с блочного устройства, если размер файлов
    мог бы исчисляться гигабайтами
Клиентская сторона решает сколько соединений необходимо для оптимального получения данных

*/

class ServerTcp::Impl {
public:
    explicit Impl(std::shared_ptr<IBlock> block) 
        : 
        m_block(std::move(block)),
        m_work {boost::asio::make_work_guard(m_service)}, 
        m_acceptor {m_service, ip::tcp::endpoint(ip::tcp::v4(), common::TCP_SERVER_PORT)},
        m_sock {m_service},
        m_context {ssl::context::sslv23}
    {
        m_context.set_options(ssl::context::default_workarounds | ssl::context::no_sslv2 | ssl::context::single_dh_use);
        m_context.set_password_callback(std::bind(&ServerTcp::Impl::get_password, this));
        m_context.use_private_key_file(common::SERVER_KEY, ssl::context::pem);
        m_context.use_certificate_chain_file(common::SERVER_CRT);
        m_context.use_tmp_dh_file(common::DH2048);

        for (unsigned i = 0; i < common::SERVER_THREADS_MAX; ++i) {
            m_tp.create_thread(std::bind(&ServerTcp::Impl::thread_handler, this));
        }
    }
    ~Impl() = default;

    void process() {
        m_acceptor.listen(socket_base::max_connections);
        for (;;) {
            boost::system::error_code ec;
            auto client = m_acceptor.accept(ec);
            if (ec) { client.close(); continue; }
            client_handler(client);
        }
        m_tp.join_all();
    }

protected:
    std::shared_ptr<IBlock> m_block;
    ThreadPool m_tp;
    io_service m_service;
    executor_work_guard<io_context::executor_type> m_work;
    ip::tcp::acceptor m_acceptor;
    ip::tcp::socket m_sock;
    ssl::context m_context;

    void thread_handler() {
        m_service.run();
    }
    std::string get_password() const {
        return "";
    }

    struct Client : public std::enable_shared_from_this<Client> {
        static auto create(ssl::stream<ip::tcp::socket> s, std::shared_ptr<IBlock> block) {
            return std::make_shared<Client>(std::move(s), std::move(block));
        }
        void start_process() { 
            std::cout << "handling client [" << m_port << "] " << std::endl;
            async_handshake();
        }

        Client(const Client&) = delete;
        Client(Client&&) = delete;
        Client& operator=(const Client&) = delete;
        Client& operator=(Client&&) = delete;

        Client(ssl::stream<ip::tcp::socket> s, std::shared_ptr<IBlock> block) 
            : 
            m_sock(std::move(s)),
            m_block(std::move(block))
        {
            m_port = m_sock.lowest_layer().remote_endpoint().port();
            m_hash.reserve(common::HASH_SIZE+1);
        }
        ~Client(){}

    protected:
        ssl::stream<ip::tcp::socket> m_sock;
        std::shared_ptr<IBlock> m_block;
        unsigned short m_port;
        std::string m_hash;
        common::BlockMsgHeader m_msgHeader;
        struct {
            char d[common::CHUNK_SIZE];
            size_t len;
            size_t offs;
        } m_chunk;

        template<typename F>
        constexpr auto bind(F f) {
            return std::bind(f, shared_from_this(), std::placeholders::_1,  std::placeholders::_2);
        }

        void async_handshake() {
            m_sock.async_handshake(ssl::stream_base::server, std::bind(&Client::handshake_handler, shared_from_this(), std::placeholders::_1));
        }
        void handshake_handler(const boost::system::error_code& ec) {
            if (ec) { return; }
            async_read_hash();
        }
        void async_read_hash() {
            async_read(m_sock, buffer(&m_hash[0], common::HASH_SIZE), bind(&Client::hash_handler));
        }
        void hash_handler(const boost::system::error_code& ec, size_t bytes) {
            if (ec || bytes < common::HASH_SIZE) { return; } // may end here
            m_hash[common::HASH_SIZE] = '\0';
            m_msgHeader.blockSize = m_block->getBlockSize(m_hash);
            std::cout << "sending data [" << m_port << "] " << m_msgHeader.blockSize << std::endl;
            async_write(m_sock, buffer(&m_msgHeader, sizeof(common::BlockMsgHeader)), bind(&Client::header_sent_handler));
        }
        void header_sent_handler(const boost::system::error_code& ec, std::size_t bytes) {
            if (ec || bytes < sizeof(common::BlockMsgHeader)) { return; }
            m_chunk.offs = 0;
            if (m_msgHeader.blockSize <= 0) { // next block
                async_read_hash();
                return;
            }
            async_write_chunk();
        }
        void async_write_chunk() {
            auto dataLen = m_msgHeader.blockSize;
            size_t bufsz = dataLen < common::CHUNK_SIZE? dataLen : common::CHUNK_SIZE; // trick
            m_chunk.len = m_block->getBlockData(m_hash, m_chunk.d, bufsz); // there is no method to get chunk. get bufsz (offs unused)
            if (m_chunk.len <= 0) { // next block
                async_read_hash();
                return;
            }
            async_write(m_sock, buffer(m_chunk.d, m_chunk.len), bind(&Client::chunk_sent_handler));
        }
        void chunk_sent_handler(const boost::system::error_code& ec, std::size_t bytes) {
            if (ec || bytes < m_chunk.len) { return; }
            m_chunk.offs += bytes;
            m_msgHeader.blockSize -= bytes;
            async_write_chunk();
        }
    };

    void client_handler(ip::tcp::socket &sock) {
        auto c = Client::create(ssl::stream<ip::tcp::socket>(std::move(sock), m_context), m_block);
        c->start_process(); // auto destroy inside
    }
};


ServerTcp::ServerTcp(std::shared_ptr<IBlock> block)
    : IServer(std::move(block)), m_impl(std::make_unique<ServerTcp::Impl>(m_block))
{
}

ServerTcp::~ServerTcp()
{
}

void ServerTcp::loop()
{
    m_impl->process();
}
