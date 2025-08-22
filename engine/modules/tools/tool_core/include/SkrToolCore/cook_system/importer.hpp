#pragma once
#include "SkrBase/types/guid.h"
#include "SkrCore/memory/rc.hpp"
#include "SkrToolCore/fwd_types.hpp"
#ifndef __meta__
    #include "SkrToolCore/cook_system/importer.generated.h" // IWYU pragma: export
#endif

namespace skd::asset
{
using namespace skr;
template <class T>
void RegisterImporter(skr::GUID guid);

sreflect_struct(guid = "76044661-E2C9-43A7-A4DE-AEDD8FB5C847"; serde = @json)
TOOL_CORE_API Importer
{
public:
    SKR_GENERATE_BODY(Importer);

    using CreateFN = skr::RC<Importer> (*)();
    using LoadFromJson = bool (*)(skr::archive::JsonReader* reader, skr::RC<Importer> object);
    using StoreToJson = bool (*)(skr::archive::JsonWriter* writer, skr::RC<Importer> object);

    static constexpr uint32_t kDevelopmentVersion = UINT32_MAX;
    static uint32_t Version() { return kDevelopmentVersion; }

    virtual ~Importer() = default;
    virtual void* Import(skr::io::IRAMService*, CookContext * context) = 0;
    virtual void Destroy(void*) = 0;
    inline skr::GUID GetType() const { return importer_type; }

    template <typename T>
    inline static skr::RC<T> Create()
    {
        auto i = skr::RC<T>::New();
        i->Load = +[](skr::archive::JsonReader* reader, skr::RC<Importer> object) {
            auto derived = object.cast_static<T>();
            return skr::json_read<T>(reader, *derived);
        };
        i->Store = +[](skr::archive::JsonWriter* writer, skr::RC<Importer> object) {
            auto derived = object.cast_static<T>();
            return skr::json_write<T>(writer, *derived);
        };
        i->importer_type = skr::type_id_of<T>();
        return i;
    }

protected:
    skr::GUID importer_type;

    sattr(serde = @disable)
    LoadFromJson Load = nullptr;

    sattr(serde = @disable)
    StoreToJson Store = nullptr;

    Importer();
    SKR_RC_IMPL();
};

struct TOOL_CORE_API ImporterTypeInfo
{
    Importer::CreateFN Create;
    Importer::LoadFromJson Load;
    Importer::StoreToJson Store;
    uint32_t (*Version)();
};

struct ImporterRegistry
{
    virtual skr::RC<Importer> LoadImporter(skr::archive::JsonReader* importer) = 0;
    virtual void StoreImporter(skr::archive::JsonWriter* writer, skr::RC<Importer> importer) = 0;
    virtual uint32_t GetImporterVersion(skr::GUID type) = 0;
    virtual void RegisterImporter(skr::GUID type, ImporterTypeInfo info) = 0;
};

TOOL_CORE_API ImporterRegistry* GetImporterRegistry();
} // namespace skd::asset

template <class T>
void skd::asset::RegisterImporter(skr::GUID guid)
{
    auto registry = GetImporterRegistry();
    auto create = +[]() {
        auto derived = Importer::Create<T>();
        return derived.template cast_static<Importer>();
    };
    auto loader = +[](skr::archive::JsonReader* reader, skr::RC<Importer> object) {
        auto derived = object.cast_static<T>();
        return skr::json_read<T>(reader, *derived);
    };
    auto store = +[](skr::archive::JsonWriter* writer, skr::RC<Importer> object) {
        auto derived = object.cast_static<T>();
        return skr::json_write<T>(writer, *derived);
    };
    ImporterTypeInfo info{ create, loader, store, T::Version };
    registry->RegisterImporter(guid, info);
}