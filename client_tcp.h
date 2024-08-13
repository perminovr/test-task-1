#ifndef CLIENT_TCP_H
#define CLIENT_TCP_H

#include "client.h"

class ClientTcp : public IClient {
public:
    explicit ClientTcp(std::shared_ptr<IDataList> dl);
    ~ClientTcp();
    void getData() override;
protected:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // CLIENT_TCP_H
