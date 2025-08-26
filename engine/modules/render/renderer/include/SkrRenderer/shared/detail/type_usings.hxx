#pragma once
#include <std/types/vec.hpp>
#include <std/types/matrix.hpp>
#include <std/resources/buffer.hpp>

#define sreflect_managed_component(...) struct 

namespace skr::data_layout
{
using ::float2;
using ::float4;

using ::float2x2;
using ::float4x4;

using ::ByteAddressBuffer;
using ::RWByteAddressBuffer;

using uint32 = uint;
using uint32_t = uint;
using uint64_t = uint64;
using AddressType = uint32_t;

template <bool Writable = false>
struct DataAccessor
{
public:
    uint32_t GetCapacity() const { return _capacity; }
    uint32_t GetSize() const { return _size; }

    DataAccessor(ByteAddressBuffer b)
        : buffer(b)
    {
        _size = buffer.Load(0);
        _capacity = buffer.Load(4);
    }

protected:
    ByteAddressBuffer buffer;
    uint32_t _size = 0;
    uint32_t _capacity = 0;
};


} // namespace skr::data_layout