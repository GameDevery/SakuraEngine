#pragma once
#include "SkrContainersDef/path.hpp"// IWYU pragma: export
#include "SkrContainers/string.hpp" // IWYU pragma: export

// bin serde
#include "SkrSerde/bin_serde.hpp"
namespace skr
{
template <>
struct BinSerde<Path> {
    inline static bool read(SBinaryReader* r, Path& p)
    {
        // read path string
        String path_str;
        if (!bin_read(r, path_str)) return false;
        // construct path
        p = Path(std::move(path_str), skr::Path::Universal);
        return true;
    }
    
    inline static bool write(SBinaryWriter* w, const Path& p)
    {
        // write path string
        if (!bin_write(w, p.string())) return false;
        return true;
    }
};
} // namespace skr

// json serde
#include "SkrSerde/json_serde.hpp"
namespace skr
{
template <>
struct JsonSerde<Path> {
    inline static bool read(skr::archive::JsonReader* r, Path& p)
    {
        skr::String v;
        JsonSerde<skr::String>::read(r, v);
        p = skr::Path(std::move(v));
        return true;
    }
    
    inline static bool write(skr::archive::JsonWriter* w, const Path& p)
    {
        JsonSerde<skr::String>::write(w, p.string());
        return true;
    }
};
} // namespace skr