#include "SkrGraphics/config.h"
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
#endif

#ifdef ENABLE_NSIGHT_AFTERMATH
    #include "extensions/cgpu_nsight_aftermath.cpp"
#else
    #include "SkrGraphics/extensions/cgpu_nsight.h"
    CGPUNSightTrackerId cgpu_create_nsight_tracker(CGPUInstanceId instance, const CGPUNSightTrackerDescriptor* descriptor)
    {
        return nullptr;
    }
    void cgpu_free_nsight_tracker(CGPUNSightTrackerId tracker)
    {

    }
#endif

#ifdef ENABLE_NSIGHT_PERF
    #include "extensions/cgpu_nsight_perf.cpp"
#else
    #include "SkrGraphics/extensions/cgpu_nsight.h"

#endif