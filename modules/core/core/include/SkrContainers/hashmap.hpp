#pragma once
#include "SkrContainersDef/hashmap.hpp" // IWYU pragma: export

// bin serde
#include "SkrSerde/bin_serde.hpp"
namespace skr
{
// Generic hashmap serialization template
namespace detail
{
template <template<class...> class Map, class K, class V, class Hash, class Eq>
struct BinSerdeHashMapImpl {
    inline static bool read(SBinaryReader* r, Map<K, V, Hash, Eq>& v)
    {
        // read size
        uint32_t size;
        if (!bin_read(r, size)) return false;

        // read content
        Map<K, V, Hash, Eq> temp;
        for (uint32_t i = 0; i < size; ++i)
        {
            K key;
            V value;
            if (!bin_read(r, key)) 
                return false;
            if (!bin_read(r, value)) 
                return false;
            temp.insert({ std::move(key), std::move(value) });
        }

        // move to target
        v = std::move(temp);
        return true;
    }
    inline static bool write(SBinaryWriter* w, const Map<K, V, Hash, Eq>& v)
    {
        // write size
        uint32_t size = static_cast<uint32_t>(v.size());
        if (!bin_write(w, size)) return false;

        // write content
        for (auto& pair : v)
        {
            if (!bin_write(w, pair.first)) return false;
            if (!bin_write(w, pair.second)) return false;
        }
        return true;
    }
};
} // namespace detail

// FlatHashMap specialization
template <class K, class V, class Hash, class Eq>
struct BinSerde<skr::FlatHashMap<K, V, Hash, Eq>> 
    : detail::BinSerdeHashMapImpl<skr::FlatHashMap, K, V, Hash, Eq> {};

// ParallelFlatHashMap specialization
template <class K, class V, class Hash, class Eq>
struct BinSerde<skr::ParallelFlatHashMap<K, V, Hash, Eq>> 
    : detail::BinSerdeHashMapImpl<skr::ParallelFlatHashMap, K, V, Hash, Eq> {};
} // namespace skr

// json serde
#include "SkrSerde/json_serde.hpp"
namespace skr
{
// Generic hashmap json serialization template
namespace detail
{
template <template<class...> class Map, class K, class V, class Hash, class Eq>
struct JsonSerdeHashMapImpl {
    inline static bool read(skr::archive::JsonReader* r, Map<K, V, Hash, Eq>& v)
    {
        size_t count = 0;
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
            v.emplace(std::move(key), std::move(value));
        }
        SKR_EXPECTED_CHECK(r->EndArray(), false);
        return true;
    }
    inline static bool write(skr::archive::JsonWriter* w, const Map<K, V, Hash, Eq>& v)
    {
        SKR_EXPECTED_CHECK(w->StartArray(), false);
        for (const auto& pair : v)
        {
            if (!json_write<K>(w, pair.first)) return false;
            if (!json_write<V>(w, pair.second)) return false;
        }
        SKR_EXPECTED_CHECK(w->EndArray(), false);
        return true;
    }
};
} // namespace detail

// FlatHashMap specialization
template <class K, class V, class Hash, class Eq>
struct JsonSerde<skr::FlatHashMap<K, V, Hash, Eq>> 
    : detail::JsonSerdeHashMapImpl<skr::FlatHashMap, K, V, Hash, Eq> {};

// ParallelFlatHashMap specialization
template <class K, class V, class Hash, class Eq>
struct JsonSerde<skr::ParallelFlatHashMap<K, V, Hash, Eq>> 
    : detail::JsonSerdeHashMapImpl<skr::ParallelFlatHashMap, K, V, Hash, Eq> {};
} // namespace skr