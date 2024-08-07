#pragma once
#include "SkrBase/config.h"
#include "SkrBase/types.h"
#include "SkrRT/config.h"

struct skr_vfs_t;
struct SkrToolCoreModule;
SKR_DECLARE_TYPE_ID_FWD(skr::io, IRAMService, skr_io_ram_service);
SKR_DECLARE_TYPE_ID_FWD(skr::io, IVRAMService, skr_io_vram_service);

namespace skd
{
struct SProject;
namespace asset
{
struct SImporter;
struct SAssetRecord;
struct SCookSystem;
struct SCooker;
struct SCookContext;
} // namespace asset
} // namespace skd