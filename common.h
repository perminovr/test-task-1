#ifndef COMMON_H
#define COMMON_H

#include <memory>

namespace common {

constexpr const unsigned TCP_SERVER_PORT = 10001;
constexpr const unsigned CHUNK_SIZE = (1<<20);
constexpr const unsigned HASH_SIZE = 128;
constexpr const unsigned SERVER_THREADS_MAX = 16;

constexpr const char *DH2048 = "../key/dh2048.pem";
constexpr const char *SERVER_CRT = "../key/server.crt";
constexpr const char *SERVER_KEY = "../key/server.key";
constexpr const char *ROOTCA_CRT = "../key/rootca.crt";

struct BlockMsgHeader {
    unsigned blockSize;
};

};

#endif // COMMON_H
