#pragma once
#include "SkrSerde/json_serde.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrToolCore/cook_system/importer.hpp"
#ifndef __meta__
    #include "SkrToolCore/cook_system/asset_meta.generated.h"
#endif

namespace skd::asset
{

sreflect_struct(guid = "8e78354d-acf9-40e2-8175-1ad40654360d" serde = @json)
TOOL_CORE_API AssetMetadata
{
public:
    using LoadFromJson = bool (*)(skr::archive::JsonReader* reader, skr::RC<AssetMetadata> object);
    using StoreToJson = bool (*)(skr::archive::JsonWriter* writer, skr::RC<AssetMetadata> object);
    virtual ~AssetMetadata();

    template <typename T>
    inline static skr::RC<T> Create()
    {
        auto m = skr::RC<T>::New();
        m->Load = +[](skr::archive::JsonReader* reader, skr::RC<AssetMetadata> object) {
            auto derived = object.cast_static<T>();
            return skr::json_read<T>(reader, *derived);
        };
        m->Store = +[](skr::archive::JsonWriter* writer, skr::RC<AssetMetadata> object) {
            auto derived = object.cast_static<T>();
            return skr::json_write<T>(writer, *derived);
        };
        return m;
    }

protected:
    sattr(serde = @disable)
    LoadFromJson Load;

    sattr(serde = @disable)
    StoreToJson Store;

    friend struct JsonSerde<skd::asset::AssetMetaFile>;
    AssetMetadata();
    SKR_RC_IMPL();
};

sreflect_struct(guid = "6c429147-d680-4345-a4a3-e8aefd671e6a")
TOOL_CORE_API AssetMetaFile final
{
public:
    AssetMetaFile(const URI& uri);
    AssetMetaFile(const URI& uri, skr::GUID guid, skr::GUID resource_type, skr::GUID cooker);

    template <typename T>
    inline skr::RC<T> GetMetadata();
    inline auto GetProject() const { return project; }
    inline const auto& GetURI() const { return uri; }
    inline skr::GUID GetGUID() const { return guid; }
    inline skr::GUID GetResourceType() const { return resource_type; }
    inline skr::GUID GetCooker() const { return cooker; }
    inline auto GetImporter() const { return importer; }

private:
    inline void SetContent(skr::String && content)
    {
        meta_content = std::move(content);
    }

    URI uri;
    skr::GUID guid;
    skr::GUID resource_type;
    skr::GUID cooker;

    sattr(serde = @disable)
    skr::RC<AssetMetadata> metadata = nullptr;

    sattr(serde = @disable)
    skr::RC<Importer> importer = nullptr;

    sattr(serde = @disable)
    const SProject* project = nullptr;

    sattr(serde = @disable)
    skr::String meta_content;

    friend struct CookSystemImpl;
    friend struct JsonSerde<skd::asset::AssetMetaFile>;
    SKR_RC_IMPL();
};

template <typename T>
inline skr::RC<T> AssetMetaFile::GetMetadata()
{
    if (metadata != nullptr)
    {
        return metadata;
    }
    else if (!meta_content.is_empty())
    {
        auto METADATA = AssetMetadata::Create<T>();
        skr::archive::JsonReader reader(meta_content.view());
        reader.StartObject();
        reader.Key(u8"metadata");
        skr::json_read(&reader, *METADATA);
        reader.EndObject();
        metadata = METADATA;
        return METADATA;
    }
    return nullptr;
}

} // namespace skd::asset

namespace skr
{
template <>
struct JsonSerde<skd::asset::AssetMetaFile>
{
    inline static bool Parse(skr::archive::JsonReader& reader, const char8_t* key, bool required = true)
    {
        bool exist = true;
        auto parse = reader.Key(key);
        parse.error_then([&](skr::archive::JsonReadError e) {
            exist = false;
            if (!required && (e == skr::archive::JsonReadError::KeyNotFound))
                return;
            SKR_LOG_FATAL(u8"Parse asset meta file failed, error code %d");
        });
        return exist;
    }

    inline static bool read(skr::archive::JsonReader* r, skd::asset::AssetMetaFile& v)
    {
        r->StartObject();
        bool _ = read_fields(r, v);
        r->EndObject();
        return _;
    }

    inline static bool write(skr::archive::JsonWriter* w, const skd::asset::AssetMetaFile& v)
    {
        w->StartObject();
        bool _ = write_fields(w, v);
        w->EndObject();
        return _;
    }

    inline static bool read_fields(skr::archive::JsonReader* r, skd::asset::AssetMetaFile& v)
    {
        // Parse guid and type first
        auto& reader = *r;
        if (Parse(reader, u8"guid"))
            skr::json_read(&reader, v.guid);
        if (Parse(reader, u8"resource_type"))
            skr::json_read(&reader, v.resource_type);
        if (Parse(reader, u8"cooker", false))
            skr::json_read(&reader, v.cooker);
        // construct importer
        if (Parse(reader, u8"importer", false))
            v.importer = skd::asset::GetImporterRegistry()->LoadImporter(&reader);
        return true;
    }

    inline static bool write_fields(skr::archive::JsonWriter* w, const skd::asset::AssetMetaFile& v)
    {
        w->Key(u8"guid");
        skr::json_write(w, v.guid);

        w->Key(u8"resource_type");
        skr::json_write(w, v.resource_type);

        w->Key(u8"cooker");
        skr::json_write(w, v.cooker);

        if (v.importer)
        {
            w->Key(u8"importer");
            skd::asset::GetImporterRegistry()->StoreImporter(w, v.importer);
        }

        if (v.metadata != nullptr)
        {
            w->Key(u8"metadata");
            v.metadata->Store(w, v.metadata);
        }

        return true;
    }
};
} // namespace skr
