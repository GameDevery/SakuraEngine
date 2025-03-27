#pragma once
#include "SkrBase/types.h"
#include "SkrContainersDef/function_ref.hpp"
#include "SkrContainersDef/string.hpp"

namespace skr
{
struct RTTRType;
using RTTRTypeLoaderFunc = void (*)(RTTRType* type);

// type register (loader)
SKR_CORE_API void register_type_loader(const GUID& guid, RTTRTypeLoaderFunc load_func);
SKR_CORE_API void unregister_type_loader(const GUID& guid, RTTRTypeLoaderFunc load_func);

// TODO. type extender

// get type (after register)
SKR_CORE_API RTTRType* get_type_from_guid(const GUID& guid);
SKR_CORE_API void      load_all_types();
SKR_CORE_API void      unload_all_types();

// each types
SKR_CORE_API void each_types(FunctionRef<bool(const RTTRType* type)> func);
SKR_CORE_API void each_types_of_module(StringView module_name, FunctionRef<bool(const RTTRType* type)> func);

} // namespace skr