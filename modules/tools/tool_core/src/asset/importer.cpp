#include "SkrToolCore/asset/cook_system.hpp"
#include "SkrToolCore/asset/importer.hpp"
#include "SkrSerde/json_serde.hpp"
#include "SkrContainers/hashmap.hpp"

namespace skd::asset
{
struct ImporterRegistryImpl : public ImporterRegistry
{
    Importer* LoadImporter(const AssetInfo* record, skr::archive::JsonReader* object, skr::GUID* pGuid = nullptr) override;
    uint32_t GetImporterVersion(skr::GUID type) override;
    void RegisterImporter(skr::GUID type, ImporterTypeInfo info) override;

    skr::FlatHashMap<skr::GUID, ImporterTypeInfo, skr::Hash<skr::GUID>> loaders;
};

ImporterRegistry* GetImporterRegistry()
{
    static ImporterRegistryImpl registry;
    return &registry;
}

Importer* ImporterRegistryImpl::LoadImporter(const AssetInfo* record, skr::archive::JsonReader* object, skr::GUID* pGuid)
{
    skr::GUID type;
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

uint32_t ImporterRegistryImpl::GetImporterVersion(skr::GUID type)
{
    auto iter = loaders.find(type);
    if (iter != loaders.end())
        return iter->second.Version();
    return UINT32_MAX;
}

void ImporterRegistryImpl::RegisterImporter(skr::GUID type, ImporterTypeInfo info)
{
    loaders.insert({ type, info });
}
} // namespace skd::asset
