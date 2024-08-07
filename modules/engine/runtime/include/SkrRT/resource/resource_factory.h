#pragma once
#include "SkrRT/config.h"
#include "SkrRT/resource/resource_header.hpp"

typedef enum ESkrInstallStatus
{
    SKR_INSTALL_STATUS_INPROGRESS,
    SKR_INSTALL_STATUS_SUCCEED,
    SKR_INSTALL_STATUS_FAILED,
} ESkrInstallStatus;

typedef struct skr_vfs_t skr_vfs_t;
#if defined(__cplusplus)
    #include "SkrBase/types.h"
    #include "SkrBase/types.h"
namespace skr
{
namespace resource
{
/*
    resource load phase:
    Unloaded => request -> load binary -> deserialize => Loaded
    Loaded  => [requst & wait dependencies -> install/update install] => Installed
*/
struct SKR_RUNTIME_API SResourceFactory {
    virtual skr_guid_t GetResourceType() = 0;
    virtual bool AsyncIO() { return true; }
    /*
        load factor range : [0, 100]
        0 means no async deserialize
        100 means one job per resource
        in between affect the number of jobs per frame, higher value means more jobs per frame
    */
    virtual float AsyncSerdeLoadFactor() { return 1.f; }
    virtual bool Deserialize(skr_resource_record_t* record, SBinaryReader* reader);
#ifdef SKR_RESOURCE_DEV_MODE
    virtual bool DerserializeArtifacts(skr_resource_record_t* record, SBinaryReader* reader) { return 0; };
#endif
    virtual bool Unload(skr_resource_record_t* record);
    virtual ESkrInstallStatus Install(skr_resource_record_t* record) { return ESkrInstallStatus::SKR_INSTALL_STATUS_SUCCEED; }
    virtual bool Uninstall(skr_resource_record_t* record) { return true; }
    virtual ESkrInstallStatus UpdateInstall(skr_resource_record_t* record);
};
} // namespace resource
} // namespace skr
#endif