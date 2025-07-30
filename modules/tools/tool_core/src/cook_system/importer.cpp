#include "SkrToolCore/cook_system/cook_system.hpp"
#include "SkrToolCore/cook_system/importer.hpp"
#include "SkrSerde/json_serde.hpp"
#include "SkrContainers/hashmap.hpp"

namespace skd::asset
{
struct ImporterFactoryImpl : public ImporterFactory
{
    skr::RC<Importer> LoadImporter(skr::archive::JsonReader* object) override;
    void StoreImporter(skr::archive::JsonWriter* writer, skr::RC<Importer> importer) override;
    uint32_t GetImporterVersion(skr::GUID type) override;
    void RegisterImporter(skr::GUID type, ImporterTypeInfo info) override;

    skr::FlatHashMap<skr::GUID, ImporterTypeInfo, skr::Hash<skr::GUID>> importer_types;
};

ImporterFactory* GetImporterFactory()
{
    static ImporterFactoryImpl registry;
    return &registry;
}

skr::RC<Importer> ImporterFactoryImpl::LoadImporter(skr::archive::JsonReader* object)
{
    skr::GUID type;
    object->StartObject();
    {
        object->Key(u8"importer_type");
        skr::json_read(object, type);
    }
    object->EndObject();

    auto iter = importer_types.find(type);
    if (iter != importer_types.end())
    {
        object->Key(u8"importer");
        return iter->second.Load(object);
    }
    return nullptr;
}

void ImporterFactoryImpl::StoreImporter(skr::archive::JsonWriter* writer, skr::RC<Importer> importer) 
{
    auto iter = importer_types.find(importer->GetType());
    if (iter != importer_types.end())
    {
        writer->Key(u8"importer");
        return iter->second.Store(writer, importer);
    }
}

uint32_t ImporterFactoryImpl::GetImporterVersion(skr::GUID type)
{
    auto iter = importer_types.find(type);
    if (iter != importer_types.end())
        return iter->second.Version();
    return UINT32_MAX;
}

void ImporterFactoryImpl::RegisterImporter(skr::GUID type, ImporterTypeInfo info)
{
    importer_types.insert({ type, info });
}
} // namespace skd::asset
