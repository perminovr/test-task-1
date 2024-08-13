#ifndef BLOCK_RND_H
#define BLOCK_RND_H

#include "block.h"

class BlockRnd : public IBlock {
public:
    size_t getBlockNumber(const std::string &hash) const override {
        return rand();
    }
    size_t getBlockSize(const std::string &hash) const override {
        return (rand() + 1) % (1<<20);
    }
    size_t getBlockData(size_t block_num, char *buffer, size_t buffer_size) const override {
        return buffer_size;
    }
};

#endif // BLOCK_RND_H