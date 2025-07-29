#include "SkrProfile/profile.h"
#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrBase/misc/defer.hpp"
#include "SkrRTTR/type.hpp"
#include "SkrTask/parallel_for.hpp"
#include "SkrContainers/string.hpp"
#include "SkrCore/module/module.hpp"
#include "SkrCore/async/thread_job.hpp"
#include "SkrCore/platform/vfs.h"
#include "SkrSerde/bin_serde.hpp"
#include "SkrSerde/json_serde.hpp"
#include "SkrRT/io/ram_io.hpp"
#include "SkrToolCore/cook_system/cook_system.hpp"
#include "SkrToolCore/cook_system/importer.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrContainers/hashmap.hpp"

namespace skd::asset
{
struct CookSystemImpl : public skd::asset::CookSystem
{
    friend struct ::SkrToolCoreModule;
    using AssetMap = skr::ParallelFlatHashMap<skr_guid_t, skr::RC<AssetMetaFile>, skr::Hash<skr_guid_t>>;
    using CookingMap = skr::ParallelFlatHashMap<skr_guid_t, CookContext*, skr::Hash<skr_guid_t>>;

    ~CookSystemImpl()
    {
        for (auto& [k, asset] : assets)
            asset.reset();
        assets.clear();
    }

    Cooker* GetCooker(AssetMetaFile* info) const;
    skr::task::event_t AddCookTask(skr_guid_t resource) override;
    skr::task::event_t EnsureCooked(skr_guid_t resource) override;

    void WaitForAll() override;
    bool AllCompleted() const override;

    void RegisterCooker(bool isDefault, skr_guid_t cooker, skr_guid_t type, Cooker* instance) override;
    void UnregisterCooker(skr_guid_t type) override;

    skr::RC<AssetMetaFile> LoadAssetMeta(SProject* project, const skr::String& uri) override;
    skr::RC<AssetMetaFile> GetAssetMetaFile(skr_guid_t guid) const override;

    skr::io::IRAMService* GetIOService() override;

    void ParallelForEachAsset(uint32_t batch, skr::FunctionRef<void(skr::span<skr::RC<AssetMetaFile>>)> f) override;

protected:
    template <class F, class Iter>
    void ParallelFor(Iter begin, Iter end, size_t batch, F f)
    {
        skr::parallel_for(std::move(begin), std::move(end), batch, std::move(f));
    }

    AssetMap assets;
    CookingMap cooking;
    SMutex ioMutex;

    skr::task::counter_t mainCounter;

    skr::FlatHashMap<skr_guid_t, Cooker*, skr::Hash<skr_guid_t>> defaultCookers;
    skr::FlatHashMap<skr_guid_t, Cooker*, skr::Hash<skr_guid_t>> cookers;
    skr::io::IRAMService* ioServices[ioServicesMaxCount];
};
} // namespace skd::asset

namespace skd::asset
{
CookSystem* GetCookSystem()
{
    static skd::asset::CookSystemImpl cook_system;
    return &cook_system;
}

void RegisterCookerToSystem(CookSystem* system, bool isDefault, skr_guid_t cooker, skr_guid_t type, Cooker* instance)
{
    system->RegisterCooker(isDefault, cooker, type, instance);
}

AssetMetaFile::AssetMetaFile(skr::StringView _uri, skr::String&& _meta,
                             const skr::GUID& _guid, const skr::GUID& _type, const skr::GUID& _cooker)
    : uri(_uri), meta(std::move(_meta)), guid(_guid), type(_type), cooker(_cooker)
{

}

void CookSystemImpl::WaitForAll()
{
    mainCounter.wait(true);
}

bool CookSystemImpl::AllCompleted() const
{
    return mainCounter.test();
}

skr::io::IRAMService* CookSystemImpl::GetIOService()
{
    SMutexLock lock(ioMutex);
    static std::atomic_uint32_t cursor = 0;
    cursor = (cursor % ioServicesMaxCount);
    return ioServices[cursor++];
}

skr::task::event_t CookSystemImpl::AddCookTask(skr_guid_t guid)
{
    CookContext* cookContext = nullptr;
    auto asset = GetAssetMetaFile(guid);
    // return existed task if it's already cooking
    {
        skr::task::event_t existed_task{ nullptr };
        cooking.lazy_emplace_l(guid, [&](const auto& ctx_kv) { existed_task = ctx_kv.second->GetCounter(); }, [&](const CookingMap::constructor& ctor) {
            cookContext = CookContext::Create(asset, GetIOService());
            ctor(guid, cookContext); });
        if (existed_task)
            return existed_task;
    }
    skr::task::event_t counter;
    cookContext->SetCounter(counter);
    auto guidName = skr::format(u8"Fiber{}", asset->guid);
    mainCounter.add(1);
    skr::task::schedule([cookContext]() {
        auto system = static_cast<CookSystemImpl*>(GetCookSystem());
        const auto metaAsset = cookContext->GetAssetMetaFile();
        auto cooker = system->GetCooker(metaAsset.get());
        SKR_ASSERT(cooker);

        SkrZoneScopedN("CookingTask");
        {
            const auto rtti_type = skr::get_type_from_guid(metaAsset->type);
            const auto cookerTypeName = rtti_type ? rtti_type->name().c_str_raw() : (const char*)u8"UnknownResource";
            const auto guidString = skr::format(u8"Guid: {}", metaAsset->guid);
            const auto assetTypeGuidString = skr::format(u8"TypeGuid: {}", metaAsset->type);
            const auto scopeName = skr::format(u8"Cook.[{}]", (const skr_char8*)cookerTypeName);
            const auto assetString = skr::format(u8"Asset: {}", metaAsset->uri.c_str());
            ZoneName(scopeName.c_str_raw(), scopeName.size());
            SkrMessage(guidString.c_str_raw(), guidString.size());
            SkrMessage(assetTypeGuidString.c_str_raw(), assetTypeGuidString.size());
            SkrMessage(assetString.c_str_raw(), assetString.size());
        }

        SKR_DEFER({
            auto system = static_cast<CookSystemImpl*>(GetCookSystem());
            auto guid = cookContext->GetAssetMetaFile()->guid;
            system->cooking.erase_if(guid, [](const auto& ctx_kv) { CookContext::Destroy(ctx_kv.second); return true; });
            system->mainCounter.decrement();
        });

        // setup cook context
        cookContext->SetCookerVersion(cooker->Version());
        // SKR_ASSERT(iter != system->cookers.end()); // TODO: error handling
        SKR_LOG_INFO(u8"[CookTask] resource %s cook started!", metaAsset->uri.c_str());
        if (cooker->Cook(cookContext))
        {
            // write resource header
            {
                SKR_LOG_INFO(u8"[CookTask] resource %s cook finished! updating resource metas.", metaAsset->uri.c_str());
                skr::Vector<uint8_t> buffer;
                skr::archive::BinVectorWriter writer{ &buffer };
                SBinaryWriter archive(writer);
                cookContext->WriteHeader(archive, cooker);
                
                auto resource_vfs = metaAsset->project->GetResourceVFS();
                auto relative_path = skr::format(u8"{}.rh", metaAsset->guid);
                auto file = skr_vfs_fopen(resource_vfs, relative_path.u8_str(), 
                                          SKR_FM_WRITE_BINARY, SKR_FILE_CREATION_ALWAYS_NEW);
                if (!file)
                {
                    SKR_LOG_ERROR(u8"[CookTask] failed to write header file for resource %s!", metaAsset->uri.c_str());
                    return;
                }
                SKR_DEFER({ skr_vfs_fclose(file); });
                skr_vfs_fwrite(file, buffer.data(), 0, buffer.size());
            }
            // write resource dependencies
            {
                SKR_LOG_INFO(u8"[CookTask] resource %s cook finished! updating dependencies.", metaAsset->uri.c_str());
                // write dependencies
                skr::archive::JsonWriter writer(2);
                writer.StartObject();
                writer.Key(u8"importerVersion");
                writer.UInt64(cookContext->GetImporterVersion());
                writer.Key(u8"cookerVersion");
                writer.UInt64(cookContext->GetCookerVersion());
                writer.Key(u8"files");
                writer.StartArray();
                for (auto& source_path : cookContext->GetSourceFiles())
                {
                    skr::json_write<skr::String>(&writer, source_path);
                }
                writer.EndArray();
                writer.Key(u8"dependencies");
                writer.StartArray();
                for (auto& dep : cookContext->GetStaticDependencies())
                    skr::json_write<SResourceHandle>(&writer, dep);
                writer.EndArray();
                writer.EndObject();
                
                auto dependency_vfs = metaAsset->project->GetDependencyVFS();
                auto relative_path = skr::format(u8"{}.d", metaAsset->guid);
                auto file = skr_vfs_fopen(dependency_vfs, relative_path.u8_str(), 
                                          SKR_FM_WRITE, SKR_FILE_CREATION_ALWAYS_NEW);
                if (!file)
                {
                    SKR_LOG_ERROR(u8"[CookTask] failed to write dependency file for resource %s!", metaAsset->uri.c_str());
                    return;
                }
                SKR_DEFER({ skr_vfs_fclose(file); });
                auto jString = writer.Write();
                skr_vfs_fwrite(file, jString.c_str_raw(), 0, jString.length_buffer());
            }
        }
    },
        &counter,
        guidName.c_str_raw());
    return counter;
}

void CookSystemImpl::RegisterCooker(bool isDefault, skr_guid_t cooker, skr_guid_t type, Cooker* instance)
{
    SKR_ASSERT(instance->system == nullptr);
    instance->system = this;
    cookers.insert(std::make_pair(cooker, instance));
    if (isDefault)
    {
        auto result = defaultCookers.insert(std::make_pair(type, instance));
        SKR_ASSERT(result.second);
        (void)result;
    }
}

void CookSystemImpl::UnregisterCooker(skr_guid_t guid)
{
    cookers.erase(guid);
}

Cooker* CookSystemImpl::GetCooker(AssetMetaFile* info) const
{
    if (info->cooker == skr_guid_t{})
    {
        auto it = defaultCookers.find(info->type);
        if (it != defaultCookers.end())
            return it->second;
        return nullptr;
    }
    auto it = cookers.find(info->cooker);
    if (it != cookers.end()) return it->second;
    return nullptr;
}

#define SKR_CHECK_RESULT(result, name)                                                                                                      \
    if (result.error() != simdjson::SUCCESS)                                                                                                \
    {                                                                                                                                       \
        SKR_LOG_INFO(u8"[CookSystemImpl::EnsureCooked] " name " file parse failed! resource guid: %s", metaAsset->uri.c_str()); \
        return false;                                                                                                                       \
    }

skr::task::event_t CookSystemImpl::EnsureCooked(skr_guid_t guid)
{
    SkrZoneScoped;
    {
        skr::task::event_t result{ nullptr };
        cooking.if_contains(guid, [&](const auto& ctx_kv) {
            result = ctx_kv.second->GetCounter();
        });
        if (result)
            return result;
    }
    auto metaAsset = GetAssetMetaFile(guid);
    if (!metaAsset)
    {
        SKR_LOG_ERROR(u8"[CookSystemImpl::EnsureCooked] resource not exist! asset path: %s", metaAsset->uri.c_str());
        return nullptr;
    }
    return AddCookTask(guid);
}

skr::RC<AssetMetaFile> CookSystemImpl::LoadAssetMeta(SProject* project, const skr::String& uri)
{
    SkrZoneScoped;
    std::error_code ec = {};
    skr::String meta_content;
    if (project->LoadAssetMeta(uri.view(), meta_content))
    {
        // Parse guid and type first
        skr::GUID guid, type, cooker;
        {
            skr::archive::JsonReader reader(meta_content.view());
            reader.StartObject();
            reader.Key(u8"guid");
            skr::json_read(&reader, guid);
            reader.Key(u8"type");
            skr::json_read(&reader, type);
            // Try to read cooker if it exists
            try {
                reader.Key(u8"cooker");
                skr::json_read(&reader, cooker);
            } catch (...) {
                // cooker is optional, ignore if not present
            }
            reader.EndObject();
        }
        
        // Create record with proper constructor
        auto record = skr::RC<AssetMetaFile>::New(uri, std::move(meta_content), guid, type, cooker);
        const_cast<SProject*&>(record->project) = project;
        assets.insert(std::make_pair(record->guid, record));
        return record;
    }
    SKR_ASSERT(false);
    return nullptr;
}

skr::RC<AssetMetaFile> CookSystemImpl::GetAssetMetaFile(skr_guid_t guid) const 
{
    auto it = assets.find(guid);
    if (it != assets.end()) 
        return it->second;
    return nullptr;
}

void CookSystemImpl::ParallelForEachAsset(uint32_t batch, skr::FunctionRef<void(skr::span<skr::RC<AssetMetaFile>>)> f) 
{
    ParallelFor(assets.begin(), assets.end(), batch, [f, batch](auto begin, auto end) {
        skr::Vector<skr::RC<AssetMetaFile>> records;
        records.reserve(batch);
        for (auto it = begin; it != end; ++it)
        {
            records.add(it->second);
        }
        f(records);
    });
}

} // namespace skd::asset

struct TOOL_CORE_API SkrToolCoreModule : public skr::IDynamicModule
{
    skr::JobQueue* io_job_queue = nullptr;
    skr::JobQueue* io_callback_job_queue = nullptr;
    virtual void on_load(int argc, char8_t** argv) override
    {
        auto cook_system = (skd::asset::CookSystemImpl*)skd::asset::GetCookSystem();
        skr_init_mutex(&cook_system->ioMutex);

        auto jqDesc = make_zeroed<skr::JobQueueDesc>();
        jqDesc.thread_count = 1;
        jqDesc.priority = SKR_THREAD_ABOVE_NORMAL;
        jqDesc.name = u8"Tool-IOJobQueue";
        io_job_queue = SkrNew<skr::JobQueue>(jqDesc);

        jqDesc.name = u8"Tool-IOCallbackJobQueue";
        io_callback_job_queue = SkrNew<skr::JobQueue>(jqDesc);

        for (auto& ioService : cook_system->ioServices)
        {
            // all used up
            if (ioService == nullptr)
            {
                skr_ram_io_service_desc_t desc = {};
                desc.sleep_time = SKR_ASYNC_SERVICE_SLEEP_TIME_MAX;
                desc.awake_at_request = true;
                desc.name = u8"Tool-IOService";
                desc.io_job_queue = io_job_queue;
                desc.callback_job_queue = io_callback_job_queue;
                ioService = skr_io_ram_service_t::create(&desc);
                ioService->run();
            }
        }
    }

    virtual void on_unload() override
    {
        auto cook_system = (skd::asset::CookSystemImpl*)skd::asset::GetCookSystem();
        skr_destroy_mutex(&cook_system->ioMutex);
        for (auto ioService : cook_system->ioServices)
        {
            if (ioService)
                skr_io_ram_service_t::destroy(ioService);
        }

        SkrDelete(io_callback_job_queue);
        SkrDelete(io_job_queue);
    }
};
IMPLEMENT_DYNAMIC_MODULE(SkrToolCoreModule, SkrToolCore);