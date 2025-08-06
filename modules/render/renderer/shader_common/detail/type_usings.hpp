#pragma once
#include "SkrBase/math.h"
#include "SkrContainersDef/vector.hpp"

namespace skr::data_layout
{
using namespace skr;

struct DataReader
{
public:
    uint32_t GetCapacity() const;
    uint32_t GetSize() const;

protected:
    DataReader(skr::Vector<uint8_t>&& b)
        : buffer(b)
    {
    
    }
    skr::Vector<uint8_t> buffer;
};

struct DataWriter : public DataReader
{

};

} // namespace skr::data_layout