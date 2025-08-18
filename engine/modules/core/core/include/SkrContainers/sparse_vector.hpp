#pragma once
#include "SkrContainersDef/sparse_vector.hpp"

// rttr
#include "SkrRTTR/type_signature.hpp"
namespace skr
{
static constexpr GUID kSparseVectorGenericId = u8"3cc29798-9798-412c-b493-beefe6653356"_guid;
template <typename T>
struct TypeSignatureTraits<::skr::SparseVector<T>> {
    inline static constexpr size_t buffer_size = type_signature_size_v<ETypeSignatureSignal::GenericTypeId> + TypeSignatureTraits<T>::buffer_size;
    inline static uint8_t*         write(uint8_t* pos, uint8_t* end)
    {
        pos = TypeSignatureHelper::write_generic_type_id(pos, end, kSparseVectorGenericId, 1);
        return TypeSignatureTraits<T>::write(pos, end);
    }
};
} // namespace skr

// bin serde
#include "SkrSerde/bin_serde.hpp"
namespace skr
{
template <typename T>
struct BinSerde<skr::SparseVector<T>> {
    inline static bool read(SBinaryReader* r, skr::SparseVector<T>& v)
    {
        // read size
        uint32_t size;
        if (!bin_read(r, size)) return false;

        // read sparse indices and values
        skr::SparseVector<T> temp;
        for (uint32_t i = 0; i < size; ++i)
        {
            uint64_t index;
            T value;
            if (!bin_read(r, index))
                return false;
            if (!bin_read(r, value))
                return false;
            temp.add_at(index, std::move(value));
        }

        // move to target
        v = std::move(temp);
        return true;
    }
    inline static bool write(SBinaryWriter* w, const skr::SparseVector<T>& v)
    {
        // write size
        uint32_t size = static_cast<uint32_t>(v.size());
        if (!bin_write(w, size)) return false;

        // write sparse indices and values
        for (const auto& slot : v)
        {
            if (!bin_write(w, slot.index)) return false;
            if (!bin_write(w, slot.value)) return false;
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
struct JsonSerde<skr::SparseVector<T>> {
    inline static bool read(skr::archive::JsonReader* r, skr::SparseVector<T>& v)
    {
        size_t count;
        SKR_EXPECTED_CHECK(r->StartArray(count), false);
        
        skr::SparseVector<T> temp;
        for (size_t i = 0; i < count; ++i)
        {
            SKR_EXPECTED_CHECK(r->StartObject(), false);
            
            SKR_EXPECTED_CHECK(r->Key(u8"index"), false);
            uint64_t index;
            if (!json_read(r, index))
                return false;
                
            SKR_EXPECTED_CHECK(r->Key(u8"value"), false);
            T value;
            if (!json_read<T>(r, value))
                return false;
                
            SKR_EXPECTED_CHECK(r->EndObject(), false);
            
            temp.add_at(index, std::move(value));
        }
        
        SKR_EXPECTED_CHECK(r->EndArray(), false);
        v = std::move(temp);
        return true;
    }
    inline static bool write(skr::archive::JsonWriter* w, const skr::SparseVector<T>& v)
    {
        SKR_EXPECTED_CHECK(w->StartArray(), false);
        
        for (const auto& slot : v)
        {
            SKR_EXPECTED_CHECK(w->StartObject(), false);
            
            SKR_EXPECTED_CHECK(w->Key(u8"index"), false);
            if (!json_write(w, slot.index))
                return false;
                
            SKR_EXPECTED_CHECK(w->Key(u8"value"), false);
            if (!json_write<T>(w, slot.value))
                return false;
                
            SKR_EXPECTED_CHECK(w->EndObject(), false);
        }
        
        SKR_EXPECTED_CHECK(w->EndArray(), false);
        return true;
    }
};
} // namespace skr