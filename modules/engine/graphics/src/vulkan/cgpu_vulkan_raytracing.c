#include "SkrGraphics/backend/vulkan/cgpu_vulkan_raytracing.h"
#include "vulkan_utils.h"
#include "SkrBase/misc/debug.h"
#include "SkrCore/log.h"
#include "SkrProfile/profile.h"

const CGPURayTracingProcTable rt_tbl_vulkan = {
    .create_acceleration_structure = &cgpu_create_acceleration_structure_vulkan,
    .free_acceleration_structure = &cgpu_free_acceleration_structure_vulkan,
    .cmd_build_acceleration_structure = &cgpu_cmd_build_acceleration_structures_vulkan,
};

const CGPURayTracingProcTable* CGPU_VulkanRayTracingProcTable()
{
    return &rt_tbl_vulkan;
}

inline static VkAccelerationStructureTypeKHR ToVkASType(ECGPUAccelerationStructureType type);
inline static VkBuildAccelerationStructureFlagsKHR ToVkBuildFlags(CGPUAccelerationStructureBuildFlags flags);
inline static VkGeometryFlagsKHR ToVkGeometryFlags(CGPUAccelerationStructureGeometryFlags flags);
inline static VkGeometryInstanceFlagsKHR ToVkInstanceFlags(CGPUAccelerationStructureInstanceFlags flags);

CGPUAccelerationStructureId cgpu_create_acceleration_structure_vulkan(CGPUDeviceId device, const struct CGPUAccelerationStructureDescriptor* desc)
{
    CGPUDevice_Vulkan* D = (CGPUDevice_Vulkan*)device;
    const CGPUAdapter_Vulkan* A = (CGPUAdapter_Vulkan*)device->adapter;
    size_t memSize = sizeof(CGPUAccelerationStructure_Vulkan);
    if (desc->type == CGPU_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL)
    {
        memSize += desc->bottom.count * sizeof(VkAccelerationStructureGeometryKHR);
        memSize += desc->bottom.count * sizeof(VkAccelerationStructureBuildRangeInfoKHR);
    }

    CGPUAccelerationStructure_Vulkan* AS = (CGPUAccelerationStructure_Vulkan*)cgpu_calloc(1, memSize);
    AS->mType = ToVkASType(desc->type);
    AS->mFlags = ToVkBuildFlags(desc->flags);
    AS->super.device = device;
    AS->super.type = desc->type;

    AS->super.scratch_buffer_size = 0;
    if (desc->type == CGPU_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL)
    {
        SkrCZoneN(zz, "CreateBLAS", 1);
        AS->mDescCount = desc->bottom.count;
        AS->asBottom.pGeometryDescs = (VkAccelerationStructureGeometryKHR*)(AS + 1);
        AS->asBottom.pBuildRangeInfos = (VkAccelerationStructureBuildRangeInfoKHR*)(AS->asBottom.pGeometryDescs + AS->mDescCount);
        
        for (uint32_t j = 0; j < AS->mDescCount; ++j)
        {
            const CGPUAccelerationStructureGeometryDesc* pGeom = &desc->bottom.geometries[j];
            VkAccelerationStructureGeometryKHR* pGeomVk = &AS->asBottom.pGeometryDescs[j];
            VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &AS->asBottom.pBuildRangeInfos[j];

            *pGeomVk = (VkAccelerationStructureGeometryKHR){
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
                .pNext = NULL,
                .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
                .flags = ToVkGeometryFlags(pGeom->flags)
            };

            VkAccelerationStructureGeometryTrianglesDataKHR* triangles = &pGeomVk->geometry.triangles;
            triangles->sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            triangles->pNext = NULL;

            cgpu_assert(pGeom->vertex_buffer);
            cgpu_assert(pGeom->vertex_count);
            const CGPUBuffer_Vulkan* pVertexBuffer = (CGPUBuffer_Vulkan*)pGeom->vertex_buffer;
            triangles->vertexData.deviceAddress = D->mVkDeviceTable.vkGetBufferDeviceAddress(D->pVkDevice, 
                &(VkBufferDeviceAddressInfo){
                    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                    .buffer = pVertexBuffer->pVkBuffer
                }) + pGeom->vertex_offset;
            triangles->vertexStride = pGeom->vertex_stride;
            triangles->vertexFormat = VkUtil_FormatTranslateToVk(pGeom->vertex_format);
            triangles->maxVertex = pGeom->vertex_count - 1;

            if (pGeom->index_count)
            {
                cgpu_assert(pGeom->index_buffer);
                const CGPUBuffer_Vulkan* pIndexBuffer = (CGPUBuffer_Vulkan*)pGeom->index_buffer;
                triangles->indexData.deviceAddress = D->mVkDeviceTable.vkGetBufferDeviceAddress(D->pVkDevice,
                    &(VkBufferDeviceAddressInfo){
                        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                        .buffer = pIndexBuffer->pVkBuffer
                    }) + pGeom->index_offset;
                triangles->indexType = (pGeom->index_stride == sizeof(uint16_t)) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
                
                pRangeInfo->primitiveCount = pGeom->index_count / 3;
            }
            else
            {
                triangles->indexType = VK_INDEX_TYPE_NONE_KHR;
                pRangeInfo->primitiveCount = pGeom->vertex_count / 3;
            }

            pRangeInfo->primitiveOffset = 0;
            pRangeInfo->firstVertex = 0;
            pRangeInfo->transformOffset = 0;
        }

        // Get the size requirement for the Acceleration Structures
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .pNext = NULL,
            .type = AS->mType,
            .flags = AS->mFlags,
            .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .srcAccelerationStructure = VK_NULL_HANDLE,
            .dstAccelerationStructure = VK_NULL_HANDLE,
            .geometryCount = AS->mDescCount,
            .pGeometries = AS->asBottom.pGeometryDescs,
            .ppGeometries = NULL,
            .scratchData = {0}
        };

        VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
            .pNext = NULL
        };

        SKR_DECLARE_ZERO_VLA(uint32_t, pMaxPrimitiveCounts, AS->mDescCount);
        for (uint32_t i = 0; i < AS->mDescCount; ++i)
        {
            pMaxPrimitiveCounts[i] = AS->asBottom.pBuildRangeInfos[i].primitiveCount;
        }

        {
            SkrCZoneN(zzz, "vkGetAccelerationStructureBuildSizesKHR", 1);
            D->mVkDeviceTable.vkGetAccelerationStructureBuildSizesKHR(D->pVkDevice,
                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &buildInfo, pMaxPrimitiveCounts, &sizeInfo);
            SkrCZoneEnd(zzz);
        }

        // Allocate Acceleration Structure Buffer
        {
            SkrCZoneN(zzz, "CreateASBuffer", 1);
            CGPUBufferDescriptor bufferDesc = {
                .descriptors = CGPU_RESOURCE_TYPE_RW_BUFFER,
                .memory_usage = CGPU_MEM_USAGE_GPU_ONLY,
                .flags = CGPU_BCF_NO_DESCRIPTOR_VIEW_CREATION | CGPU_BCF_DEDICATED_BIT,
                .element_stride = sizeof(uint32_t),
                .first_element = 0,
                .element_count = (uint32_t)(sizeInfo.accelerationStructureSize / sizeof(uint32_t)),
                .size = sizeInfo.accelerationStructureSize,
                .start_state = CGPU_RESOURCE_STATE_ACCELERATION_STRUCTURE_WRITE
            };
            AS->pASBuffer = cgpu_create_buffer(device, &bufferDesc);
            AS->super.scratch_buffer_size = (uint32_t)sizeInfo.buildScratchSize;
            SkrCZoneEnd(zzz);
        }
        SkrCZoneEnd(zz);
    }
    else if (desc->type == CGPU_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL)
    {
        SkrCZoneN(zz, "CreateTLAS", 1);
        AS->mDescCount = desc->top.count;

        // Create instance buffer
        CGPUBufferDescriptor instanceDesc = {
            .memory_usage = CGPU_MEM_USAGE_CPU_TO_GPU,
            .flags = CGPU_BCF_PERSISTENT_MAP_BIT,
            .size = desc->top.count * sizeof(VkAccelerationStructureInstanceKHR),
        };
        AS->asTop.pInstanceDescBuffer = cgpu_create_buffer(device, &instanceDesc);

        // Fill instance data
        VkAccelerationStructureInstanceKHR* instanceDescs = 
            (VkAccelerationStructureInstanceKHR*)AS->asTop.pInstanceDescBuffer->info->cpu_mapped_address;

        for (uint32_t i = 0; i < desc->top.count; ++i)
        {
            const CGPUAccelerationStructureInstanceDesc* pInst = &desc->top.instances[i];
            CGPUAccelerationStructure_Vulkan* pBLAS = (CGPUAccelerationStructure_Vulkan*)pInst->bottom;
            cgpu_assert(pBLAS && "Instance Bottom AS must not be NULL!");

            VkAccelerationStructureInstanceKHR* pInstanceVk = &instanceDescs[i];
            
            // Copy transform matrix (3x4)
            memcpy(&pInstanceVk->transform, pInst->transform, sizeof(float[12]));
            
            pInstanceVk->instanceCustomIndex = pInst->instance_id & 0xFFFFFF; // 24 bits
            pInstanceVk->mask = pInst->instance_mask;
            pInstanceVk->instanceShaderBindingTableRecordOffset = 0; // TODO: support hit group index
            pInstanceVk->flags = ToVkInstanceFlags(pInst->flags);
            pInstanceVk->accelerationStructureReference = pBLAS->mASDeviceAddress;
        }

        // Get the size requirement for the Acceleration Structures
        VkAccelerationStructureGeometryKHR topASGeometry = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .pNext = NULL,
            .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
            .flags = 0
        };

        const CGPUBuffer_Vulkan* pInstanceBuffer = (CGPUBuffer_Vulkan*)AS->asTop.pInstanceDescBuffer;
        topASGeometry.geometry.instances = (VkAccelerationStructureGeometryInstancesDataKHR){
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
            .pNext = NULL,
            .arrayOfPointers = VK_FALSE,
            .data.deviceAddress = D->mVkDeviceTable.vkGetBufferDeviceAddress(D->pVkDevice,
                &(VkBufferDeviceAddressInfo){
                    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                    .buffer = pInstanceBuffer->pVkBuffer
                })
        };

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .pNext = NULL,
            .type = AS->mType,
            .flags = AS->mFlags,
            .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .srcAccelerationStructure = VK_NULL_HANDLE,
            .dstAccelerationStructure = VK_NULL_HANDLE,
            .geometryCount = 1,
            .pGeometries = &topASGeometry,
            .ppGeometries = NULL,
            .scratchData = {0}
        };

        VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
            .pNext = NULL
        };

        D->mVkDeviceTable.vkGetAccelerationStructureBuildSizesKHR(D->pVkDevice,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &buildInfo, &AS->mDescCount, &sizeInfo);

        // Allocate Acceleration Structure Buffer
        CGPUBufferDescriptor bufferDesc = {
            .descriptors = CGPU_RESOURCE_TYPE_RW_BUFFER_RAW | CGPU_RESOURCE_TYPE_BUFFER_RAW,
            .memory_usage = CGPU_MEM_USAGE_GPU_ONLY,
            .flags = CGPU_BCF_DEDICATED_BIT,
            .element_stride = sizeof(uint32_t),
            .first_element = 0,
            .element_count = (uint32_t)(sizeInfo.accelerationStructureSize / sizeof(uint32_t)),
            .size = sizeInfo.accelerationStructureSize,
            .start_state = CGPU_RESOURCE_STATE_ACCELERATION_STRUCTURE_WRITE
        };
        AS->pASBuffer = cgpu_create_buffer(device, &bufferDesc);
        AS->super.scratch_buffer_size = (uint32_t)sizeInfo.buildScratchSize;
        SkrCZoneEnd(zz);
    }

    // Create VkAccelerationStructure
    {
        const CGPUBuffer_Vulkan* pASBuffer = (CGPUBuffer_Vulkan*)AS->pASBuffer;
        VkAccelerationStructureCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
            .pNext = NULL,
            .createFlags = 0,
            .buffer = pASBuffer->pVkBuffer,
            .offset = 0,
            .size = AS->pASBuffer->info->size,
            .type = AS->mType,
            .deviceAddress = 0
        };

        CHECK_VKRESULT(D->mVkDeviceTable.vkCreateAccelerationStructureKHR(D->pVkDevice, &createInfo, 
            GLOBAL_VkAllocationCallbacks, &AS->mVkAccelerationStructure));

        // Get device address
        VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
            .pNext = NULL,
            .accelerationStructure = AS->mVkAccelerationStructure
        };
        AS->mASDeviceAddress = D->mVkDeviceTable.vkGetAccelerationStructureDeviceAddressKHR(D->pVkDevice, &addressInfo);
    }

    // Create scratch buffer
    CGPUBufferDescriptor scratchBufferDesc = {
        .descriptors = CGPU_RESOURCE_TYPE_RW_BUFFER,
        .memory_usage = CGPU_MEM_USAGE_GPU_ONLY,
        .start_state = CGPU_RESOURCE_STATE_COMMON,
        .flags = CGPU_BCF_NO_DESCRIPTOR_VIEW_CREATION,
        .size = AS->super.scratch_buffer_size,
    };
    AS->pScratchBuffer = cgpu_create_buffer(device, &scratchBufferDesc);

    return &AS->super;
}

void cgpu_free_acceleration_structure_vulkan(CGPUAccelerationStructureId as)
{
    CGPUAccelerationStructure_Vulkan* AS = (CGPUAccelerationStructure_Vulkan*)as;
    CGPUDevice_Vulkan* D = (CGPUDevice_Vulkan*)as->device;

    if (AS->mVkAccelerationStructure)
    {
        D->mVkDeviceTable.vkDestroyAccelerationStructureKHR(D->pVkDevice, AS->mVkAccelerationStructure, GLOBAL_VkAllocationCallbacks);
        AS->mVkAccelerationStructure = VK_NULL_HANDLE;
    }

    if (AS->pASBuffer)
    {
        cgpu_free_buffer(AS->pASBuffer);
        AS->pASBuffer = CGPU_NULLPTR;
    }
    if (AS->pScratchBuffer)
    {
        cgpu_free_buffer(AS->pScratchBuffer);
        AS->pScratchBuffer = CGPU_NULLPTR;
    }
    if (AS->super.type == CGPU_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL)
    {
        if (AS->asTop.pInstanceDescBuffer)
        {
            cgpu_free_buffer(AS->asTop.pInstanceDescBuffer);
            AS->asTop.pInstanceDescBuffer = CGPU_NULLPTR;
        }
    }
    cgpu_free(AS);
}

void cgpu_cmd_build_acceleration_structures_vulkan(CGPUCommandBufferId cmd, const struct CGPUAccelerationStructureBuildDescriptor* desc)
{
    const CGPUCommandBuffer_Vulkan* CMD = (const CGPUCommandBuffer_Vulkan*)cmd;
    CGPUDevice_Vulkan* D = (CGPUDevice_Vulkan*)cmd->device;

    SKR_DECLARE_ZERO_VLA(VkAccelerationStructureBuildGeometryInfoKHR, buildInfos, desc->as_count)
    SKR_DECLARE_ZERO_VLA(VkAccelerationStructureBuildRangeInfoKHR*, rangeInfos, desc->as_count)

    for (uint32_t i = 0; i < desc->as_count; i++)
    {
        CGPUAccelerationStructure_Vulkan* AS = (CGPUAccelerationStructure_Vulkan*)desc->as[i];
        const CGPUBuffer_Vulkan* pScratchBuffer = (CGPUBuffer_Vulkan*)AS->pScratchBuffer;

        buildInfos[i] = (VkAccelerationStructureBuildGeometryInfoKHR){
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
            .pNext = NULL,
            .type = AS->mType,
            .flags = AS->mFlags,
            .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
            .srcAccelerationStructure = VK_NULL_HANDLE,
            .dstAccelerationStructure = AS->mVkAccelerationStructure,
            .geometryCount = 0,
            .pGeometries = NULL,
            .ppGeometries = NULL,
            .scratchData.deviceAddress = D->mVkDeviceTable.vkGetBufferDeviceAddress(D->pVkDevice,
                &(VkBufferDeviceAddressInfo){
                    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                    .buffer = pScratchBuffer->pVkBuffer
                })
        };

        if (AS->mType == VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR)
        {
            buildInfos[i].geometryCount = AS->mDescCount;
            buildInfos[i].pGeometries = AS->asBottom.pGeometryDescs;
            rangeInfos[i] = AS->asBottom.pBuildRangeInfos;
        }
        else if (AS->mType == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR)
        {
            SKR_DECLARE_ZERO_VLA(VkAccelerationStructureGeometryKHR, topASGeometry, 1);
            topASGeometry[0] = (VkAccelerationStructureGeometryKHR){
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
                .pNext = NULL,
                .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
                .flags = 0
            };

            const CGPUBuffer_Vulkan* pInstanceBuffer = (CGPUBuffer_Vulkan*)AS->asTop.pInstanceDescBuffer;
            topASGeometry[0].geometry.instances = (VkAccelerationStructureGeometryInstancesDataKHR){
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
                .pNext = NULL,
                .arrayOfPointers = VK_FALSE,
                .data.deviceAddress = D->mVkDeviceTable.vkGetBufferDeviceAddress(D->pVkDevice,
                    &(VkBufferDeviceAddressInfo){
                        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                        .buffer = pInstanceBuffer->pVkBuffer
                    })
            };

            buildInfos[i].geometryCount = 1;
            buildInfos[i].pGeometries = topASGeometry;

            SKR_DECLARE_ZERO_VLA(VkAccelerationStructureBuildRangeInfoKHR, rangeInfo, 1);
            rangeInfo[0] = (VkAccelerationStructureBuildRangeInfoKHR){
                .primitiveCount = AS->mDescCount,
                .primitiveOffset = 0,
                .firstVertex = 0,
                .transformOffset = 0
            };
            rangeInfos[i] = rangeInfo;
        }
    }

    D->mVkDeviceTable.vkCmdBuildAccelerationStructuresKHR(CMD->pVkCmdBuf, desc->as_count, buildInfos, (const VkAccelerationStructureBuildRangeInfoKHR* const*)rangeInfos);

    // Add barriers for BLAS
    if (desc->type == CGPU_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL)
    {
        const uint32_t STORE_COUNT = 4;
        SKR_DECLARE_ZERO_VLA(CGPUBufferBarrier, STORE, STORE_COUNT);
        const bool USE_STORE = desc->as_count < STORE_COUNT;

        CGPUBufferBarrier* BufferBarriers = USE_STORE ? STORE : cgpu_malloc(desc->as_count * sizeof(CGPUBufferBarrier));
        for (uint32_t i = 0; i < desc->as_count; i++)
        {
            CGPUAccelerationStructure_Vulkan* AS = (CGPUAccelerationStructure_Vulkan*)desc->as[i];
            if (AS->pASBuffer)
            {
                BufferBarriers[i].buffer = AS->pASBuffer;
                BufferBarriers[i].src_state = CGPU_RESOURCE_STATE_ACCELERATION_STRUCTURE_WRITE;
                BufferBarriers[i].dst_state = CGPU_RESOURCE_STATE_ACCELERATION_STRUCTURE_READ;
            }
        }
        CGPUResourceBarrierDescriptor barriers = { .buffer_barriers = BufferBarriers, .buffer_barriers_count = desc->as_count };
        cgpu_cmd_resource_barrier(cmd, &barriers);

        if (!USE_STORE)
            cgpu_free(BufferBarriers);
    }
}

inline static VkAccelerationStructureTypeKHR ToVkASType(ECGPUAccelerationStructureType type)
{
    return type == CGPU_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL ? VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR : VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
}

inline static VkBuildAccelerationStructureFlagsKHR ToVkBuildFlags(CGPUAccelerationStructureBuildFlags flags)
{
    VkBuildAccelerationStructureFlagsKHR ret = 0;
    if (flags & CGPU_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION)
        ret |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    if (flags & CGPU_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE)
        ret |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    if (flags & CGPU_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY)
        ret |= VK_BUILD_ACCELERATION_STRUCTURE_MOTION_BIT_NV; // No direct equivalent, using motion bit
    if (flags & CGPU_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE)
        ret |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    if (flags & CGPU_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD)
        ret |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
    if (flags & CGPU_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE)
        ret |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;

    return ret;
}

inline static VkGeometryFlagsKHR ToVkGeometryFlags(CGPUAccelerationStructureGeometryFlags flags)
{
    VkGeometryFlagsKHR ret = 0;
    if (flags & CGPU_ACCELERATION_STRUCTURE_GEOMETRY_FLAG_OPAQUE)
        ret |= VK_GEOMETRY_OPAQUE_BIT_KHR;
    if (flags & CGPU_ACCELERATION_STRUCTURE_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION)
        ret |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;

    return ret;
}

inline static VkGeometryInstanceFlagsKHR ToVkInstanceFlags(CGPUAccelerationStructureInstanceFlags flags)
{
    VkGeometryInstanceFlagsKHR ret = 0;
    if (flags & CGPU_ACCELERATION_STRUCTURE_INSTANCE_FLAG_FORCE_OPAQUE)
        ret |= VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
    if (flags & CGPU_ACCELERATION_STRUCTURE_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE)
        ret |= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    if (flags & CGPU_ACCELERATION_STRUCTURE_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE)
        ret |= VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR;
    if (flags & CGPU_ACCELERATION_STRUCTURE_INSTANCE_FLAG_FORCE_NON_OPAQUE)
        ret |= VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR;

    return ret;
}