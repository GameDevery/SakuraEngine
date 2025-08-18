#pragma once
#include "SkrContainersDef/ring_buffer.hpp"

// bin serde
#include "SkrSerde/bin_serde.hpp"
namespace skr
{
template <typename T>
struct BinSerde<skr::RingBuffer<T>> {
    inline static bool read(SBinaryReader* r, skr::RingBuffer<T>& v)
    {
        // read capacity and size
        uint32_t capacity, size;
        if (!bin_read(r, capacity)) return false;
        if (!bin_read(r, size)) return false;

        // recreate ring buffer
        skr::RingBuffer<T> temp(capacity);
        for (uint32_t i = 0; i < size; ++i)
        {
            T value;
            if (!bin_read(r, value))
                return false;
            temp.enqueue(std::move(value));
        }

        // move to target
        v = std::move(temp);
        return true;
    }
    inline static bool write(SBinaryWriter* w, const skr::RingBuffer<T>& v)
    {
        // write capacity and size
        uint32_t capacity = static_cast<uint32_t>(v.capacity());
        uint32_t size = static_cast<uint32_t>(v.size());
        if (!bin_write(w, capacity)) return false;
        if (!bin_write(w, size)) return false;

        // write content in order
        for (const auto& value : v)
        {
            if (!bin_write(w, value))
                return false;
        }
        return true;
    }
};
} // namespace skr

// json serde
#include "SkrSerde/json_serde.hpp"
namespace skr
{
template <typename T>
struct JsonSerde<skr::RingBuffer<T>> {
    inline static bool read(skr::archive::JsonReader* r, skr::RingBuffer<T>& v)
    {
        SKR_EXPECTED_CHECK(r->StartObject(), false);
        
        SKR_EXPECTED_CHECK(r->Key(u8"capacity"), false);
        uint32_t capacity;
        if (!json_read(r, capacity))
            return false;
            
        SKR_EXPECTED_CHECK(r->Key(u8"data"), false);
        size_t count;
        SKR_EXPECTED_CHECK(r->StartArray(count), false);
        
        skr::RingBuffer<T> temp(capacity);
        for (size_t i = 0; i < count; ++i)
        {
            T value;
            if (!json_read<T>(r, value))
                return false;
            temp.enqueue(std::move(value));
        }
        SKR_EXPECTED_CHECK(r->EndArray(), false);
        
        SKR_EXPECTED_CHECK(r->EndObject(), false);
        v = std::move(temp);
        return true;
    }
    inline static bool write(skr::archive::JsonWriter* w, const skr::RingBuffer<T>& v)
    {
        SKR_EXPECTED_CHECK(w->StartObject(), false);
        
        SKR_EXPECTED_CHECK(w->Key(u8"capacity"), false);
        if (!json_write(w, static_cast<uint32_t>(v.capacity())))
            return false;
            
        SKR_EXPECTED_CHECK(w->Key(u8"data"), false);
        SKR_EXPECTED_CHECK(w->StartArray(), false);
        for (const auto& value : v)
        {
            if (!json_write<T>(w, value))
                return false;
        }
        SKR_EXPECTED_CHECK(w->EndArray(), false);
        
        SKR_EXPECTED_CHECK(w->EndObject(), false);
        return true;
    }
};
} // namespace skr