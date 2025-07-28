#include "SkrToolCore/asset/cook_system.hpp"
#include "SkrToolCore/asset/importer.hpp"
#include "SkrSerde/json_serde.hpp"
#include "SkrContainers/hashmap.hpp"

namespace skd::asset
{
struct ImporterRegistryImpl : public ImporterRegistry {
    Importer* LoadImporter(const AssetRecord* record, skr::archive::JsonReader* object, skr_guid_t* pGuid = nullptr) override;
    uint32_t   GetImporterVersion(skr_guid_t type) override;
    void       RegisterImporter(skr_guid_t type, ImporterTypeInfo info) override;

    skr::FlatHashMap<skr_guid_t, ImporterTypeInfo, skr::Hash<skr_guid_t>> loaders;
};

ImporterRegistry* GetImporterRegistry()
{
    static ImporterRegistryImpl registry;
    return &registry;
}

Importer* ImporterRegistryImpl::LoadImporter(const AssetRecord* record, skr::archive::JsonReader* object, skr_guid_t* pGuid)
{
    skr_guid_t type;
    {
        object->StartObject(); // start importer object
        object->Key(u8"importerType");
        skr::json_read(object, type);
        object->EndObject();
    }
    if (pGuid) *pGuid = type;
    auto iter = loaders.find(type);
    if (iter != loaders.end())
    {
        object->Key(u8"importer");
        return iter->second.Load(record, object);
    }
    return nullptr;
}
uint32_t ImporterRegistryImpl::GetImporterVersion(skr_guid_t type)
{
    auto iter = loaders.find(type);
    if (iter != loaders.end())
        return iter->second.Version();
    return UINT32_MAX;
}

void ImporterRegistryImpl::RegisterImporter(skr_guid_t type, ImporterTypeInfo info)
{
    loaders.insert({ type, info });
}
} // namespace skd::asset
