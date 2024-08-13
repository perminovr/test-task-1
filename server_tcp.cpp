#include "server_tcp.h"
#include "thread_pool.h"
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/thread.hpp>
#include <thread>
#include <iostream>

using namespace boost::asio;

/*

Сервер работает на одном сокете TCP
По подключению клиента создается поток для обработки соединения
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
        m_acceptor {m_service, ip::tcp::endpoint(ip::tcp::v4(), common::TCP_SERVER_PORT)},
        m_sock {m_service}
    {
    }
    ~Impl() = default;

    void process() {
        m_acceptor.listen(socket_base::max_connections);
        for (;;) {
            boost::system::error_code ec;
            auto client = m_acceptor.accept(ec);
            if (ec) { client.close(); continue; }
            m_tp.create_thread([this, client = std::move(client) ]() mutable { client_handler(std::move(client)); });
        }
        m_tp.join_all();
    }

protected:
    std::shared_ptr<IBlock> m_block;
    ThreadPool m_tp;
    io_service m_service;
    ip::tcp::acceptor m_acceptor;
    ip::tcp::socket m_sock;

    void client_handler(ip::tcp::socket client) {
        boost::system::error_code ec;
        char data[common::CHUNK_SIZE];
        std::string hash;
        hash.reserve(common::HASH_SIZE+1);
        auto client_port = client.remote_endpoint().port();
        std::cout << "handling client [" << client_port << "] " << std::endl;
        // handle hashes
        for (;;) {
            auto len = client.read_some(buffer(&hash[0], common::HASH_SIZE), ec);
            if (ec || len < common::HASH_SIZE) { return; }
            hash[common::HASH_SIZE] = '\0';
            // data info
            auto dataLen = m_block->getBlockSize(hash);
            auto ch = reinterpret_cast<common::BlockMsgHeader *>(data);
            ch->blockSize = dataLen;
            auto wlen = client.write_some(buffer(ch, sizeof(common::BlockMsgHeader)), ec);
            if (ec || wlen < sizeof(common::BlockMsgHeader)) { return; }
            // chunk data
            std::cout << "sending data [" << client_port << "] " << dataLen << std::endl;
            size_t offs = 0;
            while (dataLen) {
                size_t bufsz = dataLen < common::CHUNK_SIZE? dataLen : common::CHUNK_SIZE; // trick
                len = m_block->getBlockData(hash, data, bufsz); // there is no method to get chunk. get bufsz (offs unused)
                if (ec || len <= 0) { break; }
                wlen = client.write_some(buffer(data, len));
                if (ec || wlen < len) { return; }
                dataLen -= len;
                offs += len;
            }
        }
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
