#include "SkrToolCore/cook_system/cook_system.hpp"
#include "SkrToolCore/cook_system/importer.hpp"
#include "SkrSerde/json_serde.hpp"
#include "SkrContainers/hashmap.hpp"

namespace skd::asset
{
Importer::Importer()
{
    
}

struct ImporterRegistryImpl : public ImporterRegistry
{
    skr::RC<Importer> LoadImporter(skr::archive::JsonReader* json) override;
    void StoreImporter(skr::archive::JsonWriter* writer, skr::RC<Importer> importer) override;
    uint32_t GetImporterVersion(skr::GUID type) override;
    void RegisterImporter(skr::GUID type, ImporterTypeInfo info) override;

    skr::FlatHashMap<skr::GUID, ImporterTypeInfo, skr::Hash<skr::GUID>> importer_types;
};

ImporterRegistry* GetImporterRegistry()
{
    static ImporterRegistryImpl registry;
    return &registry;
}

skr::RC<Importer> ImporterRegistryImpl::LoadImporter(skr::archive::JsonReader* json)
{
    skr::GUID type;
    json->StartObject();
    {
        json->Key(u8"importer_type");
        skr::json_read(json, type);
    }
    json->EndObject();

    auto iter = importer_types.find(type);
    if (iter != importer_types.end())
    {
        auto importer = iter->second.Create();
        json->Key(u8"importer");
        if (iter->second.Load(json, importer))
            return importer;
    }
    return nullptr;
}

void ImporterRegistryImpl::StoreImporter(skr::archive::JsonWriter* writer, skr::RC<Importer> importer) 
{
    auto iter = importer_types.find(importer->GetType());
    if (iter != importer_types.end())
    {
        iter->second.Store(writer, importer);
    }
}

uint32_t ImporterRegistryImpl::GetImporterVersion(skr::GUID type)
{
    auto iter = importer_types.find(type);
    if (iter != importer_types.end())
        return iter->second.Version();
    return UINT32_MAX;
}

void ImporterRegistryImpl::RegisterImporter(skr::GUID type, ImporterTypeInfo info)
{
    importer_types.insert({ type, info });
}
} // namespace skd::asset
