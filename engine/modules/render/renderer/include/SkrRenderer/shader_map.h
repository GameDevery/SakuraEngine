#pragma once
#include "SkrBase/config.h"
#include "SkrRenderer/fwd_types.h"
#include "SkrGraphics/api.h"

struct skr_vfs_t;
SKR_DECLARE_TYPE_ID_FWD(skr, JobQueue, skr_job_queue)

namespace skr {
    
typedef enum EShaderMapShaderStatus
{
    NONE,
    REQUESTED,
    LOADED,
    FAILED,
    INSTALLED,
    UNINSTALLED,
    COUNT,
    MAX_ENUM_BIT = 0x7FFFFFFF
} EShaderMapShaderStatus;

struct ShaderMapRoot {
    skr_vfs_t* bytecode_vfs SKR_IF_CPP(= nullptr);
    skr_io_ram_service_t* ram_service SKR_IF_CPP(= nullptr);
    CGPUDeviceId device SKR_IF_CPP(= nullptr);
    skr_job_queue_id job_queue SKR_IF_CPP(= nullptr);
};

// {guid} -> shader collection 
//              -> {stable_hash} -> multi_shader 
//                                      -> {stable_hash} -> shader_identifier (final)
struct SKR_RENDERER_API ShaderMap
{
    virtual ~ShaderMap() = default;

    virtual EShaderMapShaderStatus install_shader(const PlatformShaderIdentifier& id) SKR_NOEXCEPT = 0;
    virtual CGPUShaderLibraryId find_shader(const PlatformShaderIdentifier& id) SKR_NOEXCEPT = 0;
    virtual bool free_shader(const PlatformShaderIdentifier& id) SKR_NOEXCEPT = 0;

    virtual void new_frame(uint64_t frame_index) SKR_NOEXCEPT = 0;
    virtual void garbage_collect(uint64_t critical_frame) SKR_NOEXCEPT = 0;

    static ShaderMap* Create(const struct ShaderMapRoot* desc) SKR_NOEXCEPT;
    static bool Free(ShaderMap* shader_map) SKR_NOEXCEPT;
};

} // namespace skr