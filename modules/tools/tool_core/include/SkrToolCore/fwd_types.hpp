#pragma once
#include <SkrBase/config.h>

struct skr_vfs_t;
struct SkrToolCoreModule;
SKR_DECLARE_TYPE_ID_FWD(skr::io, IRAMService, skr_io_ram_service);
SKR_DECLARE_TYPE_ID_FWD(skr::io, IVRAMService, skr_io_vram_service);

namespace skr::task
{
    struct event_t;
}

namespace skd
{
struct SProject;
namespace asset
{
struct SImporter;
struct AssetRecord;
struct CookSystem;
struct Cooker;
struct CookContext;
} // namespace asset
} // namespace skd