#ifndef BLOCK_H
#define BLOCK_H

#include <string>

class IBlock {
public:
    virtual size_t getBlockNumber(const std::string &hash) const = 0;
    virtual size_t getBlockSize(const std::string &hash) const = 0;
    virtual size_t getBlockData(size_t block_num, char *buffer, size_t buffer_size) const = 0;
    inline size_t getBlockData(const std::string &hash, char *buffer, size_t buffer_size) const {
        return getBlockData(getBlockNumber(hash), buffer, buffer_size);
    }
};

#endif // BLOCK_H