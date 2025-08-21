#include "SkrGraphics/api.h"
#include "SkrGraphics/driver-extensions/cgpu_ags.h"
#include "SkrBase/misc/defer.hpp"
#include "SkrBase/atomic/atomic.h"
#include "SkrBase/atomic/atomic_mutex.hpp"

// AGS
#if defined(AMDAGS)

struct CGPUAMDAGSSingleton
{
    static skr::shared_atomic_mutex ags_init_mtx;
    static CGPUAMDAGSSingleton* ags_instance;

    static ECGPUAGSReturnCode Initialize(struct CGPUInstance* Inst)
    {
        ags_init_mtx.lock();
        SKR_DEFER({ ags_init_mtx.unlock(); });

        auto ags_instance = CGPUAMDAGSSingleton::Get(Inst);
        if (!ags_instance) return CGPU_AGS_FAILURE;
        
        // Increment reference count
        ags_instance->AddRef();
        
        // Only initialize AGS once (when first instance is created)
        if (ags_instance->pAgsContext == NULL)
        {
            AGSConfiguration config = {};
            int apiVersion = AGS_MAKE_VERSION(AMD_AGS_VERSION_MAJOR, AMD_AGS_VERSION_MINOR, AMD_AGS_VERSION_PATCH);
            auto Status = ags_instance->agsStatus = ags_instance->_agsInitialize(apiVersion, &config, &ags_instance->pAgsContext, &ags_instance->gAgsGpuInfo);
            Inst->ags_status = (ECGPUAGSReturnCode)Status;
            if (Status == AGS_SUCCESS)
            {
                char* stopstring;
                ags_instance->driverVersion = strtoul(ags_instance->gAgsGpuInfo.driverVersion, &stopstring, 10);
            }
        }
        else
        {
            // Already initialized, just set the status
            Inst->ags_status = (ECGPUAGSReturnCode)ags_instance->agsStatus;
        }
        return Inst->ags_status;
    }

    static CGPUAMDAGSSingleton* Get(CGPUInstanceId instance)
    {
        if (ags_instance == nullptr)
        {
            ags_instance = SkrNew<CGPUAMDAGSSingleton>();
            ags_instance->ref_count = 0;  // Initialize ref count
            {
                #if defined(_WIN64)
                    auto  dllname = u8"amd_ags_x64.dll";
                #elif defined(_WIN32)
                    auto  dllname = u8"amd_ags_x86.dll";
                #endif
                ags_instance->ags_library.load(dllname);
                if (!ags_instance->ags_library.isLoaded())
                {
                    cgpu_trace(u8"%s not found, amd ags is disabled", dllname);
                    ags_instance->dll_dont_exist = true;
                }
                else
                {
                    cgpu_trace(u8"%s loaded", dllname);
                    // Load PFNs
                    ags_instance->_agsInitialize = SKR_SHARED_LIB_LOAD_API(ags_instance->ags_library, agsInitialize);
                    ags_instance->_agsDeInitialize = SKR_SHARED_LIB_LOAD_API(ags_instance->ags_library, agsDeInitialize);
                    // End load PFNs
                }
            }
        }
        return ags_instance->dll_dont_exist ? nullptr : ags_instance;
    }
    
    void AddRef()
    {
        skr_atomic_fetch_add_relaxed(&ref_count, 1);
    }
    
    void Release(CGPUInstanceId instance)
    {
        ags_init_mtx.lock();
        SKR_DEFER({ ags_init_mtx.unlock(); });

        if (skr_atomic_fetch_add_relaxed(&ref_count, -1) == 1)
        {
            // Last reference, remove from runtime table and delete
            SkrDelete(this);
        }
    }

    ~CGPUAMDAGSSingleton()
    {
        if (_agsDeInitialize && (agsStatus == AGS_SUCCESS)) 
            _agsDeInitialize(pAgsContext);
        if (ags_library.isLoaded()) 
            ags_library.unload();
    }

    SKR_SHARED_LIB_API_PFN(agsInitialize) _agsInitialize = nullptr;
    SKR_SHARED_LIB_API_PFN(agsDeInitialize) _agsDeInitialize = nullptr;

    AGSReturnCode agsStatus = AGS_SUCCESS;
    AGSContext* pAgsContext = NULL;
    AGSGPUInfo gAgsGpuInfo = {};
    uint32_t driverVersion = 0;
    skr::SharedLibrary ags_library;
    bool dll_dont_exist = false;
    SAtomicU32 ref_count = 0;
};

skr::shared_atomic_mutex CGPUAMDAGSSingleton::ags_init_mtx = {};
CGPUAMDAGSSingleton* CGPUAMDAGSSingleton::ags_instance = nullptr;

#endif

ECGPUAGSReturnCode cgpu_ags_init(struct CGPUInstance* Inst)
{
#if defined(AMDAGS)
    return CGPUAMDAGSSingleton::Initialize(Inst);
#else
    return CGPU_AGS_NONE;
#endif
}

uint32_t cgpu_ags_get_driver_version(CGPUInstanceId instance)
{
#if defined(AMDAGS)
    auto ags_instance = CGPUAMDAGSSingleton::Get(instance);
    if (!ags_instance) return 0;
    
    return ags_instance->driverVersion;
#endif
    return 0;
}

void cgpu_ags_exit(CGPUInstanceId instance)
{
#if defined(AMDAGS)
    auto ags_instance = CGPUAMDAGSSingleton::Get(instance);
    if (!ags_instance) return;

    // Decrement reference count and delete if last reference
    ags_instance->Release(instance);
#endif
}