#pragma once
#include "SkrContainersDef/set.hpp"

// rttr
#include "SkrRTTR/type_signature.hpp"
namespace skr
{
inline constexpr GUID kSetGenericId = u8"7218b701-572f-48f3-ab3a-d6c0b31efeb0"_guid;
template <typename T>
struct TypeSignatureTraits<::skr::Set<T>> {
    inline static constexpr size_t buffer_size = type_signature_size_v<ETypeSignatureSignal::GenericTypeId> + TypeSignatureTraits<T>::buffer_size;
    inline static uint8_t*         write(uint8_t* pos, uint8_t* end)
    {
        pos = TypeSignatureHelper::write_generic_type_id(pos, end, kSetGenericId, 1);
        return TypeSignatureTraits<T>::write(pos, end);
    }
};
} // namespace skr

// bin serde
#include "SkrSerde/bin_serde.hpp"
namespace skr
{
template <typename T>
struct BinSerde<skr::Set<T>> {
    inline static bool read(SBinaryReader* r, skr::Set<T>& v)
    {
        // read size
        uint32_t size;
        if (!bin_read(r, size)) return false;

        // read content
        skr::Set<T> temp;
        for (uint32_t i = 0; i < size; ++i)
        {
            T value;
            if (!bin_read(r, value))
                return false;
            temp.add(std::move(value));
        }

        // move to target
        v = std::move(temp);
        return true;
    }
    inline static bool write(SBinaryWriter* w, const skr::Set<T>& v)
    {
        // write size
        uint32_t size = static_cast<uint32_t>(v.size());
        if (!bin_write(w, size)) return false;

        // write content
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
struct JsonSerde<skr::Set<T>> {
    inline static bool read(skr::archive::JsonReader* r, skr::Set<T>& v)
    {
        size_t count;
        SKR_EXPECTED_CHECK(r->StartArray(count), false);
        for (size_t i = 0; i < count; ++i)
        {
            T value;
            if (!json_read<T>(r, value))
                return false;
            v.add(std::move(value));
        }
        SKR_EXPECTED_CHECK(r->EndArray(), false);
        return true;
    }
    inline static bool write(skr::archive::JsonWriter* w, const skr::Set<T>& v)
    {
        SKR_EXPECTED_CHECK(w->StartArray(), false);
        for (const auto& value : v)
        {
            if (!json_write<T>(w, value))
                return false;
        }
        SKR_EXPECTED_CHECK(w->EndArray(), false);
        return true;
    }
};
} // namespace skr