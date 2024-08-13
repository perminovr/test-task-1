#ifndef SERVER_TCP_H
#define SERVER_TCP_H

#include "server.h"


class ServerTcp : public IServer {
public:
    explicit ServerTcp(std::shared_ptr<IBlock> block);
    ~ServerTcp();
    void loop() override;
protected:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // SERVER_TCP_H
