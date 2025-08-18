#include "SkrRTTR/type_registry.hpp"
#include "SkrContainersDef/map.hpp"
#include "SkrContainersDef/multi_map.hpp"
#include "SkrCore/exec_static.hpp"
#include "SkrCore/log.hpp"
#include "SkrRTTR/type.hpp"
#include "SkrBase/misc.h"
#include <SkrOS/thread.h>

namespace skr
{
// static data
static Map<GUID, RTTRTypeLoaderFunc>& type_load_funcs()
{
    static Map<GUID, RTTRTypeLoaderFunc> s_type_load_funcs;
    return s_type_load_funcs;
}
static Map<GUID, RTTRType*>& loaded_types()
{
    static Map<GUID, RTTRType*> s_types;
    return s_types;
}
static auto& load_type_mutex()
{
    // TODO. use shared_atomic_mutex for better performance
    static SMutexObject s_load_type_mutex;
    return s_load_type_mutex;
}

// auto unload
SKR_EXEC_STATIC_DTOR
{
    unload_all_types();
};

// type register (loader)
void register_type_loader(const GUID& guid, RTTRTypeLoaderFunc load_func)
{
    auto ref = type_load_funcs().find(guid);
    if (ref)
    {
        SKR_LOG_FMT_ERROR(u8"duplicated type loader for guid: {}", guid);
        SKR_DEBUG_BREAK();
    }
    else
    {
        type_load_funcs().add(guid, load_func, ref);
    }
}
void unregister_type_loader(const GUID& guid, RTTRTypeLoaderFunc load_func)
{
    type_load_funcs().remove(guid);
}

// get type (after register)
RTTRType* get_type_from_guid(const GUID& guid)
{
    SMutexLock _lock(load_type_mutex().mMutex);

    auto loaded_result = loaded_types().find(guid);
    if (loaded_result)
    {
        return loaded_result.value();
    }
    else
    {
        auto loader_result = type_load_funcs().find(guid);
        if (loader_result)
        {
            // create type
            auto type = SkrNew<RTTRType>();
            loaded_types().add(guid, type);

            // load type
            loader_result.value()(type);

            // TODO. build optimize data?
            return type;
        }
    }

    return nullptr;
}
void load_all_types()
{
    SMutexLock _lock(load_type_mutex().mMutex);
    for (const auto& [type_id, type_loader] : type_load_funcs())
    {
        if (!loaded_types().contains(type_id))
        {
            // create type
            auto type = SkrNew<RTTRType>();
            loaded_types().add(type_id, type);

            // load type
            type_loader(type);
        }
    }
}
void unload_all_types()
{
    SMutexLock _lock(load_type_mutex().mMutex);

    // release type memory
    for (auto& type : loaded_types())
    {
        SkrDelete(type.value);
    }

    // clear loaded types
    loaded_types().clear();
}

// each types
void each_types(FunctionRef<bool(const RTTRType* type)> func)
{
    for (const auto& [type_id, type] : loaded_types())
    {
        if (!func(type)) { break; }
    }
}
void each_types_of_module(StringView module_name, FunctionRef<bool(const RTTRType* type)> func)
{
    for (const auto& [type_id, type] : loaded_types())
    {
        if (type->module() == module_name)
        {
            if (!func(type)) { break; }
        }
    }
}
} // namespace skr