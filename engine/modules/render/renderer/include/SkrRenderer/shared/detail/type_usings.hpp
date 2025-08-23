#pragma once
#include "SkrBase/math.h" // IWYU pragma: export
#include "SkrRT/ecs/component.hpp" // IWYU pragma: export
#include "SkrContainersDef/vector.hpp"

namespace skr::data_layout
{
using namespace skr;
using uint32 = ::uint32_t;
using uint32_t = ::uint32_t;
using uint64 = ::uint64_t;
using uint64_t = ::uint64_t;
using AddressType = uint32_t;

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