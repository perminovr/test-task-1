#include "client_tcp.h"
#include "thread_pool.h"
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/thread.hpp>
#include <iostream>

using namespace boost::asio;

/*

Клиент, зная число запрашиваемых блоков, выбирает необходимое число подключений к серверу
Каждое подключение выполняется в отдельном потоке
Хэши блоков извлекаются потоками из общей очереди
Если очередь пуста, то поток завершает работу
Хэш передается серверу
В ответ сервер присылает заголовок блока, содержащий его размер, и отправляет содержимое блока

*/

class ClientConnection {
public:
    explicit ClientConnection(ip::tcp::socket sock, ip::tcp::endpoint server_ep, std::shared_ptr<IDataList> dl)
        : 
        m_sock(std::move(sock)), 
        m_server_ep(server_ep),
        m_dl(std::move(dl))
    {
    }

    void process() {
        boost::system::error_code ec;
        char data[common::CHUNK_SIZE];
        common::BlockMsgHeader h;
        // connect to server
        m_sock.connect(m_server_ep, ec);
        if (ec) { return; }
        auto client_port = m_sock.local_endpoint().port();
        // handle blocks
        for (;;) {
            auto hash = m_dl->getNext();
            if (!hash) { return; }
            // hash to server
            auto wlen = m_sock.write_some(buffer(hash->c_str(), common::HASH_SIZE), ec);
            if (ec || wlen < common::HASH_SIZE) { return; }
            // get block header
            auto len = m_sock.read_some(buffer(&h, sizeof(h)), ec);
            if (ec || len < sizeof(h)) { return; }
            // get block
            auto dataLen = h.blockSize;
            std::cout << "getting data [" << client_port << "] " << dataLen << std::endl;
            size_t offs = 0;
            while (dataLen) {
                len = m_sock.read_some(buffer(data, common::CHUNK_SIZE), ec);
                if (ec || len <= 0) { return; }
                dataLen -= len;
                offs += len;
            }
        }
    }

protected:
    ip::tcp::socket m_sock;
    ip::tcp::endpoint m_server_ep;
    std::shared_ptr<IDataList> m_dl;
};

class ClientTcp::Impl {
public:
    explicit Impl(std::shared_ptr<IDataList> dl) 
        : 
        m_dl(std::move(dl))
    {
    }
    ~Impl() = default;

    void process() {
        ip::tcp::endpoint ep {ip::tcp::v4(), common::TCP_SERVER_PORT};
        const auto thr_max = threadMax();
        for (unsigned i = 0; i < thr_max; ++i) {
            ClientConnection cc( ip::tcp::socket{m_service}, ep, m_dl );
            m_tp.add_thread(std::thread([this, cc = std::move(cc)]() mutable { connection_handler(std::move(cc)); }));
        }
        m_tp.join_all();
    }

protected:
    std::shared_ptr<IDataList> m_dl;
    io_service m_service;
    ThreadPool m_tp;

    void connection_handler(ClientConnection cc) {
        cc.process();
    }
    unsigned threadMax() {
        auto sz = static_cast<unsigned>(m_dl->getSize());
        auto hw = std::thread::hardware_concurrency();
        return sz < hw? sz : hw;
    }
};


void ClientTcp::getData()
{
    m_impl->process();
}

ClientTcp::ClientTcp(std::shared_ptr<IDataList> dl)
    : IClient(std::move(dl)), m_impl(std::make_unique<ClientTcp::Impl>(m_dl))
{
}

ClientTcp::~ClientTcp()
{
}
