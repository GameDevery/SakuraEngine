#pragma once
#include "SkrGraphics/raytracing.h"
#include "cgpu_vulkan.h"

#ifdef __cplusplus
extern "C" {
#endif

CGPU_API const CGPURayTracingProcTable* CGPU_VulkanRayTracingProcTable();

typedef struct CGPUAccelerationStructure_Vulkan {
    CGPUAccelerationStructure super;
    
    VkAccelerationStructureKHR mVkAccelerationStructure;
    CGPUBufferId pASBuffer;
    CGPUBufferId pScratchBuffer;
    uint64_t mASDeviceAddress;
    
    VkAccelerationStructureTypeKHR mType;
    VkBuildAccelerationStructureFlagsKHR mFlags;
    uint32_t mDescCount;
    
    union {
        struct {
            VkAccelerationStructureGeometryKHR* pGeometryDescs;
            VkAccelerationStructureBuildRangeInfoKHR* pBuildRangeInfos;
        } asBottom;
        struct {
            CGPUBufferId pInstanceDescBuffer;
        } asTop;
    };
} CGPUAccelerationStructure_Vulkan;

CGPU_API CGPUAccelerationStructureId cgpu_create_acceleration_structure_vulkan(CGPUDeviceId device, const struct CGPUAccelerationStructureDescriptor* desc);
CGPU_API void cgpu_free_acceleration_structure_vulkan(CGPUAccelerationStructureId as);
CGPU_API void cgpu_cmd_build_acceleration_structures_vulkan(CGPUCommandBufferId cmd, const struct CGPUAccelerationStructureBuildDescriptor* desc);

#ifdef __cplusplus
}
#endif