#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include "block.h"

class IServer {
public:
    explicit IServer(std::shared_ptr<IBlock> block) : m_block(std::move(block)) {
    }
    virtual ~IServer() = default;
    virtual void loop() = 0;
protected:
    std::shared_ptr<IBlock> m_block;
};

#endif // SERVER_H
