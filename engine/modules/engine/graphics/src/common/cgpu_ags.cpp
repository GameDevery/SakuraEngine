#include "SkrGraphics/api.h"
#include "SkrGraphics/driver-extensions/cgpu_ags.h"
#include "SkrBase/atomic/atomic.h"

// AGS
#if defined(AMDAGS)

struct CGPUAMDAGSSingleton
{
    static CGPUAMDAGSSingleton* Get(CGPUInstanceId instance)
    {
        static CGPUAMDAGSSingleton* _this = nullptr;
        if (_this == nullptr)
        {
            _this = SkrNew<CGPUAMDAGSSingleton>();
            _this->ref_count = 0;  // Initialize ref count
            {
                #if defined(_WIN64)
                    auto  dllname = u8"amd_ags_x64.dll";
                #elif defined(_WIN32)
                    auto  dllname = u8"amd_ags_x86.dll";
                #endif
                _this->ags_library.load(dllname);
                if (!_this->ags_library.isLoaded())
                {
                    cgpu_trace(u8"%s not found, amd ags is disabled", dllname);
                    _this->dll_dont_exist = true;
                }
                else
                {
                    cgpu_trace(u8"%s loaded", dllname);
                    // Load PFNs
                    _this->_agsInitialize = SKR_SHARED_LIB_LOAD_API(_this->ags_library, agsInitialize);
                    _this->_agsDeInitialize = SKR_SHARED_LIB_LOAD_API(_this->ags_library, agsDeInitialize);
                    // End load PFNs
                }
            }
        }
        return _this->dll_dont_exist ? nullptr : _this;
    }
    
    void AddRef()
    {
        skr_atomic_fetch_add_relaxed(&ref_count, 1);
    }
    
    void Release(CGPUInstanceId instance)
    {
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

        cgpu_trace(u8"AMD AGS unloaded");
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
#endif

ECGPUAGSReturnCode cgpu_ags_init(struct CGPUInstance* Inst)
{
#if defined(AMDAGS)
    auto _this = CGPUAMDAGSSingleton::Get(Inst);
    if (!_this) return CGPU_AGS_FAILURE;
    
    // Increment reference count
    _this->AddRef();
    
    // Only initialize AGS once (when first instance is created)
    if (_this->pAgsContext == NULL)
    {
        AGSConfiguration config = {};
        int apiVersion = AGS_MAKE_VERSION(6, 0, 1);
        auto Status = _this->agsStatus = _this->_agsInitialize(apiVersion, &config, &_this->pAgsContext, &_this->gAgsGpuInfo);
        Inst->ags_status = (ECGPUAGSReturnCode)Status;
        if (Status == AGS_SUCCESS)
        {
            char* stopstring;
            _this->driverVersion = strtoul(_this->gAgsGpuInfo.driverVersion, &stopstring, 10);
        }
    }
    else
    {
        // Already initialized, just set the status
        Inst->ags_status = (ECGPUAGSReturnCode)_this->agsStatus;
    }
    return Inst->ags_status;
#else
    return CGPU_AGS_NONE;
#endif
}

uint32_t cgpu_ags_get_driver_version(CGPUInstanceId instance)
{
#if defined(AMDAGS)
    auto _this = CGPUAMDAGSSingleton::Get(instance);
    if (!_this) return 0;
    
    return _this->driverVersion;
#endif
    return 0;
}

void cgpu_ags_exit(CGPUInstanceId instance)
{
#if defined(AMDAGS)
    auto _this = CGPUAMDAGSSingleton::Get(instance);
    if (!_this) return;

    // Decrement reference count and delete if last reference
    _this->Release(instance);
#endif
}