#pragma once
#include "SkrBase/misc/defer.hpp"
#include "SkrOS/filesystem.hpp"
#include "SkrContainers/span.hpp"
#include "SkrContainers/function_ref.hpp"
#include "SkrCore/log.hpp"
#include "SkrCore/blob.hpp"
#include "SkrRT/resource/resource_header.hpp"
#include "SkrToolCore/asset/cooker.hpp"

SKR_DECLARE_TYPE_ID_FWD(skr::io, IRAMService, skr_io_ram_service);
namespace skr::task
{
struct event_t;
} // namespace skr::task

namespace skd::asset
{
struct SAssetRecord {
    SProject*             project;
    skr_guid_t            guid;
    skr_guid_t            type;
    skr_guid_t            cooker;
    skr::filesystem::path path;
    skr::String           meta;
};

struct TOOL_CORE_API SCookContext { // context per job
    friend struct SCookSystem;
    friend struct SCookSystemImpl;

public:
    virtual ~SCookContext()                             = default;
    virtual skr::filesystem::path GetOutputPath() const = 0;

    virtual SImporter*          GetImporter() const        = 0;
    virtual skr_guid_t          GetImporterType() const    = 0;
    virtual uint32_t            GetImporterVersion() const = 0;
    virtual uint32_t            GetCookerVersion() const   = 0;
    virtual const SAssetRecord* GetAssetRecord() const     = 0;
    virtual skr::String         GetAssetPath() const       = 0;

    virtual skr::filesystem::path                  AddSourceFile(const skr::filesystem::path& path)                                                                   = 0;
    virtual skr::filesystem::path                  AddSourceFileAndLoad(skr_io_ram_service_t* ioService, const skr::filesystem::path& path, skr::BlobId& destination) = 0;
    virtual skr::span<const skr::filesystem::path> GetSourceFiles() const                                                                                             = 0;

    virtual void     AddRuntimeDependency(skr_guid_t resource)              = 0;
    virtual void     AddSoftRuntimeDependency(skr_guid_t resource)          = 0;
    virtual uint32_t AddStaticDependency(skr_guid_t resource, bool install) = 0;

    virtual skr::span<const skr_guid_t>            GetRuntimeDependencies() const            = 0;
    virtual skr::span<const skr_resource_handle_t> GetStaticDependencies() const             = 0;
    virtual const skr_resource_handle_t&           GetStaticDependency(uint32_t index) const = 0;

    virtual const skr::task::event_t& GetCounter() = 0;

    template <class T>
    T* Import() { return (T*)_Import(); }

    template <class T>
    void Destroy(T* ptr)
    {
        if (ptr) _Destroy(ptr);
    }

    template <class T>
    bool Save(T& resource)
    {
        //------save resource to disk
        auto outputPath = GetOutputPath().u8string();
        auto file       = fopen((const char*)outputPath.c_str(), "wb");
        if (!file)
        {
            SKR_LOG_FMT_ERROR(u8"[SConfigCooker::Cook] failed to write cooked file for resource {}! path: {}",
                              record->guid, (const char*)record->path.u8string().c_str());
            return false;
        }
        SKR_DEFER({ fclose(file); });
        //------write resource object
        skr::Vector<uint8_t>          buffer;
        skr::archive::BinVectorWriter writer{ &buffer };
        SBinaryWriter                 archive(writer);
        if (!skr::bin_write(&archive, resource))
        {
            SKR_LOG_FMT_ERROR(u8"[SConfigCooker::Cook] failed to serialize resource {}! path: {}",
                              record->guid, (const char*)record->path.u8string().c_str());
            return false;
        }
        if (fwrite(buffer.data(), 1, buffer.size(), file) < buffer.size())
        {
            SKR_LOG_FMT_ERROR(u8"[SConfigCooker::Cook] failed to write cooked file for resource {}! path: {}",
                              record->guid, (const char*)record->path.u8string().c_str());
            return false;
        }
        return true;
    }

protected:
    static SCookContext* Create(skr_io_ram_service_t* service);
    static void          Destroy(SCookContext* ctx);

    virtual void SetCounter(skr::task::event_t&)                  = 0;
    virtual void SetCookerVersion(uint32_t version)               = 0;
    virtual void SetOutputPath(const skr::filesystem::path& path) = 0;

    virtual void* _Import()       = 0;
    virtual void  _Destroy(void*) = 0;

    template <class S>
    void WriteHeader(S& s, SCooker* cooker)
    {
        skr_resource_header_t header;
        header.guid       = record->guid;
        header.type       = record->type;
        header.version    = cooker->Version();
        auto runtime_deps = GetRuntimeDependencies();
        header.dependencies.append(runtime_deps.data(), runtime_deps.size());
        skr::bin_write(&s, header);
    }

    SAssetRecord* record = nullptr;
};

struct TOOL_CORE_API SCookSystem { // system
public:
    SCookSystem() SKR_NOEXCEPT          = default;
    virtual ~SCookSystem() SKR_NOEXCEPT = default;

    virtual void Initialize() {}
    virtual void Shutdown() {}

    virtual skr::task::event_t AddCookTask(skr_guid_t resource)  = 0;
    virtual skr::task::event_t EnsureCooked(skr_guid_t resource) = 0;
    virtual void               WaitForAll()                      = 0;
    virtual bool               AllCompleted() const              = 0;

    virtual void RegisterCooker(bool isDefault, skr_guid_t cooker, skr_guid_t type, SCooker* instance) = 0;
    virtual void UnregisterCooker(skr_guid_t type)                                                     = 0;

    virtual SAssetRecord* GetAssetRecord(skr_guid_t type) const                    = 0;
    virtual SAssetRecord* LoadAssetMeta(SProject* project, const skr::String& uri) = 0;

    virtual void ParallelForEachAsset(uint32_t batch, skr::FunctionRef<void(skr::span<SAssetRecord*>)> f) = 0;

    virtual skr_io_ram_service_t* getIOService() = 0;

    static constexpr uint32_t ioServicesMaxCount = 1;
};
} // namespace skd::asset