#pragma once
#include "SkrCore/log.hpp"
#include "SkrCore/blob.hpp"
#include "SkrCore/platform/vfs.h"
#include "SkrContainers/vector.hpp"
#include "SkrRT/resource/resource_header.hpp"
#include "SkrToolCore/cook_system/cooker.hpp"
#include "SkrToolCore/project/project.hpp"

SKR_DECLARE_TYPE_ID_FWD(skr::io, IRAMService, skr_io_ram_service);

namespace skd::asset
{
using URI = skr::String;
struct AssetMetaFile
{
    SKR_RC_IMPL();
public:
    AssetMetaFile(skr::StringView uri, skr::String&& meta, 
                  const skr::GUID& guid, const skr::GUID& type, const skr::GUID& cooker);

    template <class T>
    T GetAssetMetadata()
    {
        // TODO: now it parses twice, add cursor to reader to avoid this
        skr::archive::JsonReader reader(meta.view());
        T settings;
        skr::json_read(&reader, settings);
        return settings;
    }

    const SProject* project = nullptr;
    const skr::GUID guid;
    const skr::GUID type;
    const skr::GUID cooker;
    const URI uri;
    const skr::String meta;
};

struct TOOL_CORE_API CookContext
{
    friend struct CookSystem;
    friend struct CookSystemImpl;

public:
    virtual ~CookContext() = default;
    virtual Importer* GetImporter() const = 0;
    virtual skr::GUID GetImporterType() const = 0;
    virtual uint32_t GetImporterVersion() const = 0;
    virtual uint32_t GetCookerVersion() const = 0;
    virtual skr::RC<AssetMetaFile> GetAssetMetaFile() = 0;
    virtual skr::RC<const AssetMetaFile> GetAssetMetaFile() const = 0;
    virtual skr::String GetAssetPath() const = 0;

    virtual URI AddSourceFile(const URI& path) = 0;
    virtual URI AddSourceFileAndLoad(skr::io::IRAMService* ioService, const URI& path, skr::BlobId& destination) = 0;
    virtual skr::span<const URI> GetSourceFiles() const = 0;

    virtual void AddRuntimeDependency(skr::GUID resource) = 0;
    virtual void AddSoftRuntimeDependency(skr::GUID resource) = 0;
    virtual uint32_t AddStaticDependency(skr::GUID resource, bool install) = 0;

    virtual skr::span<const skr::GUID> GetRuntimeDependencies() const = 0;
    virtual skr::span<const SResourceHandle> GetStaticDependencies() const = 0;
    virtual const SResourceHandle& GetStaticDependency(uint32_t index) const = 0;

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
        auto record = GetAssetMetaFile();
        //------save resource to disk
        auto resource_vfs = record->project->GetResourceVFS();
        auto filename = skr::format(u8"{}.bin", record->guid);
        auto file = skr_vfs_fopen(resource_vfs, filename.u8_str(),
                                  SKR_FM_WRITE_BINARY, SKR_FILE_CREATION_ALWAYS_NEW);
        if (!file)
        {
            SKR_LOG_FMT_ERROR(u8"[ConfigCooker::Cook] failed to write cooked file for resource {}! path: {}",
                record->guid,
                (const char*)record->uri.c_str());
            return false;
        }
        SKR_DEFER({ skr_vfs_fclose(file); });
        //------write resource object
        skr::Vector<uint8_t> buffer;
        skr::archive::BinVectorWriter writer{ &buffer };
        SBinaryWriter archive(writer);
        if (!skr::bin_write(&archive, resource))
        {
            SKR_LOG_FMT_ERROR(u8"[ConfigCooker::Cook] failed to serialize resource {}! path: {}",
                record->guid,
                (const char*)record->uri.c_str());
            return false;
        }
        auto write_size = skr_vfs_fwrite(file, buffer.data(), 0, buffer.size());
        if (write_size != buffer.size())
        {
            SKR_LOG_FMT_ERROR(u8"[ConfigCooker::Cook] failed to write cooked file for resource {}! path: {}",
                record->guid,
                (const char*)record->uri.c_str());
            return false;
        }
        return true;
    }

    bool SaveExtra(skr::span<const uint8_t> data, const char8_t* filename)
    {
        auto record = GetAssetMetaFile();
        //------save extra file to disk
        auto resource_vfs = record->project->GetResourceVFS();
        auto file = skr_vfs_fopen(resource_vfs, filename,
                                  SKR_FM_WRITE_BINARY, SKR_FILE_CREATION_ALWAYS_NEW);
        if (!file)
        {
            SKR_LOG_FMT_ERROR(u8"[CookContext::SaveExtra] failed to create file: {}", filename);
            return false;
        }
        SKR_DEFER({ skr_vfs_fclose(file); });
        
        //------write data
        if (skr_vfs_fwrite(file, data.data(), 0, data.size()) != data.size())
        {
            SKR_LOG_FMT_ERROR(u8"[CookContext::SaveExtra] failed to write data to file: {}", filename);
            return false;
        }
        return true;
    }

protected:
    static CookContext* Create(skr::RC<AssetMetaFile>, skr::RC<Importer> importer, skr::GUID importerType);
    static void Destroy(CookContext* ctx);

    virtual void SetCounter(skr::task::event_t&) = 0;
    virtual void SetIOService(skr::io::IRAMService*) = 0;
    virtual void SetCookerVersion(uint32_t version) = 0;

    virtual void* _Import() = 0;
    virtual void _Destroy(void*) = 0;

    template <class S>
    void WriteHeader(S& s, Cooker* cooker)
    {
        auto record = GetAssetMetaFile();
        SResourceHeader header;
        header.guid = record->guid;
        header.type = record->type;
        header.version = cooker->Version();
        const auto runtime_deps = GetRuntimeDependencies();
        header.dependencies.append(runtime_deps.data(), runtime_deps.size());
        skr::bin_write(&s, header);
    }
};

struct TOOL_CORE_API CookSystem
{ // system
public:
    CookSystem() SKR_NOEXCEPT = default;
    virtual ~CookSystem() SKR_NOEXCEPT = default;

    virtual void Initialize() {}
    virtual void Shutdown() {}

    virtual skr::task::event_t AddCookTask(skr_guid_t resource) = 0;
    virtual skr::task::event_t EnsureCooked(skr_guid_t resource) = 0;
    virtual void WaitForAll() = 0;
    virtual bool AllCompleted() const = 0;

    virtual void RegisterCooker(bool isDefault, skr_guid_t cooker, skr_guid_t type, Cooker* instance) = 0;
    virtual void UnregisterCooker(skr_guid_t type) = 0;

    virtual skr::RC<AssetMetaFile> LoadAssetMeta(SProject* project, const skr::String& uri) = 0;
    virtual skr::RC<AssetMetaFile> GetAssetMetaFile(skr_guid_t type) const = 0;

    virtual void ParallelForEachAsset(uint32_t batch, skr::FunctionRef<void(skr::span<skr::RC<AssetMetaFile>>)> f) = 0;

    virtual skr::io::IRAMService* GetIOService() = 0;

    static constexpr uint32_t ioServicesMaxCount = 1;
};
} // namespace skd::asset