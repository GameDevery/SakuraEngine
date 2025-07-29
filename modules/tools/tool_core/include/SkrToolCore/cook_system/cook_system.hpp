#pragma once
#include "SkrOS/filesystem.hpp"
#include "SkrCore/log.hpp"
#include "SkrCore/blob.hpp"
#include "SkrContainers/vector.hpp"
#include "SkrRT/resource/resource_header.hpp"
#include "SkrToolCore/cook_system/cooker.hpp"

SKR_DECLARE_TYPE_ID_FWD(skr::io, IRAMService, skr_io_ram_service);

namespace skd::asset
{
using URI = skr::String;

struct AssetMetaFile
{
    SKR_RC_IMPL();
public:
    AssetMetaFile(skr::StringView uri, skr::String&& meta);

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
    virtual skr::filesystem::path GetOutputPath() const = 0;

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
        auto outputPath = GetOutputPath().u8string();
        auto file = fopen((const char*)outputPath.c_str(), "wb");
        if (!file)
        {
            SKR_LOG_FMT_ERROR(u8"[ConfigCooker::Cook] failed to write cooked file for resource {}! path: {}",
                record->guid,
                (const char*)record->uri.c_str());
            return false;
        }
        SKR_DEFER({ fclose(file); });
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
        if (fwrite(buffer.data(), 1, buffer.size(), file) < buffer.size())
        {
            SKR_LOG_FMT_ERROR(u8"[ConfigCooker::Cook] failed to write cooked file for resource {}! path: {}",
                record->guid,
                (const char*)record->uri.c_str());
            return false;
        }
        return true;
    }

protected:
    static CookContext* Create(skr::RC<AssetMetaFile> record, skr_io_ram_service_t* service);
    static void Destroy(CookContext* ctx);

    virtual void SetCounter(skr::task::event_t&) = 0;
    virtual void SetCookerVersion(uint32_t version) = 0;
    virtual void SetOutputPath(const skr::filesystem::path& path) = 0;

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