#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrBase/atomic/atomic.h"
#include "SkrCore/memory/sp.hpp"
#include "SkrCore/async/thread_job.hpp"
#include "SkrRT/io/ram_io.hpp"
#include "SkrContainers/hashmap.hpp"
#include "SkrRenderer/graphics/shader_map.hpp"
#include "SkrRenderer/graphics/shader_hash.hpp"

#include "SkrProfile/profile.h"

namespace skr
{
struct ShaderMapImpl;

struct ShaderProgress : public skr::AsyncProgress<skr::FutureLauncher<bool>, int, bool>
{
    ShaderProgress(ShaderMapImpl* factory, const char8_t* uri, const PlatformShaderIdentifier& identifier)
        : factory(factory), bytes_uri(uri), identifier(identifier)
    {

    }

    bool do_in_background() override;

    ShaderMapImpl* factory = nullptr;
    skr::String bytes_uri;
    skr_io_future_t data_future;
    skr::BlobId blob = nullptr;
    PlatformShaderIdentifier identifier = {};
};

struct ShaderMapImpl : public ShaderMap
{
    ShaderMapImpl(const ShaderMapRoot& root)
        : root(root)
    {
        future_launcher = SP<skr::FutureLauncher<bool>>::New(root.job_queue);
    }

    ~ShaderMapImpl()
    {
        for (auto iter : mShaderMap)
        {
            if (skr_atomic_load_relaxed(&iter.second->frame) != UINT64_MAX)
            {
                cgpu_free_shader_library(iter.second->shader);
            }
        }
    }

    CGPUShaderLibraryId find_shader(const PlatformShaderIdentifier& id) SKR_NOEXCEPT override;
    EShaderMapShaderStatus install_shader(const PlatformShaderIdentifier& id) SKR_NOEXCEPT override;
    bool free_shader(const PlatformShaderIdentifier& id) SKR_NOEXCEPT override;

    void new_frame(uint64_t frame_index) SKR_NOEXCEPT override;
    void garbage_collect(uint64_t critical_frame) SKR_NOEXCEPT override;

    EShaderMapShaderStatus install_shader_from_vfs(const PlatformShaderIdentifier& id) SKR_NOEXCEPT;

    struct MappedShader
    {
        CGPUShaderLibraryId shader = nullptr;
        SAtomicU64 frame = UINT64_MAX;
        SAtomicU32 rc = 0;
        SAtomicU32 shader_status = 0;
    };

    SP<skr::FutureLauncher<bool>> future_launcher;
    uint64_t frame_index = 0;
    ShaderMapRoot root;

    skr::ParallelFlatHashMap<PlatformShaderIdentifier, SP<MappedShader>, PlatformShaderIdentifier::hasher> mShaderMap;
    skr::ParallelFlatHashMap<PlatformShaderIdentifier, SP<ShaderProgress>, PlatformShaderIdentifier::hasher> mShaderTasks;
};

bool ShaderProgress::do_in_background()
{
    SkrZoneScopedN("CreateShader");

    auto device = factory->root.device;
    auto& shader = factory->mShaderMap[identifier];

    skr_atomic_store_relaxed(&shader->shader_status, EShaderMapShaderStatus::LOADED);

    auto desc = make_zeroed<CGPUShaderLibraryDescriptor>();
    desc.code = (const uint32_t*)blob->get_data();
    desc.code_size = (uint32_t)blob->get_size();
    desc.name = bytes_uri.c_str();
    const auto created_shader = cgpu_create_shader_library(device, &desc);
    blob.reset();
    if (!created_shader)
    {
        skr_atomic_store_relaxed(&shader->shader_status, EShaderMapShaderStatus::FAILED);
        skr_atomic_store_relaxed(&factory->mShaderMap[identifier]->frame, UINT64_MAX);
        return false;
    }
    else
    {
        shader->shader = created_shader;
        skr_atomic_store_relaxed(&shader->shader_status, EShaderMapShaderStatus::INSTALLED);
        skr_atomic_store_relaxed(&factory->mShaderMap[identifier]->frame, factory->frame_index);
    }
    return true;
}

CGPUShaderLibraryId ShaderMapImpl::find_shader(const PlatformShaderIdentifier& identifier) SKR_NOEXCEPT
{
    auto found = mShaderMap.find(identifier);
    if (found != mShaderMap.end())
    {
        if (skr_atomic_load_relaxed(&found->second->frame) != UINT64_MAX)
        {
            return found->second->shader;
        }
    }
    return nullptr;
}

bool ShaderMapImpl::free_shader(const PlatformShaderIdentifier& identifier) SKR_NOEXCEPT
{
    auto found = mShaderMap.find(identifier);
    if (found != mShaderMap.end())
    {
    #ifdef _DEBUG
        const auto frame = skr_atomic_load_relaxed(&found->second->frame);
        SKR_ASSERT(frame != UINT64_MAX && "this shader is freed but never installed, check your code for errors!");
    #endif
        skr_atomic_fetch_add_relaxed(&found->second->rc, -1);
        return true;
    }
    return false;
}

EShaderMapShaderStatus ShaderMapImpl::install_shader(const PlatformShaderIdentifier& identifier) SKR_NOEXCEPT
{
    auto found = mShaderMap.find(identifier);
    // 1. found mapped shader
    if (found != mShaderMap.end())
    {
        // 1.1 found alive shader request
        auto status = skr_atomic_load_relaxed(&found->second->shader_status);

        // 1.2 request is done, add rc & record frame index
        skr_atomic_fetch_add_relaxed(&found->second->rc, 1);
        skr_atomic_store_relaxed(&found->second->frame, frame_index);
        return (EShaderMapShaderStatus)status;
    }
    // 2. not found mapped shader
    else
    {
        // fire request
        auto mapped_shader = SP<MappedShader>::New();
        skr_atomic_fetch_add_relaxed(&mapped_shader->rc, 1);
        // keep mapped_shader::frame at UINT64_MAX until shader is loaded
        mShaderMap.emplace(identifier, mapped_shader);
        return install_shader_from_vfs(identifier);
    }
}

EShaderMapShaderStatus ShaderMapImpl::install_shader_from_vfs(const PlatformShaderIdentifier& identifier) SKR_NOEXCEPT
{
    bool launch_success = false;
    auto bytes_vfs = root.bytecode_vfs;
    SKR_ASSERT(bytes_vfs);
    const auto hash = identifier.hash;
    const auto uri = skr::format(u8"{}#{}-{}-{}-{}.bytes", hash.flags, 
        hash.encoded_digits[0], hash.encoded_digits[1], hash.encoded_digits[2], hash.encoded_digits[3]);
    auto sRequest = SP<ShaderProgress>::New(this, uri.c_str(), identifier);
    auto found = mShaderTasks.find(identifier);
    SKR_ASSERT(found == mShaderTasks.end());
    mShaderTasks.emplace(identifier, sRequest);
    
    auto service = root.ram_service;
    auto request = service->open_request();
    request->set_vfs(bytes_vfs);
    request->set_path(sRequest->bytes_uri.c_str());
    request->add_block({}); // read all
    request->add_callback(SKR_IO_STAGE_COMPLETED, +[](skr_io_future_t* future, skr_io_request_t* request, void* data) noexcept {
        SkrZoneScopedN("CreateShaderFromBytes");
        
        auto progress = (ShaderProgress*)data;
        auto factory = progress->factory;
        if (auto launcher = factory->future_launcher.get()) // create shaders on aux thread
        {
            progress->execute(*launcher);
        }
        else // create shaders inplace
        {
            SKR_ASSERT("launcher is unexpected null!");
        }
    }, (void*)sRequest.get());
    sRequest->blob = root.ram_service->request(request, &sRequest->data_future);
    launch_success = true;
    return launch_success ? EShaderMapShaderStatus::REQUESTED : EShaderMapShaderStatus::FAILED;
}

void ShaderMapImpl::new_frame(uint64_t index) SKR_NOEXCEPT
{
    SKR_ASSERT(index > frame_index);

    frame_index = index;
}

void ShaderMapImpl::garbage_collect(uint64_t critical_frame) SKR_NOEXCEPT
{
    // erase request when exit this scope
    // SKR_DEFER({factory->mShaderTasks.erase(identifier);});
    for (auto it = mShaderMap.begin(); it != mShaderMap.end();)
    {
        auto status = skr_atomic_load_relaxed(&it->second->shader_status);
        if (status == EShaderMapShaderStatus::INSTALLED)
        {
            mShaderTasks.erase(it->first);
        }

        if (skr_atomic_load_relaxed(&it->second->rc) == 0 && 
            skr_atomic_load_relaxed(&it->second->frame) < critical_frame)
        {
            if (mShaderTasks.find(it->first) != mShaderTasks.end())
            {
                // shader is still loading, skip & wait for it to finish
                // TODO: cancel shader loading
                ++it;
            }
            else
            {
                it = mShaderMap.erase(it);
            }
        }
        else
        {
            ++it;
        }
    }
}

ShaderMap* ShaderMap::Create(const struct ShaderMapRoot* desc) SKR_NOEXCEPT
{
    return SkrNew<skr::ShaderMapImpl>(*desc);
}

bool ShaderMap::Free(ShaderMap* shader_map) SKR_NOEXCEPT
{
    SkrDelete(shader_map);
    return true;
}

} // namespace skr

using namespace skr;

ShaderMap* skr_shader_map_create(const struct ShaderMapRoot* desc)
{
    return ShaderMap::Create(desc);
}

EShaderMapShaderStatus skr_shader_map_install_shader(ShaderMap* shaderMap, const PlatformShaderIdentifier* key)
{
    return shaderMap->install_shader(*key);
}

CGPUShaderLibraryId skr_shader_map_find_shader(ShaderMap* shaderMap, const PlatformShaderIdentifier* id)
{
    return shaderMap->find_shader(*id);
}

void skr_shader_map_free_shader(ShaderMap* shaderMap, const PlatformShaderIdentifier* id)
{
    shaderMap->free_shader(*id);
}

void skr_shader_map_new_frame(ShaderMap* shaderMap, uint64_t frame_index)
{
    shaderMap->new_frame(frame_index);
}

void skr_shader_map_garbage_collect(ShaderMap* shaderMap, uint64_t critical_frame)
{
    shaderMap->garbage_collect(critical_frame);
}

void skr_shader_map_free(ShaderMap* id)
{
    ShaderMap::Free(id);
}