#pragma once
#include "SkrBase/config.h"
#include "SkrBase/math.h"
#include "SkrRT/io/io.h"
#include "SkrBase/atomic/atomic.h"

namespace animd
{

struct animd_model_resource_t
{
};
typedef struct animd_model_resource_t animd_model_resource_t;
typedef struct animd_model_resource_t* animd_model_resource_id;

typedef void (*animd_async_io_callback_t)(struct animd_ram_io_future_t* request, void* data);
typedef struct animd_ram_io_future_t
{
    struct skr_vfs_t* vfs_override;
    skr_io_future_t* future;
    SAtomicU32 io_status;
    animd_model_resource_id model_resource;
    animd_async_io_callback_t callback;
    void* callback_data;

    ESkrIOStage get_status() const SKR_NOEXCEPT
    {
        return (ESkrIOStage)skr_atomic_load_acquire(&io_status);
    }
    bool is_ready() const SKR_NOEXCEPT
    {
        return get_status() == SKR_IO_STAGE_COMPLETED;
    }
} animd_ram_io_future_t;

} // namespace animd