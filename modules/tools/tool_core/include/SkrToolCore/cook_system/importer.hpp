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
    SKR_GENERATE_BODY();
    static constexpr uint32_t kDevelopmentVersion = UINT32_MAX;
    static uint32_t Version() { return kDevelopmentVersion; }

    virtual ~Importer() = default;
    virtual void* Import(skr::io::IRAMService*, CookContext* context) = 0;
    virtual void Destroy(void*) = 0;
    inline skr::GUID GetType() const { return importer_type; }

    skr::GUID importer_type;
    SKR_RC_IMPL();
};

struct TOOL_CORE_API ImporterTypeInfo
{
    skr::RC<Importer> (*Load)(skr::archive::JsonReader* reader);
    void (*Store)(skr::archive::JsonWriter* writer, skr::RC<Importer> object);
    uint32_t (*Version)();
};

struct ImporterFactory
{
    virtual skr::RC<Importer> LoadImporter(skr::archive::JsonReader* importer) = 0;
    virtual void StoreImporter(skr::archive::JsonWriter* writer, skr::RC<Importer> importer) = 0;
    virtual uint32_t GetImporterVersion(skr::GUID type) = 0;
    virtual void RegisterImporter(skr::GUID type, ImporterTypeInfo info) = 0;
};

TOOL_CORE_API ImporterFactory* GetImporterFactory();
} // namespace skd::asset

template <class T>
void skd::asset::RegisterImporter(skr::GUID guid)
{
    auto registry = GetImporterFactory();
    auto loader = +[](skr::archive::JsonReader* object) -> skr::RC<Importer> {
        auto importer = skr::RC<T>::New();
        skr::json_read(object, *importer);
        return importer;
    };
    auto store = +[](skr::archive::JsonWriter* writer, skr::RC<Importer> p) {
        auto _this = (T*)p.get();
        skr::json_write(writer, *_this);
    };
    ImporterTypeInfo info{ loader, store, T::Version };
    registry->RegisterImporter(guid, info);
}