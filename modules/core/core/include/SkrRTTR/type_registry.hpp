#pragma once
#include "SkrBase/types.h"

namespace skr
{
struct RTTRType;
using RTTRTypeLoaderFunc = void (*)(RTTRType* type);

// type register (loader)
SKR_CORE_API void register_type_loader(const GUID& guid, RTTRTypeLoaderFunc load_func);
SKR_CORE_API void unregister_type_loader(const GUID& guid, RTTRTypeLoaderFunc load_func);

// TODO. type extender

// TODO. type convert for any support

// get type (after register)
SKR_CORE_API RTTRType* get_type_from_guid(const GUID& guid);
SKR_CORE_API void      unload_all_types();

} // namespace skr