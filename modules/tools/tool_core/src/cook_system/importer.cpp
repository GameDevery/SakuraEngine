#include "SkrToolCore/cook_system/cook_system.hpp"
#include "SkrToolCore/cook_system/importer.hpp"
#include "SkrSerde/json_serde.hpp"
#include "SkrContainers/hashmap.hpp"

namespace skd::asset
{
struct ImporterRegistryImpl : public ImporterRegistry
{
    skr::RC<Importer> LoadImporter(const AssetMetaFile* metafile, skr::GUID* pGuid = nullptr) override;
    uint32_t GetImporterVersion(skr::GUID type) override;
    void RegisterImporter(skr::GUID type, ImporterTypeInfo info) override;

    skr::FlatHashMap<skr::GUID, ImporterTypeInfo, skr::Hash<skr::GUID>> importer_types;
};

ImporterRegistry* GetImporterRegistry()
{
    static ImporterRegistryImpl registry;
    return &registry;
}

skr::RC<Importer> ImporterRegistryImpl::LoadImporter(const AssetMetaFile* metafile, skr::GUID* pGuid)
{
    skr::archive::JsonReader reader(metafile->meta.view());
    reader.StartObject();
    if (auto jobj = reader.Key(u8"importer"); jobj.has_value())
    {
        auto object = &reader;
        skr::GUID type;
        {
            object->StartObject(); // start importer object
            object->Key(u8"importerType");
            skr::json_read(object, type);
            object->EndObject();
        }
        if (pGuid) *pGuid = type;
        auto iter = importer_types.find(type);
        if (iter != importer_types.end())
        {
            object->Key(u8"importer");
            return iter->second.Load(metafile, object);
        }
    }
    reader.EndObject();
    return nullptr;
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
