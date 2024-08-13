#ifndef DATA_LIST_RND_H
#define DATA_LIST_RND_H

#include "data_list.h"
#include "common.h"
#include <atomic>

class DataListRnd : public IDataList {
public:
    DataType getNext() override {
        if (m_sz-- <= 0) { return {}; }
        std::string hash;
        hash.reserve(common::HASH_SIZE+1);
        hash[common::HASH_SIZE] = '\0';
        return hash;
    }
    int getSize() override {
        return m_sz;
    }
    void setSize(int sz) { m_sz = sz; }
protected:
    std::atomic<int> m_sz;
};

#endif // DATA_LIST_RND_H