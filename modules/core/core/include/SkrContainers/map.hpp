#pragma once
#include "SkrContainersDef/map.hpp"

// rttr
#include "SkrRTTR/type_signature.hpp"
namespace skr
{
static constexpr GUID kMapGenericId = u8"9eae06c4-d7ea-4246-af0e-c95d401a7a71"_guid;
template <typename K, typename V>
struct TypeSignatureTraits<::skr::Map<K, V>> {
    inline static constexpr size_t buffer_size = type_signature_size_v<ETypeSignatureSignal::GenericTypeId> + TypeSignatureTraits<K>::buffer_size + TypeSignatureTraits<V>::buffer_size;
    inline static uint8_t*         write(uint8_t* pos, uint8_t* end)
    {
        pos = TypeSignatureHelper::write_generic_type_id(pos, end, kMapGenericId, 2);
        pos = TypeSignatureTraits<K>::write(pos, end);
        return TypeSignatureTraits<V>::write(pos, end);
    }
};
} // namespace skr

// json serde
#include "SkrSerde/json_serde.hpp"
namespace skr
{
template <typename K, typename V>
struct JsonSerde<skr::Map<K, V>> {
    inline static bool read(skr::archive::JsonReader* r, skr::Map<K, V>& v)
    {
        size_t count;
        SKR_EXPECTED_CHECK(r->StartArray(count), false);
        v.reserve(count / 2);
        for (size_t i = 0; i < count; i += 2)
        {
            K key;
            V value;

            if (!json_read<K>(r, key))
                return false;
            if (!json_read<V>(r, value))
                return false;
            v.add(std::move(key), std::move(value));
        }
        SKR_EXPECTED_CHECK(r->EndArray(), false);
        return true;
    }
    inline static bool write(skr::archive::JsonWriter* w, const skr::Map<K, V>& v)
    {
        SKR_EXPECTED_CHECK(w->StartArray(), false);
        for (const auto& [key, value] : v)
        {
            if (!json_write<K>(w, key)) return false;
            if (!json_write<V>(w, value)) return false;
        }
        SKR_EXPECTED_CHECK(w->EndArray(), false);
        return true;
    }
};
} // namespace skr