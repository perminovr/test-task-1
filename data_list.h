#ifndef DATA_LIST_H
#define DATA_LIST_H

#include <string>
#include <optional>

class IDataList {
public:
    using DataType = std::optional<std::string>; 
    virtual DataType getNext() = 0;
    virtual int getSize() = 0;
};

#endif // DATA_LIST_H