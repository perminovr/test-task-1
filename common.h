#ifndef COMMON_H
#define COMMON_H

#include <memory>

namespace common {

constexpr const unsigned TCP_SERVER_PORT = 10001;
constexpr const unsigned CHUNK_SIZE = (1<<20);
constexpr const unsigned HASH_SIZE = 128;

struct BlockMsgHeader {
    unsigned blockSize;
};

};

#endif // COMMON_H
