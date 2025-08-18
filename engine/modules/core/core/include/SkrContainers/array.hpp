#pragma once
#include "SkrContainersDef/array.hpp"

// bin serde
#include "SkrSerde/bin_serde.hpp"
namespace skr
{
template <typename T, size_t N>
struct BinSerde<skr::Array<T, N>> {
    inline static bool read(SBinaryReader* r, skr::Array<T, N>& v)
    {
        for (size_t i = 0; i < N; ++i)
        {
            if (!bin_read(r, v[i]))
                return false;
        }
        return true;
    }
    inline static bool write(SBinaryWriter* w, const skr::Array<T, N>& v)
    {
        for (size_t i = 0; i < N; ++i)
        {
            if (!bin_write(w, v[i]))
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
template <typename T, size_t N>
struct JsonSerde<skr::Array<T, N>> {
    inline static bool read(skr::archive::JsonReader* r, skr::Array<T, N>& v)
    {
        size_t count;
        SKR_EXPECTED_CHECK(r->StartArray(count), false);
        if (count != N)
        {
            SKR_LOG_ERROR(u8"Array size mismatch: expected %zu, got %zu", N, count);
            return false;
        }
        for (size_t i = 0; i < N; ++i)
        {
            if (!json_read<T>(r, v[i]))
                return false;
        }
        SKR_EXPECTED_CHECK(r->EndArray(), false);
        return true;
    }
    inline static bool write(skr::archive::JsonWriter* w, const skr::Array<T, N>& v)
    {
        SKR_EXPECTED_CHECK(w->StartArray(), false);
        for (size_t i = 0; i < N; ++i)
        {
            if (!json_write<T>(w, v[i]))
                return false;
        }
        SKR_EXPECTED_CHECK(w->EndArray(), false);
        return true;
    }
};
} // namespace skr