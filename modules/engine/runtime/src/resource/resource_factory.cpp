#include "SkrRT/resource/resource_factory.h"
#include "SkrRT/resource/resource_header.hpp"
#include "SkrBase/misc/debug.h"
#include "SkrRTTR/type_registry.hpp"
#include "SkrRTTR/type.hpp"
#include "SkrCore/log.hpp"
#include "SkrRTTR/export/extern_methods.hpp"

namespace skr
{
namespace resource
{

bool ResourceFactory::Deserialize(SResourceRecord* record, SBinaryReader* reader)
{
    if (auto type = skr::get_type_from_guid(record->header.type))
    {
        auto p_obj = sakura_malloc_aligned(type->size(), type->alignment());
        // find & call ctor
        {
            auto ctor_data = type->find_default_ctor();
            ctor_data.invoke(p_obj);
        }
        {
            using ReadBinProc = bool(void* o, void* r);
            auto read_bin_data = type->find_extern_method_t<ReadBinProc>(
                skr::SkrCoreExternMethods::ReadBin,
                ETypeSignatureCompareFlag::Strict
            );
            if (!read_bin_data.invoke(p_obj, reader))
            {
                // TODO: CALL DTOR IF FAILED
                SKR_UNIMPLEMENTED_FUNCTION();
                sakura_free_aligned(p_obj, type->alignment());
                p_obj = nullptr;
            }
        }
        record->resource = p_obj;
        return true;
    }
    SKR_LOG_FMT_ERROR(u8"Failed to deserialize resource of type {}", record->header.type);
    SKR_UNREACHABLE_CODE();
    return false;
}

bool ResourceFactory::Unload(SResourceRecord* record)
{
    record->header.dependencies.clear();
    if (record->destructor)
        record->destructor(record->resource);
#ifdef SKR_RESOURCE_DEV_MODE
    if (record->artifactsDestructor)
        record->artifactsDestructor(record->artifacts);
#endif
    return true;
}

ESkrInstallStatus ResourceFactory::UpdateInstall(SResourceRecord* record)
{
    SKR_UNREACHABLE_CODE();
    return SKR_INSTALL_STATUS_SUCCEED;
}
} // namespace resource
} // namespace skr