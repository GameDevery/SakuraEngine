#include "d3d12-agility/d3d12.h"

#include <iostream>
#include <fstream>
#include "SkrGraphics/containers.hpp"
#include "SkrGraphics/extensions/cgpu_nsight.h"
#include "SkrGraphics/extensions/cgpu_d3d12_exts.h"
#include "nsight-aftermath/GFSDK_Aftermath.h" // IWYU pragma: keep
#include "nsight-aftermath/GFSDK_Aftermath_GpuCrashDump.h"
#include "nsight-aftermath/GFSDK_Aftermath_GpuCrashDumpDecoding.h"
#include "./../common/common_utils.h"
#include "./cgpu_nsight_tracker.hpp"
#include "./../winheaders.h" // IWYU pragma: keep

inline cgpu::String AftermathErrorMessage(GFSDK_Aftermath_Result result)
{
    switch (result)
    {
    case GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported:
        return u8"Unsupported driver version - requires an NVIDIA R495 display driver or newer.";
    default:
        return skr::format(u8"Aftermath Error 0x{}", result - GFSDK_Aftermath_Result_Fail);
    }
}

inline static void AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_Result _result)
{
    if (!GFSDK_Aftermath_SUCCEED(_result))   
    {
        cgpu_error(u8"%s\n", AftermathErrorMessage(_result).c_str());
#ifdef _WIN32
        MessageBoxA(0, AftermathErrorMessage(_result).c_str_raw(), "Aftermath Error", MB_OK); 
#endif
        SKR_BREAK();
    }
}

void CGPUNSightSingleton::register_tracker(CGPUNSightTrackerId tracker) SKR_NOEXCEPT
{
    trackers_mutex.lock();
    all_trackers.add(tracker);
    trackers_mutex.unlock();
    CGPUNSightSingleton::rc = (uint32_t)all_trackers.size();
}

void CGPUNSightSingleton::remove_tracker(CGPUNSightTrackerId tracker) SKR_NOEXCEPT
{
    trackers_mutex.lock();
    all_trackers.remove(tracker);
    trackers_mutex.unlock();
    CGPUNSightSingleton::rc = (uint32_t)all_trackers.size();
    if (!CGPUNSightSingleton::rc)
    {
        auto _this = (CGPUNSightSingleton*)cgpu_runtime_table_try_get_custom_data(
            tracker->instance->runtime_table, (const char8_t*)CGPU_NSIGNT_SINGLETON_NAME);
        cgpu_delete(_this);
    }
}

struct CGPUNSightSingletonImpl : public CGPUNSightSingleton
{
    CGPUNSightSingletonImpl() SKR_NOEXCEPT;
    ~CGPUNSightSingletonImpl() SKR_NOEXCEPT;
    
    void WriteShaderDebugInfoToFile(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize)
    {
        // Create a unique file name for writing the shader debug info to a file.
        static int count = 0;
        const cgpu::String baseFileName = skr::format(u8"CGPUShaderDebugInfo-{}", ++count);
        
        // Write the shader debug info data to a file using the .nvdbg extension
        cgpu::String shaderDebugFileName = baseFileName;
        shaderDebugFileName.append(u8".nvdbg");
        std::ofstream debugFile(shaderDebugFileName.c_str_raw(), std::ios::out | std::ios::binary);
        if (debugFile)
        {
            debugFile.write((const char*)pShaderDebugInfo, shaderDebugInfoSize);
            debugFile.close();
            cgpu_trace(u8"NSIGHT Shader Debug Info File Saved: %s", shaderDebugFileName.c_str());
        }
        else
        {
            cgpu_error(u8"Failed to write shader debug info to file: %s", shaderDebugFileName.c_str());
        }
    }
    
    void WriteGpuCrashDumpToFile(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
    {
        // Create a GPU crash dump decoder object for the GPU crash dump.
        GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};
        AFTERMATH_CHECK_ERROR(aftermath_GpuCrashDump_CreateDecoder(
            GFSDK_Aftermath_Version_API,
            pGpuCrashDump,
            gpuCrashDumpSize,
            &decoder));

        // Use the decoder object to read basic information, like application
        // name, PID, etc. from the GPU crash dump.
        GFSDK_Aftermath_GpuCrashDump_BaseInfo baseInfo = {};
        AFTERMATH_CHECK_ERROR(aftermath_GpuCrashDump_GetBaseInfo(decoder, &baseInfo));

        // Use the decoder object to query the application name that was set
        // in the GPU crash dump description.
        uint32_t applicationNameLength = 0;
        AFTERMATH_CHECK_ERROR(aftermath_GpuCrashDump_GetDescriptionSize(
            decoder,
            GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
            &applicationNameLength));

        cgpu::Vector<char8_t> applicationName(applicationNameLength, '\0');
        if (applicationNameLength)
        {
            AFTERMATH_CHECK_ERROR(aftermath_GpuCrashDump_GetDescription(
                decoder,
                GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
                uint32_t(applicationName.size()),
                (char*)applicationName.data()));
        }

        // Create a unique file name for writing the crash dump data to a file.
        // Note: due to an Nsight Aftermath bug (will be fixed in an upcoming
        // driver release) we may see redundant crash dumps. As a workaround,
        // attach a unique count to each generated file name.
        static int count = 0;
        const cgpu::String baseFileNameFmt = applicationNameLength ? cgpu::String(applicationName.data()) : cgpu::String(u8"CGPUApplication-{}-{}");
        const auto baseFileName = skr::format(baseFileNameFmt, baseInfo.pid, ++count);

        // Write the crash dump data to a file using the .nv-gpudmp extension
        // registered with Nsight Graphics.
        cgpu::String crashDumpFileName = baseFileName;
        crashDumpFileName.append(u8".nv-gpudmp");
        std::ofstream dumpFile(crashDumpFileName.c_str_raw(), std::ios::out | std::ios::binary);
        if (dumpFile)
        {
            dumpFile.write((const char*)pGpuCrashDump, gpuCrashDumpSize);
            dumpFile.close();
        }
        // Destroy the GPU crash dump decoder object.
        AFTERMATH_CHECK_ERROR(aftermath_GpuCrashDump_DestroyDecoder(decoder));
        cgpu_trace(u8"NSIGHT GPU Crash Dump File Saved");
    }

    // GPU crash dump callback.
    static void GpuCrashDumpCallback(
        const void* pGpuCrashDump,
        const uint32_t gpuCrashDumpSize,
        void* pUserData)
    {
        auto _this = (CGPUNSightSingleton*)pUserData;
        cgpu_trace(u8"NSIGHT GPU Crash Dump Callback");
        cgpu::Set<struct ID3D12Device*> devices; 
        for (auto tracker : _this->all_trackers)
        {
            auto tracker_impl = static_cast<CGPUNSightTrackerBase*>(tracker);
            if (auto callback = tracker_impl->descriptor.crash_dump_callback)
                callback(pGpuCrashDump, gpuCrashDumpSize, tracker_impl->descriptor.user_data);
        }
        ((CGPUNSightSingletonImpl*)_this)->WriteGpuCrashDumpToFile(pGpuCrashDump, gpuCrashDumpSize);
    }

    // Shader debug information callback.
    static void ShaderDebugInfoCallback(
        const void* pShaderDebugInfo,
        const uint32_t shaderDebugInfoSize,
        void* pUserData)
    {
        auto _this = (CGPUNSightSingleton*)pUserData;
        cgpu_trace(u8"NSIGHT Shader Debug Info Callback");
        for (auto tracker : _this->all_trackers)
        {
            auto tracker_impl = static_cast<CGPUNSightTrackerBase*>(tracker);
            if (auto callback = tracker_impl->descriptor.shader_debug_info_callback)
                callback(pShaderDebugInfo, shaderDebugInfoSize, tracker_impl->descriptor.user_data);
        }
        // Write shader debug info to file
        ((CGPUNSightSingletonImpl*)_this)->WriteShaderDebugInfoToFile(pShaderDebugInfo, shaderDebugInfoSize);
    }

    // GPU crash dump description callback.
    static void CrashDumpDescriptionCallback(
        PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription,
        void* pUserData)
    {
        auto _this = (CGPUNSightSingleton*)pUserData;
        cgpu_trace(u8"NSIGHT Dump Description Callback");
        for (auto tracker : _this->all_trackers)
        {
            auto tracker_impl = static_cast<CGPUNSightTrackerBase*>(tracker);
            if (auto callback = tracker_impl->descriptor.crash_dump_description_callback)
                callback(addDescription, tracker_impl->descriptor.user_data);
        }
    }

    // App-managed marker resolve callback
    // Markers are deprecated now, we use vkCmdFillBuffer & ID3D12GraphicsCommandList2::WriteBufferImmediate instead
    static void ResolveMarkerCallback(
        const void* pMarker,
        const unsigned int,
        void* pUserData,
        void** resolvedMarkerData,
        uint32_t* markerSize
    )
    {
        cgpu_trace(u8"NSIGHT Resolve Marker Callback");
    }

    skr::SharedLibrary nsight_library;
    skr::SharedLibrary llvm_library;
    SKR_SHARED_LIB_API_PFN(GFSDK_Aftermath_EnableGpuCrashDumps) aftermath_EnableGpuCrashDumps = nullptr;
    SKR_SHARED_LIB_API_PFN(GFSDK_Aftermath_DisableGpuCrashDumps) aftermath_DisableGpuCrashDumps = nullptr;
    SKR_SHARED_LIB_API_PFN(GFSDK_Aftermath_GpuCrashDump_CreateDecoder) aftermath_GpuCrashDump_CreateDecoder = nullptr;
    SKR_SHARED_LIB_API_PFN(GFSDK_Aftermath_GpuCrashDump_GetBaseInfo) aftermath_GpuCrashDump_GetBaseInfo = nullptr;
    SKR_SHARED_LIB_API_PFN(GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize) aftermath_GpuCrashDump_GetDescriptionSize = nullptr;
    SKR_SHARED_LIB_API_PFN(GFSDK_Aftermath_GpuCrashDump_GetDescription) aftermath_GpuCrashDump_GetDescription = nullptr;
    SKR_SHARED_LIB_API_PFN(GFSDK_Aftermath_GpuCrashDump_DestroyDecoder) aftermath_GpuCrashDump_DestroyDecoder = nullptr;
    SKR_SHARED_LIB_API_PFN(GFSDK_Aftermath_DX12_Initialize) aftermath_DX12_Initialize = nullptr;
};


static std::uint32_t counter = 0;
CGPUNSightSingletonImpl::CGPUNSightSingletonImpl() SKR_NOEXCEPT
{
    bool nsight = nsight_library.load(u8"GFSDK_Aftermath_Lib.x64.dll");
    if (nsight)
    {
        cgpu_trace(u8"NSIGHT loaded");
        aftermath_EnableGpuCrashDumps = SKR_SHARED_LIB_LOAD_API(nsight_library, GFSDK_Aftermath_EnableGpuCrashDumps);
        aftermath_DisableGpuCrashDumps = SKR_SHARED_LIB_LOAD_API(nsight_library, GFSDK_Aftermath_DisableGpuCrashDumps);
        aftermath_GpuCrashDump_CreateDecoder = SKR_SHARED_LIB_LOAD_API(nsight_library, GFSDK_Aftermath_GpuCrashDump_CreateDecoder);
        aftermath_GpuCrashDump_GetBaseInfo = SKR_SHARED_LIB_LOAD_API(nsight_library, GFSDK_Aftermath_GpuCrashDump_GetBaseInfo);
        aftermath_GpuCrashDump_GetDescriptionSize = SKR_SHARED_LIB_LOAD_API(nsight_library, GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize);
        aftermath_GpuCrashDump_GetDescription = SKR_SHARED_LIB_LOAD_API(nsight_library, GFSDK_Aftermath_GpuCrashDump_GetDescription);
        aftermath_GpuCrashDump_DestroyDecoder = SKR_SHARED_LIB_LOAD_API(nsight_library, GFSDK_Aftermath_GpuCrashDump_DestroyDecoder);
        aftermath_DX12_Initialize = SKR_SHARED_LIB_LOAD_API(nsight_library, GFSDK_Aftermath_DX12_Initialize);
    }
    else
    {
        cgpu_trace(u8"NSIGHT dll not found");
    }
    if (aftermath_EnableGpuCrashDumps && (counter == 0))
    {
        AFTERMATH_CHECK_ERROR(aftermath_EnableGpuCrashDumps(
            GFSDK_Aftermath_Version_API,
            GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX | GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan,
            GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks, // Let the Nsight Aftermath library cache shader debug information.
            &GpuCrashDumpCallback,                                             // Register callback for GPU crash dumps.
            &ShaderDebugInfoCallback,                                          // Register callback for shader debug information.
            &CrashDumpDescriptionCallback,                                     // Register callback for GPU crash dump description.
            &ResolveMarkerCallback,                                            // Register callback for resolving application-managed markers.
            this));      
    }
    counter += 1;
}

CGPUNSightSingletonImpl::~CGPUNSightSingletonImpl() SKR_NOEXCEPT
{
    counter -= 1;

    if (counter == 0)
    {
        if (aftermath_DisableGpuCrashDumps) aftermath_DisableGpuCrashDumps();
        if (nsight_library.isLoaded()) nsight_library.unload();
        if (llvm_library.isLoaded()) llvm_library.unload();
        cgpu_trace(u8"NSIGHT aftermath unloaded");
    }
}

CGPUNSightSingleton* CGPUNSightSingleton::Get(CGPUInstanceId instance) SKR_NOEXCEPT
{
    auto _this = (CGPUNSightSingleton*)cgpu_runtime_table_try_get_custom_data(instance->runtime_table, (const char8_t*)CGPU_NSIGNT_SINGLETON_NAME);
    if (_this == nullptr)
    {
        _this = cgpu_new<CGPUNSightSingletonImpl>();
        cgpu_runtime_table_add_custom_data(instance->runtime_table, (const char8_t*)CGPU_NSIGNT_SINGLETON_NAME, _this);
    }
    return _this;
}

void cgpu_nsight_initialize_dx12_aftermath(CGPUInstanceId instance, ID3D12Device* device) 
{
    // Get the singleton instance
    auto singleton = CGPUNSightSingleton::Get(instance);
    auto impl = static_cast<CGPUNSightSingletonImpl*>(singleton);
    if (impl && impl->aftermath_DX12_Initialize)
    {
        const uint32_t aftermathFlags = 
            GFSDK_Aftermath_FeatureFlags_EnableMarkers |             // Enable event marker tracking.
            GFSDK_Aftermath_FeatureFlags_EnableResourceTracking |    // Enable tracking of resources.
            GFSDK_Aftermath_FeatureFlags_CallStackCapturing |        // Capture call stacks for all draw calls, compute dispatches, and resource copies.
            GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo;    // Generate shader debug info for better debugging
        
        GFSDK_Aftermath_Result result = impl->aftermath_DX12_Initialize(
            GFSDK_Aftermath_Version_API,
            aftermathFlags,
            device);
        
        if (GFSDK_Aftermath_SUCCEED(result))
        {
            cgpu_trace(u8"NSIGHT Aftermath DX12 initialized with shader debug info generation");
        }
        else
        {
            cgpu_error(u8"Failed to initialize NSIGHT Aftermath for DX12: 0x%x", result);
        }
    }
}

CGPUNSightTrackerBase::CGPUNSightTrackerBase(CGPUInstanceId instance, const CGPUNSightTrackerDescriptor* pdesc) SKR_NOEXCEPT
    : descriptor(*pdesc)
{
    this->instance = instance;
    singleton = CGPUNSightSingleton::Get(instance);
    singleton->register_tracker(this);
}

CGPUNSightTrackerBase::~CGPUNSightTrackerBase() SKR_NOEXCEPT
{
    singleton = CGPUNSightSingleton::Get(instance);
    singleton->remove_tracker(this);
}

CGPUNSightTrackerId cgpu_create_nsight_tracker(CGPUDeviceId device, const CGPUNSightTrackerDescriptor* descriptor)
{
    CGPUInstanceId instance = device->adapter->instance;
    // TODO: HOOK THIS TO DEVICE CREATION & SUPPORT VULKAN DEVICES
    if (device->adapter->instance->backend == CGPU_BACKEND_D3D12)
    {
        auto d3d12_device = cgpu_d3d12_get_device(device);
        cgpu_nsight_initialize_dx12_aftermath(instance, d3d12_device);
    }
    else if (instance->backend == CGPU_BACKEND_VULKAN)
    {
        /* vulkan setup example from sdk doc:
        // Enable NV_device_diagnostic_checkpoints extension to be able to
        // use Aftermath event markers.
        extensionNames.push_back(VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME);

        // Enable NV_device_diagnostics_config extension to configure Aftermath
        // features.
        extensionNames.push_back(VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME);

        // Set up device creation info for Aftermath feature flag configuration.
        VkDeviceDiagnosticsConfigFlagsNV aftermathFlags =
            VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV |  // Enable automatic call stack checkpoints.
            VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV |      // Enable tracking of resources.
            VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV |      // Generate debug information for shaders.
            VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_ERROR_REPORTING_BIT_NV;  // Enable additional runtime shader error reporting.

        VkDeviceDiagnosticsConfigCreateInfoNV aftermathInfo = {};
        aftermathInfo.sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
        aftermathInfo.flags = aftermathFlags;

        // Set up device creation info.
        VkDeviceCreateInfo deviceInfo = {};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceInfo.pNext = &aftermathInfo;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.enabledExtensionCount = extensionNames.size();
        deviceInfo.ppEnabledExtensionNames = extensionNames.data();

        // Create the logical device.
        VkDevice device;
        vkCreateDevice(physicalDevice, &deviceInfo, NULL, &device);
        */
    }
    return cgpu_new<CGPUNSightTrackerBase>(device->adapter->instance, descriptor);
}

void cgpu_free_nsight_tracker(CGPUNSightTrackerId tracker)
{
    cgpu_delete(tracker);
}