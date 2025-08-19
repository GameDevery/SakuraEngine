#include "SkrRT/resource/config_resource.h"
#include "SkrBase/misc/debug.h"
#include "SkrCore/memory/memory.h"
#include "SkrRTTR/type_registry.hpp"
#include "SkrRTTR/type.hpp"
#include "SkrRTTR/rttr_traits.hpp"
#include "SkrRTTR/export/extern_methods.hpp"

skr_config_resource_t::~skr_config_resource_t()
{
    if (configType == skr_guid_t{})
        return;
    auto type = skr::get_type_from_guid(configType);
    type->find_default_ctor().invoke(configData);
    sakura_free_aligned(configData, type->alignment());
}

void skr_config_resource_t::SetType(skr_guid_t type)
{
    if (!(configType == skr_guid_t{}))
    {
        SKR_ASSERT(configData);
        auto oldType = skr::get_type_from_guid(configType);
        oldType->invoke_dtor(configData);
        sakura_free_aligned(configData, oldType->alignment());
    }
    configType   = type;
    auto newType = skr::get_type_from_guid(configType);
    configData   = sakura_malloc_aligned(newType->size(), newType->alignment());
    newType->find_default_ctor().invoke(configData);
}

namespace skr
{
bool BinSerde<skr_config_resource_t>::read(SBinaryReader* r, skr_config_resource_t& v)
{
    if (!bin_read(r, v.configType))
        return false;
    if (v.configType == skr_guid_t{})
        return true;
    auto type          = skr::get_type_from_guid(v.configType);
    v.configData       = sakura_malloc_aligned(type->size(), type->alignment());
    using ReadBinProc  = bool(void* o, void* r);
    auto read_bin_data = type->find_extern_method_t<ReadBinProc>(skr::SkrCoreExternMethods::ReadBin);
    read_bin_data.invoke(v.configData, r);
    SKR_UNIMPLEMENTED_FUNCTION();
    return {};
}
bool BinSerde<skr_config_resource_t>::write(SBinaryWriter* w, const skr_config_resource_t& v)
{
    if (!bin_write(w, v.configType))
        return false;
    if (v.configType == skr_guid_t{})
        return true;
    auto type           = skr::get_type_from_guid(v.configType);
    using WriteBinProc  = bool(void* o, void* w);
    auto write_bin_data = type->find_extern_method_t<WriteBinProc>(skr::SkrCoreExternMethods::WriteBin);
    return write_bin_data.invoke(v.configData, w);
    SKR_UNIMPLEMENTED_FUNCTION();
    return {};
}
} // namespace skr

namespace skr
{
skr_guid_t ConfigFactory::GetResourceType() { return skr::type_id_of<skr_config_resource_t>(); }
} // namespace skr
