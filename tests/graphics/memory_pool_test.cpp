// CGPU Memory Pool 测试用例
#include "SkrGraphics/api.h"
#include "SkrCore/log.h"
#include <vector>
#include <cstring>
#include <cstdio>

void test_linear_memory_pool(CGPUDeviceId device)
{
    // 创建一个 LINEAR 类型的内存池用于 GPU 专用资源
    CGPUMemoryPoolDescriptor poolDesc = {};
    poolDesc.type = CGPU_MEM_POOL_TYPE_LINEAR;
    poolDesc.memory_usage = CGPU_MEM_USAGE_GPU_ONLY;
    poolDesc.block_size = 64 * 1024 * 1024; // 64MB per block
    poolDesc.min_block_count = 1;
    poolDesc.max_block_count = 16;
    poolDesc.min_alloc_alignment = 256;
    
    CGPUMemoryPoolId gpuPool = cgpu_create_memory_pool(device, &poolDesc);
    if (!gpuPool)
    {
        SKR_LOG_ERROR(u8"Failed to create GPU memory pool!");
        return;
    }
    
    // 创建一个使用内存池的缓冲区
    CGPUBufferDescriptor bufferDesc = {};
    bufferDesc.size = 1024 * 1024; // 1MB
    bufferDesc.descriptors = CGPU_BUFFER_USAGE_VERTEX_BUFFER;
    bufferDesc.memory_usage = CGPU_MEM_USAGE_GPU_ONLY;
    bufferDesc.memory_pool = gpuPool; // 使用内存池
    bufferDesc.name = u8"Vertex Buffer from Pool";
    
    CGPUBufferId buffer = cgpu_create_buffer(device, &bufferDesc);
    if (!buffer)
    {
        SKR_LOG_ERROR(u8"Failed to create buffer from pool!");
        cgpu_free_memory_pool(gpuPool);
        return;
    }
    
    // 创建一个使用内存池的纹理
    CGPUTextureDescriptor texDesc = {};
    texDesc.width = 1024;
    texDesc.height = 1024;
    texDesc.depth = 1;
    texDesc.array_size = 1;
    texDesc.format = CGPU_FORMAT_R8G8B8A8_UNORM;
    texDesc.mip_levels = 1;
    texDesc.sample_count = CGPU_SAMPLE_COUNT_1;
    texDesc.descriptors = CGPU_RESOURCE_TYPE_TEXTURE;
    texDesc.memory_pool = gpuPool; // 使用内存池
    texDesc.name = u8"Texture from Pool";
    
    CGPUTextureId texture = cgpu_create_texture(device, &texDesc);
    if (!texture)
    {
        SKR_LOG_ERROR(u8"Failed to create texture from pool!");
        cgpu_free_buffer(buffer);
        cgpu_free_memory_pool(gpuPool);
        return;
    }
    
    SKR_LOG_INFO(u8"Successfully created buffer and texture from memory pool!");
    
    // 清理资源
    cgpu_free_texture(texture);
    cgpu_free_buffer(buffer);
    cgpu_free_memory_pool(gpuPool);
}

void test_upload_memory_pool(CGPUDeviceId device)
{
    // 创建一个用于上传资源的内存池
    CGPUMemoryPoolDescriptor poolDesc = {};
    poolDesc.type = CGPU_MEM_POOL_TYPE_LINEAR;
    poolDesc.memory_usage = CGPU_MEM_USAGE_CPU_TO_GPU;
    poolDesc.block_size = 16 * 1024 * 1024; // 16MB per block
    poolDesc.min_block_count = 2;
    poolDesc.max_block_count = 8;
    poolDesc.min_alloc_alignment = 256;
    
    CGPUMemoryPoolId uploadPool = cgpu_create_memory_pool(device, &poolDesc);
    if (!uploadPool)
    {
        SKR_LOG_ERROR(u8"Failed to create upload memory pool!");
        return;
    }
    
    // 创建多个上传缓冲区
    std::vector<CGPUBufferId> uploadBuffers;
    for (int i = 0; i < 5; ++i)
    {
        CGPUBufferDescriptor bufferDesc = {};
        bufferDesc.size = 256 * 1024; // 256KB each
        bufferDesc.descriptors = CGPU_RESOURCE_TYPE_NONE;
        bufferDesc.memory_usage = CGPU_MEM_USAGE_CPU_TO_GPU;
        bufferDesc.memory_pool = uploadPool; // 使用内存池
        bufferDesc.flags = CGPU_BUFFER_FLAG_PERSISTENT_MAP_BIT; // 持久映射
        
        char name[64];
        sprintf(name, "Upload Buffer %d", i);
        bufferDesc.name = (const char8_t*)name;
        
        CGPUBufferId buffer = cgpu_create_buffer(device, &bufferDesc);
        if (buffer)
        {
            uploadBuffers.push_back(buffer);
            
            // 可以立即使用映射的地址
            if (buffer->info->cpu_mapped_address)
            {
                // 写入一些数据
                memset(buffer->info->cpu_mapped_address, i, bufferDesc.size);
            }
        }
    }
    
    SKR_LOG_INFO(u8"Created %zu upload buffers from pool!", uploadBuffers.size());
    
    // 清理资源
    for (auto buffer : uploadBuffers)
    {
        cgpu_free_buffer(buffer);
    }
    cgpu_free_memory_pool(uploadPool);
}

int main()
{
    // 根据平台选择合适的后端
    ECGPUBackend preferred_backend = CGPU_BACKEND_VULKAN;
#ifdef _WIN32
    preferred_backend = CGPU_BACKEND_D3D12;
#elif defined(__APPLE__)
    preferred_backend = CGPU_BACKEND_METAL;
#endif

    // 初始化 CGPU
    CGPUInstanceDescriptor instDesc = {};
    instDesc.backend = preferred_backend;
    instDesc.enable_debug_layer = true;
    instDesc.enable_gpu_based_validation = false;
    instDesc.enable_set_name = true;
    
    CGPUInstanceId instance = cgpu_create_instance(&instDesc);
    if (!instance)
    {
        SKR_LOG_ERROR(u8"Failed to create CGPU instance with backend: %d", preferred_backend);
        
        // 尝试其他后端
        ECGPUBackend fallback_backends[] = { CGPU_BACKEND_VULKAN, CGPU_BACKEND_D3D12, CGPU_BACKEND_METAL };
        for (int i = 0; i < 3; ++i)
        {
            if (fallback_backends[i] != preferred_backend)
            {
                instDesc.backend = fallback_backends[i];
                instance = cgpu_create_instance(&instDesc);
                if (instance)
                {
                    SKR_LOG_INFO(u8"Using fallback backend: %d", fallback_backends[i]);
                    break;
                }
            }
        }
        
        if (!instance)
        {
            SKR_LOG_ERROR(u8"Failed to create CGPU instance with any backend!");
            return -1;
        }
    }
    
    // 枚举适配器
    uint32_t adapterCount = 0;
    cgpu_enum_adapters(instance, nullptr, &adapterCount);
    std::vector<CGPUAdapterId> adapters(adapterCount);
    cgpu_enum_adapters(instance, adapters.data(), &adapterCount);
    
    if (adapterCount == 0)
    {
        SKR_LOG_ERROR(u8"No GPU adapters found!");
        cgpu_free_instance(instance);
        return -1;
    }
    
    // 创建设备
    CGPUDeviceDescriptor devDesc = {};
    CGPUDeviceId device = cgpu_create_device(adapters[0], &devDesc);
    if (!device)
    {
        SKR_LOG_ERROR(u8"Failed to create device!");
        cgpu_free_instance(instance);
        return -1;
    }
    
    SKR_LOG_INFO(u8"=== CGPU Memory Pool Test ===");
    SKR_LOG_INFO(u8"Backend: %d", instance->backend);
    auto adapter_info = cgpu_query_adapter_detail(device->adapter);
    SKR_LOG_INFO(u8"Adapter: %s", adapter_info->vendor_preset.gpu_name);
    
    // 运行测试
    SKR_LOG_INFO(u8"--- Testing Linear Memory Pool for GPU Resources ---");
    test_linear_memory_pool(device);
    
    SKR_LOG_INFO(u8"--- Testing Upload Memory Pool ---");
    test_upload_memory_pool(device);
    
    SKR_LOG_INFO(u8"=== All tests completed successfully! ===");
    
    // 清理
    cgpu_free_device(device);
    cgpu_free_instance(instance);
    
    return 0;
}