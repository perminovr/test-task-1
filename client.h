#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include "data_list.h"

class IClient {
public:
    explicit IClient(std::shared_ptr<IDataList> dl) : m_dl(std::move(dl)) {
    }
    virtual ~IClient() = default;
    virtual void getData() = 0;
protected:
    std::shared_ptr<IDataList> m_dl;
};

#endif // CLIENT_H
