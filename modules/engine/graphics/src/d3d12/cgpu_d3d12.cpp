// clang-format off
#include "SkrGraphics/containers.hpp"
#include "SkrGraphics/backend/d3d12/cgpu_d3d12.h"
#include "SkrGraphics/backend/d3d12/cgpu_d3d12_raytracing.h"
#include "../common/common_utils.h"
#include "d3d12_utils.h"

#if !defined(XBOX)
    #pragma comment(lib, "d3d12.lib")
    #pragma comment(lib, "dxgi.lib")
    #pragma comment(lib, "dxguid.lib")
#endif

#include <comdef.h>

// Call this only once.

CGPUInstanceId cgpu_create_instance_d3d12(CGPUInstanceDescriptor const* descriptor)
{
#if !defined(XBOX) && defined(_WIN32)
    D3D12Util_LoadDxcDLL();
#endif

    CGPUInstance_D3D12* result = cgpu_new<CGPUInstance_D3D12>();

    D3D12Util_InitializeEnvironment(&result->super);

    D3D12Util_Optionalenable_debug_layer(result, descriptor);

    UINT flags = 0;
    if (descriptor->enable_debug_layer) flags = DXGI_CREATE_FACTORY_DEBUG;
#if defined(XBOX)
#else
    if (SUCCEEDED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&result->pDXGIFactory))))
    {
        uint32_t gpuCount             = 0;
        bool     foundSoftwareAdapter = false;
        D3D12Util_QueryAllAdapters(result, &gpuCount, &foundSoftwareAdapter);
        // If the only adapter we found is a software adapter, log error message for QA
        if (!gpuCount && foundSoftwareAdapter)
        {
            cgpu_assert(0 && "The only available GPU has DXGI_ADAPTER_FLAG_SOFTWARE. Early exiting");
            return CGPU_NULLPTR;
        }
    }
    else
    {
        cgpu_assert("[D3D12 Fatal]: Create DXGIFactory2 Failed!");
    }
#endif
    return &result->super;
}

void cgpu_free_instance_d3d12(CGPUInstanceId instance)
{
    CGPUInstance_D3D12* to_destroy = (CGPUInstance_D3D12*)instance;
    D3D12Util_DeInitializeEnvironment(&to_destroy->super);
    if (to_destroy->mAdaptersCount > 0)
    {
        for (uint32_t i = 0; i < to_destroy->mAdaptersCount; i++)
        {
            SAFE_RELEASE(to_destroy->pAdapters[i].pDxActiveGPU);
        }
    }
    cgpu_free(to_destroy->pAdapters);
    SAFE_RELEASE(to_destroy->pDXGIFactory);
    if (to_destroy->pDXDebug)
    {
        SAFE_RELEASE(to_destroy->pDXDebug);
    }
    cgpu_delete(to_destroy);

#if !defined(XBOX) && defined(_WIN32)
    D3D12Util_UnloadDxcDLL();
#endif

#ifdef _DEBUG
    {
        IDXGIDebug1* dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        }
        SAFE_RELEASE(dxgiDebug);
    }
#endif
}

CGPUDeviceId cgpu_create_device_d3d12(CGPUAdapterId adapter, const CGPUDeviceDescriptor* desc)
{
    CGPUAdapter_D3D12*  A                          = (CGPUAdapter_D3D12*)adapter;
    CGPUInstance_D3D12* I                          = (CGPUInstance_D3D12*)A->super.instance;
    CGPUDevice_D3D12*   D                          = cgpu_new<CGPUDevice_D3D12>();
    *const_cast<CGPUAdapterId*>(&D->super.adapter) = adapter;

    if (!SUCCEEDED(D3D12CreateDevice(A->pDxActiveGPU, // default adapter
                                     A->mFeatureLevel, IID_PPV_ARGS(&D->pDxDevice))))
    {
        cgpu_assert("[D3D12 Fatal]: Create D3D12Device Failed!");
    }

    // Create Requested Queues.
    for (uint32_t i = 0u; i < desc->queue_group_count; i++)
    {
        const auto& queueGroup = desc->queue_groups[i];
        const auto  type       = queueGroup.queue_type;

        *const_cast<uint32_t*>(&D->pCommandQueueCounts[type]) = queueGroup.queue_count;
        *const_cast<ID3D12CommandQueue***>(&D->ppCommandQueues[type]) =
        (ID3D12CommandQueue**)cgpu_malloc(sizeof(ID3D12CommandQueue*) * queueGroup.queue_count);

        for (uint32_t j = 0u; j < queueGroup.queue_count; j++)
        {
            SKR_DECLARE_ZERO(D3D12_COMMAND_QUEUE_DESC, queueDesc)
            switch (type)
            {
                case CGPU_QUEUE_TYPE_GRAPHICS:
                    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                    break;
                case CGPU_QUEUE_TYPE_COMPUTE:
                    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
                    break;
                case CGPU_QUEUE_TYPE_TRANSFER:
                    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
                    break;
                default:
                    cgpu_assert(0 && "[D3D12 Fatal]: Unsupported ECGPUQueueType!");
                    return CGPU_NULLPTR;
            }
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            if (!SUCCEEDED(D->pDxDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&D->ppCommandQueues[type][j]))))
            {
                cgpu_assert("[D3D12 Fatal]: Create D3D12CommandQueue Failed!");
            }
        }
    }
    // Create D3D12MA Allocator
    D3D12Util_CreateDMAAllocator(I, A, D);
    cgpu_assert(D->pResourceAllocator && "DMA Allocator Must be Created!");

    // Create Tiled Memory Pool
    const auto               kTileSize = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    CGPUMemoryPoolDescriptor poolDesc  = {};
    poolDesc.type                      = CGPU_MEM_POOL_TYPE_TILED;
    poolDesc.memory_usage              = CGPU_MEM_USAGE_GPU_ONLY;
    poolDesc.min_alloc_alignment       = kTileSize;
    poolDesc.block_size                = kTileSize * 256;
    poolDesc.min_block_count           = 32;
    poolDesc.max_block_count           = 256;
    D->pTiledMemoryPool                = (CGPUTiledMemoryPool_D3D12*)cgpu_create_memory_pool_d3d12(&D->super, &poolDesc);
    if (A->mTiledResourceTier <= D3D12_TILED_RESOURCES_TIER_1)
    {
        const auto      kPageSize           = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
        D3D12_HEAP_DESC heapDesc            = {};
        heapDesc.Alignment                  = kPageSize;
        heapDesc.SizeInBytes                = kPageSize;
        heapDesc.Flags                      = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
        heapDesc.Properties.Type            = D3D12_HEAP_TYPE_DEFAULT;
        heapDesc.Properties.VisibleNodeMask = CGPU_SINGLE_GPU_NODE_MASK;
        auto hres                           = D->pDxDevice->CreateHeap(&heapDesc, IID_PPV_ARGS(&D->pUndefinedTileHeap));
        CHECK_HRESULT(hres);
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type                     = D3D12_COMMAND_LIST_TYPE_COPY;
        queueDesc.NodeMask                 = CGPU_SINGLE_GPU_NODE_MASK;
        hres                               = D->pDxDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&D->pUndefinedTileMappingQueue));
        CHECK_HRESULT(hres);
        D->pUndefinedTileMappingQueue->SetName(L"MappingQueue");
    }

    // Create Descriptor Heaps
    D->pCPUDescriptorHeaps = (D3D12Util_DescriptorHeap**)cgpu_malloc(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES * sizeof(D3D12Util_DescriptorHeap*));
    D->pCbvSrvUavHeaps     = (D3D12Util_DescriptorHeap**)cgpu_malloc(sizeof(D3D12Util_DescriptorHeap*));
    D->pSamplerHeaps       = (D3D12Util_DescriptorHeap**)cgpu_malloc(sizeof(D3D12Util_DescriptorHeap*));
    D->pNullDescriptors    = (CGPUEmptyDescriptors_D3D12*)cgpu_calloc(1, sizeof(CGPUEmptyDescriptors_D3D12));
    for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Flags                      = gCpuDescriptorHeapProperties[i].mFlags;
        desc.NodeMask                   = 0; // CPU Descriptor Heap - Node mask is irrelevant
        desc.NumDescriptors             = gCpuDescriptorHeapProperties[i].mMaxDescriptors;
        desc.Type                       = (D3D12_DESCRIPTOR_HEAP_TYPE)i;
#ifdef _DEBUG
        auto        sz      = D->pDxDevice->GetDescriptorHandleIncrementSize(desc.Type);
        const char* types[] = {
            "CBV_SRV_UAV",
            "SAMPLER",
            "RTV",
            "DSV",
        };
        cgpu_trace(u8"D3D12 descriptor heap type: %s, increment size: %d, Total allocated: %d(bytes)", types[desc.Type], sz, sz * desc.NumDescriptors);
#endif
        D3D12Util_CreateDescriptorHeap(D->pDxDevice, &desc, &D->pCPUDescriptorHeaps[i]);
    }
    // One shader visible heap for each linked node
    for (uint32_t i = 0; i < CGPU_SINGLE_GPU_NODE_COUNT; ++i)
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask                   = CGPU_SINGLE_GPU_NODE_MASK;

        desc.NumDescriptors = 1 << 16;
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        D3D12Util_CreateDescriptorHeap(D->pDxDevice, &desc, &D->pCbvSrvUavHeaps[i]);

        desc.NumDescriptors = 1 << 11;
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        D3D12Util_CreateDescriptorHeap(D->pDxDevice, &desc, &D->pSamplerHeaps[i]);
    }
    // Allocate NULL Descriptors
    {
        D->pNullDescriptors->Sampler   = D3D12_DESCRIPTOR_ID_NONE;
        D3D12_SAMPLER_DESC samplerDesc = {};
        samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        D->pNullDescriptors->Sampler                                       = D3D12Util_ConsumeDescriptorHandles(
                                       D->pCPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER], 1)
                                       .mCpu;
        D->pDxDevice->CreateSampler(&samplerDesc, D->pNullDescriptors->Sampler);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc  = {};
        srvDesc.Format                           = DXGI_FORMAT_R8_UINT;
        srvDesc.Shader4ComponentMapping          = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format                           = DXGI_FORMAT_R8_UINT;

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
        D3D12Util_CreateSRV(D, NULL, &srvDesc, &D->pNullDescriptors->TextureSRV[CGPU_TEX_DIMENSION_1D]);
        D3D12Util_CreateUAV(D, NULL, NULL, &uavDesc, &D->pNullDescriptors->TextureUAV[CGPU_TEX_DIMENSION_1D]);

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        D3D12Util_CreateSRV(D, NULL, &srvDesc, &D->pNullDescriptors->TextureSRV[CGPU_TEX_DIMENSION_2D]);
        D3D12Util_CreateUAV(D, NULL, NULL, &uavDesc, &D->pNullDescriptors->TextureUAV[CGPU_TEX_DIMENSION_2D]);

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
        D3D12Util_CreateSRV(D, NULL, &srvDesc, &D->pNullDescriptors->TextureSRV[CGPU_TEX_DIMENSION_2DMS]);

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        D3D12Util_CreateSRV(D, NULL, &srvDesc, &D->pNullDescriptors->TextureSRV[CGPU_TEX_DIMENSION_3D]);
        D3D12Util_CreateUAV(D, NULL, NULL, &uavDesc, &D->pNullDescriptors->TextureUAV[CGPU_TEX_DIMENSION_3D]);

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        D3D12Util_CreateSRV(D, NULL, &srvDesc, &D->pNullDescriptors->TextureSRV[CGPU_TEX_DIMENSION_CUBE]);

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
        D3D12Util_CreateSRV(D, NULL, &srvDesc, &D->pNullDescriptors->TextureSRV[CGPU_TEX_DIMENSION_1D_ARRAY]);
        D3D12Util_CreateUAV(D, NULL, NULL, &uavDesc, &D->pNullDescriptors->TextureUAV[CGPU_TEX_DIMENSION_1D_ARRAY]);

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        D3D12Util_CreateSRV(D, NULL, &srvDesc, &D->pNullDescriptors->TextureSRV[CGPU_TEX_DIMENSION_2D_ARRAY]);
        D3D12Util_CreateUAV(D, NULL, NULL, &uavDesc, &D->pNullDescriptors->TextureUAV[CGPU_TEX_DIMENSION_2D_ARRAY]);

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
        D3D12Util_CreateSRV(D, NULL, &srvDesc, &D->pNullDescriptors->TextureSRV[CGPU_TEX_DIMENSION_2DMS_ARRAY]);

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        D3D12Util_CreateSRV(D, NULL, &srvDesc, &D->pNullDescriptors->TextureSRV[CGPU_TEX_DIMENSION_CUBE_ARRAY]);

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        D3D12Util_CreateSRV(D, NULL, &srvDesc, &D->pNullDescriptors->BufferSRV);
        D3D12Util_CreateUAV(D, NULL, NULL, &uavDesc, &D->pNullDescriptors->BufferUAV);
        D3D12Util_CreateCBV(D, NULL, &D->pNullDescriptors->BufferCBV);
    }
    // pipeline cache
    D3D12_FEATURE_DATA_SHADER_CACHE feature = {};
    HRESULT result = D->pDxDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_CACHE, &feature, sizeof(feature));
    if (SUCCEEDED(result))
    {
        result = E_NOTIMPL;
        if (feature.SupportFlags & D3D12_SHADER_CACHE_SUPPORT_LIBRARY)
        {
            ID3D12Device1* device1 = NULL;
            result = D->pDxDevice->QueryInterface(IID_ARGS(&device1));
            if (SUCCEEDED(result))
            {
                result = device1->CreatePipelineLibrary(
                D->pPSOCacheData, 0, IID_ARGS(&D->pPipelineLibrary));
            }
            SAFE_RELEASE(device1);
        }
    }
    // for ray tracing
    result = D->pDxDevice->QueryInterface(IID_ARGS(&D->pDxDevice5));
    return &D->super;
}

void cgpu_free_device_d3d12(CGPUDeviceId device)
{
    CGPUDevice_D3D12* D = (CGPUDevice_D3D12*)device;
    for (uint32_t t = 0u; t < CGPU_QUEUE_TYPE_COUNT; t++)
    {
        for (uint32_t i = 0; i < D->pCommandQueueCounts[t]; i++)
        {
            SAFE_RELEASE(D->ppCommandQueues[t][i]);
        }
        cgpu_free((ID3D12CommandQueue**)D->ppCommandQueues[t]);
    }
    // Free Tiled Pool
    SAFE_RELEASE(D->pUndefinedTileHeap);
    SAFE_RELEASE(D->pUndefinedTileMappingQueue);
    cgpu_free_memory_pool_d3d12((CGPUMemoryPoolId)D->pTiledMemoryPool);
    // Free D3D12MA Allocator
    SAFE_RELEASE(D->pResourceAllocator);
    // Free Descriptor Heaps
    for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; i++)
    {
        D3D12Util_FreeDescriptorHeap(D->pCPUDescriptorHeaps[i]);
    }
    D3D12Util_FreeDescriptorHeap(D->pCbvSrvUavHeaps[0]);
    D3D12Util_FreeDescriptorHeap(D->pSamplerHeaps[0]);
    cgpu_free(D->pCPUDescriptorHeaps);
    cgpu_free(D->pCbvSrvUavHeaps);
    cgpu_free(D->pSamplerHeaps);
    cgpu_free(D->pNullDescriptors);
    SAFE_RELEASE(D->pDxDevice);
    SAFE_RELEASE(D->pPipelineLibrary);
    if (D->pPSOCacheData) cgpu_free(D->pPSOCacheData);
    SAFE_RELEASE(D->pDxDevice5);
    cgpu_delete(D);
}

// for example, shader register set: (s0t0) (s0b1) [s0b0[root]] (s1t0) (s1t1) {s2s0{static}}
// rootParams: |   s0    |   s1    |  [s0b0]  |
// rootRanges: | s0t0 | s0b1 | s1t0 | s1t1 |
// staticSamplers: |  s2s0  |   ...   |
CGPURootSignatureId cgpu_create_root_signature_d3d12(CGPUDeviceId device, const struct CGPURootSignatureDescriptor* desc)
{
    CGPUDevice_D3D12*        D  = (CGPUDevice_D3D12*)device;
    CGPURootSignature_D3D12* RS = cgpu_new<CGPURootSignature_D3D12>();
    // Pick root parameters from desc data
    CGPUShaderStages shaderStages = 0;
    for (uint32_t i = 0; i < desc->shader_count; i++)
    {
        CGPUShaderEntryDescriptor* shader_desc = desc->shaders + i;
        shaderStages |= shader_desc->stage;
    }
    // Pick shader reflection data
    CGPUUtil_InitRSParamTables((CGPURootSignature*)RS, desc);
    // [RS POOL] ALLOCATION
    if (desc->pool)
    {
        CGPURootSignature_D3D12* poolSig =
        (CGPURootSignature_D3D12*)CGPUUtil_TryAllocateSignature(desc->pool, &RS->super, desc);
        if (poolSig != CGPU_NULLPTR)
        {
            RS->mRootConstantParam = poolSig->mRootConstantParam;
            RS->pDxRootSignature   = poolSig->pDxRootSignature;
            RS->mRootParamIndex    = poolSig->mRootParamIndex;
            RS->super.pool         = desc->pool;
            RS->super.pool_sig     = &poolSig->super;
            return &RS->super;
        }
    }
    // [RS POOL] END ALLOCATION
    // Fill resource slots
    // Only support descriptor tables now
    // TODO: Support root CBVs
    //       Add backend sort for better performance
    const UINT tableCount     = RS->super.table_count;
    UINT       descRangeCount = 0;
    for (uint32_t i = 0; i < tableCount; i++)
    {
        descRangeCount += RS->super.tables[i].resources_count;
    }
    D3D12_ROOT_PARAMETER1* rootParams = (D3D12_ROOT_PARAMETER1*)cgpu_calloc(
    tableCount + RS->super.push_constant_count,
    sizeof(D3D12_ROOT_PARAMETER1));
    D3D12_DESCRIPTOR_RANGE1* cbvSrvUavRanges = (D3D12_DESCRIPTOR_RANGE1*)cgpu_calloc(descRangeCount, sizeof(D3D12_DESCRIPTOR_RANGE1));
    // Create descriptor tables
    UINT valid_root_tables = 0;
    for (uint32_t i_set = 0, i_range = 0; i_set < RS->super.table_count; i_set++)
    {
        CGPUParameterTable*    paramTable              = &RS->super.tables[i_set];
        D3D12_ROOT_PARAMETER1* rootParam               = &rootParams[valid_root_tables];
        rootParam->ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        CGPUShaderStages               visStages       = CGPU_SHADER_STAGE_NONE;
        const D3D12_DESCRIPTOR_RANGE1* descRangeCursor = &cbvSrvUavRanges[i_range];
        for (uint32_t i_register = 0; i_register < paramTable->resources_count; i_register++)
        {
            CGPUShaderResource* reflSlot = &paramTable->resources[i_register];
            visStages |= reflSlot->stages;
            {
                D3D12_DESCRIPTOR_RANGE1* descRange           = &cbvSrvUavRanges[i_range];
                descRange->RegisterSpace                     = reflSlot->set;
                descRange->BaseShaderRegister                = reflSlot->binding;
                descRange->Flags                             = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
                descRange->NumDescriptors                    = (reflSlot->type != CGPU_RESOURCE_TYPE_UNIFORM_BUFFER) ? reflSlot->size : 1;
                descRange->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                descRange->RangeType                         = D3D12Util_ResourceTypeToDescriptorRangeType(reflSlot->type);
                rootParam->DescriptorTable.NumDescriptorRanges++;
                i_range++;
            }
        }
        if (visStages != 0)
        {
            rootParam->ShaderVisibility                  = D3D12Util_TranslateShaderStages(visStages);
            rootParam->DescriptorTable.pDescriptorRanges = descRangeCursor;
            valid_root_tables++;
        }
    }
    // Root Const
    assert(RS->super.push_constant_count <= 1 && "Only support 1 push const now!");
    if (RS->super.push_constant_count)
    {
        auto reflSlot                                   = RS->super.push_constants;
        RS->mRootConstantParam.ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        RS->mRootConstantParam.Constants.RegisterSpace  = reflSlot->set;
        RS->mRootConstantParam.Constants.ShaderRegister = reflSlot->binding;
        RS->mRootConstantParam.Constants.Num32BitValues = reflSlot->size / sizeof(uint32_t);
        RS->mRootConstantParam.ShaderVisibility         = D3D12Util_TranslateShaderStages(reflSlot->stages);
    }
    // Create static samplers
    UINT                       staticSamplerCount = desc->static_sampler_count;
    D3D12_STATIC_SAMPLER_DESC* staticSamplerDescs = CGPU_NULLPTR;
    if (staticSamplerCount > 0)
    {
        staticSamplerDescs = (D3D12_STATIC_SAMPLER_DESC*)alloca(
        staticSamplerCount * sizeof(D3D12_STATIC_SAMPLER_DESC));
        for (uint32_t i = 0; i < RS->super.static_sampler_count; i++)
        {
            auto& RST_slot = RS->super.static_samplers[i];
            for (uint32_t j = 0; j < desc->static_sampler_count; j++)
            {
                auto input_slot = (CGPUSampler_D3D12*)desc->static_samplers[j];
                if (strcmp((const char*)RST_slot.name, (const char*)desc->static_sampler_names[j]) == 0)
                {
                    D3D12_SAMPLER_DESC& dxSamplerDesc    = input_slot->mDxDesc;
                    staticSamplerDescs[i].Filter         = dxSamplerDesc.Filter;
                    staticSamplerDescs[i].AddressU       = dxSamplerDesc.AddressU;
                    staticSamplerDescs[i].AddressV       = dxSamplerDesc.AddressV;
                    staticSamplerDescs[i].AddressW       = dxSamplerDesc.AddressW;
                    staticSamplerDescs[i].MipLODBias     = dxSamplerDesc.MipLODBias;
                    staticSamplerDescs[i].MaxAnisotropy  = dxSamplerDesc.MaxAnisotropy;
                    staticSamplerDescs[i].ComparisonFunc = dxSamplerDesc.ComparisonFunc;
                    staticSamplerDescs[i].MinLOD         = dxSamplerDesc.MinLOD;
                    staticSamplerDescs[i].MaxLOD         = dxSamplerDesc.MaxLOD;
                    staticSamplerDescs[i].BorderColor    = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;

                    CGPUShaderResource* samplerResource    = &RST_slot;
                    staticSamplerDescs[i].RegisterSpace    = samplerResource->set;
                    staticSamplerDescs[i].ShaderRegister   = samplerResource->binding;
                    staticSamplerDescs[i].ShaderVisibility = D3D12Util_TranslateShaderStages(samplerResource->stages);
                }
            }
        }
    }
    bool useInputLayout = shaderStages & CGPU_SHADER_STAGE_VERT; // VertexStage uses input layout
    // Fill RS flags
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    if (useInputLayout)
        rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    if (!(shaderStages & CGPU_SHADER_STAGE_VERT))
        rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
    if (!(shaderStages & CGPU_SHADER_STAGE_HULL))
        rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
    if (!(shaderStages & CGPU_SHADER_STAGE_DOMAIN))
        rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
    if (!(shaderStages & CGPU_SHADER_STAGE_GEOM))
        rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    if (!(shaderStages & CGPU_SHADER_STAGE_FRAG))
        rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
    // Serialize versioned RS
    const UINT paramCount = valid_root_tables + RS->super.push_constant_count /*must be 0 or 1 now*/;
    // Root Constant
    assert(RS->super.push_constant_count <= 1 && "Only support 1 push const now!");
    for (uint32_t i = 0; i < RS->super.push_constant_count; i++)
    {
        rootParams[valid_root_tables + i] = RS->mRootConstantParam;
        RS->mRootParamIndex               = valid_root_tables + i;
    }
    // Serialize root signature
    ID3DBlob* error               = NULL;
    ID3DBlob* rootSignatureString = NULL;
    SKR_DECLARE_ZERO(D3D12_VERSIONED_ROOT_SIGNATURE_DESC, sig_desc);
    sig_desc.Version                    = D3D_ROOT_SIGNATURE_VERSION_1_1;
    sig_desc.Desc_1_1.NumParameters     = paramCount;
    sig_desc.Desc_1_1.pParameters       = rootParams;
    sig_desc.Desc_1_1.NumStaticSamplers = staticSamplerCount;
    sig_desc.Desc_1_1.pStaticSamplers   = staticSamplerDescs;
    sig_desc.Desc_1_1.Flags             = rootSignatureFlags;
    HRESULT hres                        = D3D12SerializeVersionedRootSignature(&sig_desc, &rootSignatureString, &error);
    if (!SUCCEEDED(hres))
    {
        cgpu_error(u8"Failed to serialize root signature with error (%s)", (char*)error->GetBufferPointer());
    }
    // If running Linked Mode (SLI) create root signature for all nodes
    // #NOTE : In non SLI mode, mNodeCount will be 0 which sets nodeMask to
    // default value
    CHECK_HRESULT(D->pDxDevice->CreateRootSignature(
    CGPU_SINGLE_GPU_NODE_MASK,
    rootSignatureString->GetBufferPointer(),
    rootSignatureString->GetBufferSize(),
    IID_ARGS(&RS->pDxRootSignature)));
    cgpu_free(rootParams);
    cgpu_free(cbvSrvUavRanges);
    // [RS POOL] INSERTION
    if (desc->pool)
    {
        CGPURootSignatureId result = CGPUUtil_AddSignature(desc->pool, &RS->super, desc);
        cgpu_assert(result && u8"Root signature pool insertion failed!");
        return result;
    }
    // [RS POOL] END INSERTION
    return &RS->super;
}

void cgpu_free_root_signature_d3d12(CGPURootSignatureId signature)
{
    CGPURootSignature_D3D12* RS = (CGPURootSignature_D3D12*)signature;
    // [RS POOL] FREE
    if (signature->pool)
    {
        CGPUUtil_PoolFreeSignature(signature->pool, signature);
        return;
    }
    // [RS POOL] END FREE
    CGPUUtil_FreeRSParamTables((CGPURootSignature*)signature);
    SAFE_RELEASE(RS->pDxRootSignature);
    cgpu_delete(RS);
    return;
}

inline static uint32_t D3D12Util_ComputeNeededDescriptorCount(CGPUShaderResource* resource)
{
    if (resource->dim == CGPU_TEX_DIMENSION_1D_ARRAY ||
        resource->dim == CGPU_TEX_DIMENSION_2D_ARRAY ||
        resource->dim == CGPU_TEX_DIMENSION_2DMS_ARRAY ||
        resource->dim == CGPU_TEX_DIMENSION_CUBE_ARRAY)
        return resource->size;
    else
        return 1;
}

CGPUDescriptorSetId cgpu_create_descriptor_set_d3d12(CGPUDeviceId device, const struct CGPUDescriptorSetDescriptor* desc)
{
    CGPUDevice_D3D12*                D              = (CGPUDevice_D3D12*)device;
    const CGPURootSignature_D3D12*   RS             = (const CGPURootSignature_D3D12*)desc->root_signature;
    CGPUDescriptorSet_D3D12*         Set            = cgpu_new<CGPUDescriptorSet_D3D12>();
    const uint32_t                   nodeIndex      = CGPU_SINGLE_GPU_NODE_INDEX;
    struct D3D12Util_DescriptorHeap* pCbvSrvUavHeap = D->pCbvSrvUavHeaps[nodeIndex];
    struct D3D12Util_DescriptorHeap* pSamplerHeap   = D->pSamplerHeaps[nodeIndex];
    (void)pSamplerHeap;
    CGPUParameterTable* param_table    = &RS->super.tables[desc->set_index];
    uint32_t            CbvSrvUavCount = 0;
    uint32_t            SamplerCount   = 0;
    for (uint32_t i = 0; i < param_table->resources_count; i++)
    {
        if (param_table->resources[i].type == CGPU_RESOURCE_TYPE_SAMPLER)
        {
            SamplerCount++;
        }
        else if (param_table->resources[i].type == CGPU_RESOURCE_TYPE_TEXTURE ||
                 param_table->resources[i].type == CGPU_RESOURCE_TYPE_RW_TEXTURE ||
                 param_table->resources[i].type == CGPU_RESOURCE_TYPE_BUFFER ||
                 param_table->resources[i].type == CGPU_RESOURCE_TYPE_BUFFER_RAW ||
                 param_table->resources[i].type == CGPU_RESOURCE_TYPE_RW_BUFFER ||
                 param_table->resources[i].type == CGPU_RESOURCE_TYPE_RW_BUFFER_RAW ||
                 param_table->resources[i].type == CGPU_RESOURCE_TYPE_TEXTURE_CUBE ||
                 param_table->resources[i].type == CGPU_RESOURCE_TYPE_UNIFORM_BUFFER)
        {
            CbvSrvUavCount += D3D12Util_ComputeNeededDescriptorCount(&param_table->resources[i]);
        }
    }
    // CBV/SRV/UAV
    Set->mCbvSrvUavHandle = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    Set->mSamplerHandle   = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    if (CbvSrvUavCount)
    {
        auto StartHandle      = D3D12Util_ConsumeDescriptorHandles(pCbvSrvUavHeap, CbvSrvUavCount);
        Set->mCbvSrvUavHandle = StartHandle.mGpu.ptr - pCbvSrvUavHeap->mStartHandle.mGpu.ptr;
        Set->mCbvSrvUavStride = CbvSrvUavCount * pCbvSrvUavHeap->mDescriptorSize;
    }
    if (SamplerCount)
    {
        auto StartHandle    = D3D12Util_ConsumeDescriptorHandles(pSamplerHeap, SamplerCount);
        Set->mSamplerHandle = StartHandle.mGpu.ptr - pSamplerHeap->mStartHandle.mGpu.ptr;
        Set->mSamplerStride = SamplerCount * pSamplerHeap->mDescriptorSize;
    }
    // Bind NULL handles on creation
    if (CbvSrvUavCount || SamplerCount)
    {
        uint32_t CbvSrvUavHeapOffset = 0;
        uint32_t SamplerHeapOffset   = 0;
        for (uint32_t i = 0; i < param_table->resources_count; ++i)
        {
            const auto dimension        = param_table->resources[i].dim;
            auto       srcHandle        = D3D12_DESCRIPTOR_ID_NONE;
            auto       srcSamplerHandle = D3D12_DESCRIPTOR_ID_NONE;
            switch (param_table->resources[i].type)
            {
                case CGPU_RESOURCE_TYPE_TEXTURE:
                    srcHandle = D->pNullDescriptors->TextureSRV[dimension];
                    break;
                case CGPU_RESOURCE_TYPE_BUFFER:
                    srcHandle = D->pNullDescriptors->BufferSRV;
                    break;
                case CGPU_RESOURCE_TYPE_RW_BUFFER:
                    srcHandle = D->pNullDescriptors->BufferUAV;
                    break;
                case CGPU_RESOURCE_TYPE_UNIFORM_BUFFER:
                    srcHandle = D->pNullDescriptors->BufferCBV;
                    break;
                case CGPU_RESOURCE_TYPE_SAMPLER:
                    srcSamplerHandle = D->pNullDescriptors->Sampler;
                    break;
                default:
                    break;
            }
            if (srcHandle.ptr != D3D12_DESCRIPTOR_ID_NONE.ptr)
            {
                for (uint32_t j = 0; j < param_table->resources[i].size; j++)
                {
                    D3D12Util_CopyDescriptorHandle(pCbvSrvUavHeap, srcHandle, Set->mCbvSrvUavHandle, CbvSrvUavHeapOffset);
                    CbvSrvUavHeapOffset++;
                }
            }
            if (srcSamplerHandle.ptr != D3D12_DESCRIPTOR_ID_NONE.ptr)
            {
                for (uint32_t j = 0; j < param_table->resources[i].size; j++)
                {
                    D3D12Util_CopyDescriptorHandle(pSamplerHeap, srcSamplerHandle, Set->mSamplerHandle, SamplerHeapOffset);
                    SamplerHeapOffset++;
                }
            }
        }
    }
    // TODO: Support root descriptors
    return &Set->super;
}

void cgpu_update_descriptor_set_d3d12(CGPUDescriptorSetId set, const struct CGPUDescriptorData* datas, uint32_t count)
{
    CGPUDescriptorSet_D3D12*         Set            = (CGPUDescriptorSet_D3D12*)set;
    const CGPURootSignature_D3D12*   RS             = (const CGPURootSignature_D3D12*)set->root_signature;
    CGPUDevice_D3D12*                D              = (CGPUDevice_D3D12*)set->root_signature->device;
    CGPUParameterTable*              ParamTable     = &RS->super.tables[set->index];
    const uint32_t                   nodeIndex      = CGPU_SINGLE_GPU_NODE_INDEX;
    struct D3D12Util_DescriptorHeap* pCbvSrvUavHeap = D->pCbvSrvUavHeaps[nodeIndex];
    struct D3D12Util_DescriptorHeap* pSamplerHeap   = D->pSamplerHeaps[nodeIndex];
    for (uint32_t i = 0; i < count; i++)
    {
        // Descriptor Info
        const CGPUDescriptorData* pParam     = datas + i;
        CGPUShaderResource*       ResData    = CGPU_NULLPTR;
        uint32_t                  HeapOffset = 0;
        if (pParam->name != CGPU_NULLPTR)
        {
            size_t argNameHash = cgpu_name_hash(pParam->name, strlen((const char*)pParam->name));
            for (uint32_t j = 0; j < ParamTable->resources_count; j++)
            {
                if (ParamTable->resources[j].name_hash == argNameHash &&
                    ::strcmp((const char*)ParamTable->resources[j].name, (const char*)pParam->name) == 0)
                {
                    ResData = ParamTable->resources + j;
                    break;
                }
                HeapOffset += D3D12Util_ComputeNeededDescriptorCount(&ParamTable->resources[j]);
            }
        }
        else
        {
            for (uint32_t j = 0; j < ParamTable->resources_count; j++)
            {
                if (ParamTable->resources[j].type == pParam->binding_type &&
                    ParamTable->resources[j].binding == pParam->binding)
                {
                    ResData = &ParamTable->resources[j];
                    break;
                }
                HeapOffset += D3D12Util_ComputeNeededDescriptorCount(&ParamTable->resources[j]);
            }
        }
        // Update Info
        const uint32_t arrayCount = cgpu_max(1U, pParam->count);
        switch (ResData->type)
        {
            case CGPU_RESOURCE_TYPE_SAMPLER: {
                cgpu_assert(pParam->samplers && "cgpu_assert: Binding NULL Sampler(s)!");
                const CGPUSampler_D3D12** Samplers = (const CGPUSampler_D3D12**)pParam->samplers;
                for (uint32_t arr = 0; arr < arrayCount; arr++)
                {
                    cgpu_assert(pParam->samplers[arr] && "cgpu_assert: Binding NULL Sampler!");
                    D3D12Util_CopyDescriptorHandle(pSamplerHeap,
                                                   { Samplers[arr]->mDxHandle.ptr },
                                                   Set->mSamplerHandle, arr + HeapOffset);
                }
            }
            break;
            case CGPU_RESOURCE_TYPE_TEXTURE:
            case CGPU_RESOURCE_TYPE_TEXTURE_CUBE: {
                cgpu_assert(pParam->textures && "cgpu_assert: Binding NULL Textures(s)!");
                const CGPUTextureView_D3D12** Textures = (const CGPUTextureView_D3D12**)pParam->textures;
                for (uint32_t arr = 0; arr < arrayCount; arr++)
                {
                    cgpu_assert(pParam->textures[arr] && "cgpu_assert: Binding NULL Textures!");
                    D3D12Util_CopyDescriptorHandle(pCbvSrvUavHeap,
                                                   { Textures[arr]->mDxDescriptorHandles.ptr + Textures[arr]->mDxSrvOffset },
                                                   Set->mCbvSrvUavHandle, arr + HeapOffset);
                }
            }
            break;
            case CGPU_RESOURCE_TYPE_BUFFER:
            case CGPU_RESOURCE_TYPE_BUFFER_RAW: {
                cgpu_assert(pParam->buffers && "cgpu_assert: Binding NULL Buffer(s)!");
                const CGPUBuffer_D3D12** Buffers = (const CGPUBuffer_D3D12**)pParam->buffers;
                for (uint32_t arr = 0; arr < arrayCount; arr++)
                {
                    cgpu_assert(pParam->buffers[arr] && "cgpu_assert: Binding NULL Buffer!");
                    D3D12Util_CopyDescriptorHandle(pCbvSrvUavHeap,
                                                   { Buffers[arr]->mDxDescriptorHandles.ptr + Buffers[arr]->mDxSrvOffset },
                                                   Set->mCbvSrvUavHandle, arr + HeapOffset);
                }
            }
            break;
            case CGPU_RESOURCE_TYPE_UNIFORM_BUFFER: {
                cgpu_assert(pParam->buffers && "cgpu_assert: Binding NULL Buffer(s)!");
                const CGPUBuffer_D3D12** Buffers = (const CGPUBuffer_D3D12**)pParam->buffers;
                for (uint32_t arr = 0; arr < arrayCount; arr++)
                {
                    cgpu_assert(pParam->buffers[arr] && "cgpu_assert: Binding NULL Buffer!");
                    D3D12Util_CopyDescriptorHandle(pCbvSrvUavHeap,
                                                   { Buffers[arr]->mDxDescriptorHandles.ptr },
                                                   Set->mCbvSrvUavHandle, arr + HeapOffset);
                }
            }
            break;
            case CGPU_RESOURCE_TYPE_RW_TEXTURE: {
                cgpu_assert(pParam->textures && "cgpu_assert: Binding NULL Texture(s)!");
                const CGPUTextureView_D3D12** Textures = (const CGPUTextureView_D3D12**)pParam->textures;
                for (uint32_t arr = 0; arr < arrayCount; arr++)
                {
                    cgpu_assert(pParam->textures[arr] && "cgpu_assert: Binding NULL Texture!");
                    D3D12Util_CopyDescriptorHandle(pCbvSrvUavHeap,
                                                   { Textures[arr]->mDxDescriptorHandles.ptr + Textures[arr]->mDxUavOffset },
                                                   Set->mCbvSrvUavHandle, arr + HeapOffset);
                }
            }
            break;
            case CGPU_RESOURCE_TYPE_RW_BUFFER:
            case CGPU_RESOURCE_TYPE_RW_BUFFER_RAW: {
                cgpu_assert(pParam->buffers && "cgpu_assert: Binding NULL Buffer(s)!");
                const CGPUBuffer_D3D12** Buffers = (const CGPUBuffer_D3D12**)pParam->buffers;
                for (uint32_t arr = 0; arr < arrayCount; arr++)
                {
                    cgpu_assert(pParam->buffers[arr] && "cgpu_assert: Binding NULL Buffer!");
                    D3D12Util_CopyDescriptorHandle(pCbvSrvUavHeap,
                                                   { Buffers[arr]->mDxDescriptorHandles.ptr + Buffers[arr]->mDxUavOffset },
                                                   Set->mCbvSrvUavHandle, arr + HeapOffset);
                }
            }
            break;
            case CGPU_RESOURCE_TYPE_RAY_TRACING: {
                cgpu_assert(pParam->acceleration_structures && "NULL Acceleration Structure(s)");
                CGPUAccelerationStructure_D3D12* pAccel = (CGPUAccelerationStructure_D3D12*)pParam->acceleration_structures[0];
                const CGPUBuffer_D3D12* pASBuffer = (const CGPUBuffer_D3D12*)pAccel->pASBuffer;
                D3D12Util_CopyDescriptorHandle(pCbvSrvUavHeap, 
                                                { pASBuffer->mDxDescriptorHandles.ptr + pASBuffer->mDxSrvOffset }, 
                                                Set->mCbvSrvUavHandle, HeapOffset);
                Set->pBoundAccel = pAccel;
            }
            break;
            default:
                break;
        }
    }
}

void cgpu_free_descriptor_set_d3d12(CGPUDescriptorSetId set)
{
    CGPUDevice_D3D12*        D   = (CGPUDevice_D3D12*)set->root_signature->device;
    CGPUDescriptorSet_D3D12* Set = (CGPUDescriptorSet_D3D12*)set;
    (void)D; // TODO: recycle of descriptor set heap handles
    cgpu_delete(Set);
}

CGPUComputePipelineId cgpu_create_compute_pipeline_d3d12(CGPUDeviceId device, const struct CGPUComputePipelineDescriptor* desc)
{
    CGPUDevice_D3D12*          D   = (CGPUDevice_D3D12*)device;
    CGPUComputePipeline_D3D12* PPL = cgpu_new<CGPUComputePipeline_D3D12>();
    CGPURootSignature_D3D12*   RS  = (CGPURootSignature_D3D12*)desc->root_signature;
    CGPUShaderLibrary_D3D12*   SL  = (CGPUShaderLibrary_D3D12*)desc->compute_shader->library;
    PPL->pRootSignature            = RS->pDxRootSignature;
    // Add pipeline specifying its for compute purposes
    SKR_DECLARE_ZERO(D3D12_SHADER_BYTECODE, CS);
    CS.BytecodeLength  = SL->pShaderBlob->GetBufferSize();
    CS.pShaderBytecode = SL->pShaderBlob->GetBufferPointer();
    SKR_DECLARE_ZERO(D3D12_CACHED_PIPELINE_STATE, cached_pso_desc);
    cached_pso_desc.pCachedBlob           = NULL;
    cached_pso_desc.CachedBlobSizeInBytes = 0;
    SKR_DECLARE_ZERO(D3D12_COMPUTE_PIPELINE_STATE_DESC, pipeline_state_desc);
    pipeline_state_desc.pRootSignature = RS->pDxRootSignature;
    pipeline_state_desc.CS             = CS;
    pipeline_state_desc.CachedPSO      = cached_pso_desc;
    pipeline_state_desc.Flags          = D3D12_PIPELINE_STATE_FLAG_NONE;
    pipeline_state_desc.NodeMask       = CGPU_SINGLE_GPU_NODE_MASK;
    // Pipeline cache
    HRESULT result                        = E_FAIL;
    wchar_t pipelineName[PSO_NAME_LENGTH] = {};
    size_t  psoShaderHash                 = 0;
    size_t  psoComputeHash                = 0;
    if (D->pPipelineLibrary)
    {
        if (CS.BytecodeLength)
            psoShaderHash = cgpu_hash(CS.pShaderBytecode, CS.BytecodeLength, psoShaderHash);
        psoComputeHash = cgpu_hash(&pipeline_state_desc.Flags, sizeof(D3D12_PIPELINE_STATE_FLAGS), psoComputeHash);
        psoComputeHash = cgpu_hash(&pipeline_state_desc.NodeMask, sizeof(UINT), psoComputeHash);
        swprintf(pipelineName, PSO_NAME_LENGTH, L"%S_S%zuR%zu", "COMPUTEPSO", psoShaderHash, psoComputeHash);
        result = D->pPipelineLibrary->LoadComputePipeline(pipelineName,
                                                          &pipeline_state_desc, IID_ARGS(&PPL->pDxPipelineState));
    }
    if (!SUCCEEDED(result))
    {
        // XBOX: Support PSO extensions
        CHECK_HRESULT(D->pDxDevice->CreateComputePipelineState(
        &pipeline_state_desc, IID_PPV_ARGS(&PPL->pDxPipelineState)));
    }
    return &PPL->super;
}

void cgpu_free_compute_pipeline_d3d12(CGPUComputePipelineId pipeline)
{
    CGPUComputePipeline_D3D12* PPL = (CGPUComputePipeline_D3D12*)pipeline;
    SAFE_RELEASE(PPL->pDxPipelineState);
    cgpu_delete(PPL);
}

static const char*       kD3D12PSOMemoryPoolName = "cgpu::d3d12_pso";
D3D12_DEPTH_STENCIL_DESC gDefaultDepthDesc       = {};
D3D12_BLEND_DESC         gDefaultBlendDesc       = {};
D3D12_RASTERIZER_DESC    gDefaultRasterizerDesc  = {};
CGPURenderPipelineId cgpu_create_render_pipeline_d3d12(CGPUDeviceId device, const struct CGPURenderPipelineDescriptor* desc)
{
    CGPUDevice_D3D12*        D  = (CGPUDevice_D3D12*)device;
    CGPURootSignature_D3D12* RS = (CGPURootSignature_D3D12*)desc->root_signature;

    uint32_t input_elem_count = 0;
    if (desc->vertex_layout != nullptr)
    {
        for (uint32_t attrib_index = 0; attrib_index < desc->vertex_layout->attribute_count; ++attrib_index)
        {
            const CGPUVertexAttribute* attrib = &(desc->vertex_layout->attributes[attrib_index]);
            for (uint32_t arr_index = 0; arr_index < attrib->array_size; arr_index++)
            {
                input_elem_count++;
            }
        }
    }
    uint64_t       dsize                 = sizeof(CGPURenderPipeline_D3D12);
    const uint64_t input_elements_offset = dsize;
    dsize += (sizeof(D3D12_INPUT_ELEMENT_DESC) * input_elem_count);

    auto                      ptr            = (uint8_t*)cgpu_callocN(1, dsize, kD3D12PSOMemoryPoolName);
    CGPURenderPipeline_D3D12* PPL            = cgpu_new_placed<CGPURenderPipeline_D3D12>(ptr);
    D3D12_INPUT_ELEMENT_DESC* input_elements = (D3D12_INPUT_ELEMENT_DESC*)(ptr + input_elements_offset);

    // Vertex input state
    if (desc->vertex_layout != nullptr)
    {
        cgpu::Map<cgpu::String, uint32_t> semanticIndexMap = {};
        uint32_t                          fill_index       = 0;
        for (uint32_t attrib_index = 0; attrib_index < desc->vertex_layout->attribute_count; ++attrib_index)
        {
            const CGPUVertexAttribute* attrib = &(desc->vertex_layout->attributes[attrib_index]);
            for (uint32_t arr_index = 0; arr_index < attrib->array_size; arr_index++)
            {
                input_elements[fill_index].SemanticName = (const char*)attrib->semantic_name;
                auto found                              = semanticIndexMap.find(attrib->semantic_name);
                if (found)
                    found.value() += 1;
                else
                    semanticIndexMap.add(attrib->semantic_name, 0);
                found                                        = semanticIndexMap.find(attrib->semantic_name);
                input_elements[fill_index].SemanticIndex     = found.value();
                input_elements[fill_index].Format            = DXGIUtil_TranslatePixelFormat(attrib->format, false);
                input_elements[fill_index].InputSlot         = attrib->binding;
                input_elements[fill_index].AlignedByteOffset = attrib->offset + arr_index * FormatUtil_BitSizeOfBlock(attrib->format) / 8;
                if (attrib->rate == CGPU_INPUT_RATE_INSTANCE)
                {
                    input_elements[fill_index].InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                    input_elements[fill_index].InstanceDataStepRate = 1;
                }
                else
                {
                    input_elements[fill_index].InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                    input_elements[fill_index].InstanceDataStepRate = 0;
                }
                fill_index++;
            }
        }
    }
    SKR_DECLARE_ZERO(D3D12_INPUT_LAYOUT_DESC, input_layout_desc);
    input_layout_desc.pInputElementDescs = input_elem_count ? input_elements : NULL;
    input_layout_desc.NumElements        = input_elem_count;
    // Shader stages
    SKR_DECLARE_ZERO(D3D12_SHADER_BYTECODE, VS);
    SKR_DECLARE_ZERO(D3D12_SHADER_BYTECODE, PS);
    SKR_DECLARE_ZERO(D3D12_SHADER_BYTECODE, DS);
    SKR_DECLARE_ZERO(D3D12_SHADER_BYTECODE, HS);
    SKR_DECLARE_ZERO(D3D12_SHADER_BYTECODE, GS);
    for (uint32_t i = 0; i < 5; ++i)
    {
        ECGPUShaderStage stage_mask = (ECGPUShaderStage)(1 << i);
        switch (stage_mask)
        {
            case CGPU_SHADER_STAGE_VERT: {
                if (desc->vertex_shader)
                {
                    CGPUShaderLibrary_D3D12* VertLib = (CGPUShaderLibrary_D3D12*)desc->vertex_shader->library;
                    VS.BytecodeLength                = VertLib->pShaderBlob->GetBufferSize();
                    VS.pShaderBytecode               = VertLib->pShaderBlob->GetBufferPointer();
                }
            }
            break;
            case CGPU_SHADER_STAGE_TESC: {
                if (desc->tesc_shader)
                {
                    CGPUShaderLibrary_D3D12* TescLib = (CGPUShaderLibrary_D3D12*)desc->tesc_shader->library;
                    HS.BytecodeLength                = TescLib->pShaderBlob->GetBufferSize();
                    HS.pShaderBytecode               = TescLib->pShaderBlob->GetBufferPointer();
                }
            }
            break;
            case CGPU_SHADER_STAGE_TESE: {
                if (desc->tese_shader)
                {
                    CGPUShaderLibrary_D3D12* TeseLib = (CGPUShaderLibrary_D3D12*)desc->tese_shader->library;
                    DS.BytecodeLength                = TeseLib->pShaderBlob->GetBufferSize();
                    DS.pShaderBytecode               = TeseLib->pShaderBlob->GetBufferPointer();
                }
            }
            break;
            case CGPU_SHADER_STAGE_GEOM: {
                if (desc->geom_shader)
                {
                    CGPUShaderLibrary_D3D12* GeomLib = (CGPUShaderLibrary_D3D12*)desc->geom_shader->library;
                    GS.BytecodeLength                = GeomLib->pShaderBlob->GetBufferSize();
                    GS.pShaderBytecode               = GeomLib->pShaderBlob->GetBufferPointer();
                }
            }
            break;
            case CGPU_SHADER_STAGE_FRAG: {
                if (desc->fragment_shader)
                {
                    CGPUShaderLibrary_D3D12* FragLib = (CGPUShaderLibrary_D3D12*)desc->fragment_shader->library;
                    PS.BytecodeLength                = FragLib->pShaderBlob->GetBufferSize();
                    PS.pShaderBytecode               = FragLib->pShaderBlob->GetBufferPointer();
                }
            }
            break;
            default:
                cgpu_assert(false && "Shader Stage not supported!");
                break;
        }
    }
    // Stream out
    SKR_DECLARE_ZERO(D3D12_STREAM_OUTPUT_DESC, stream_output_desc);
    stream_output_desc.pSODeclaration   = NULL;
    stream_output_desc.NumEntries       = 0;
    stream_output_desc.pBufferStrides   = NULL;
    stream_output_desc.NumStrides       = 0;
    stream_output_desc.RasterizedStream = 0;
    // Sample
    SKR_DECLARE_ZERO(DXGI_SAMPLE_DESC, sample_desc);
    sample_desc.Count   = (UINT)(desc->sample_count ? desc->sample_count : 1);
    sample_desc.Quality = (UINT)(desc->sample_quality);
    SKR_DECLARE_ZERO(D3D12_CACHED_PIPELINE_STATE, cached_pso_desc);
    cached_pso_desc.pCachedBlob           = NULL;
    cached_pso_desc.CachedBlobSizeInBytes = 0;
    // Fill pipeline object desc
    PPL->mDxGfxPipelineStateDesc.pRootSignature = RS->pDxRootSignature;
    // Single GPU
    PPL->mDxGfxPipelineStateDesc.NodeMask        = CGPU_SINGLE_GPU_NODE_MASK;
    PPL->mDxGfxPipelineStateDesc.VS              = VS;
    PPL->mDxGfxPipelineStateDesc.PS              = PS;
    PPL->mDxGfxPipelineStateDesc.DS              = DS;
    PPL->mDxGfxPipelineStateDesc.HS              = HS;
    PPL->mDxGfxPipelineStateDesc.GS              = GS;
    PPL->mDxGfxPipelineStateDesc.StreamOutput    = stream_output_desc;
    PPL->mDxGfxPipelineStateDesc.BlendState      = desc->blend_state ? D3D12Util_TranslateBlendState(desc->blend_state) : gDefaultBlendDesc;
    PPL->mDxGfxPipelineStateDesc.SampleMask      = UINT_MAX;
    PPL->mDxGfxPipelineStateDesc.RasterizerState = desc->rasterizer_state ? D3D12Util_TranslateRasterizerState(desc->rasterizer_state) : gDefaultRasterizerDesc;
    // Depth stencil
    PPL->mDxGfxPipelineStateDesc.DepthStencilState     = desc->depth_state ? D3D12Util_TranslateDephStencilState(desc->depth_state) : gDefaultDepthDesc;
    PPL->mDxGfxPipelineStateDesc.InputLayout           = input_layout_desc;
    PPL->mDxGfxPipelineStateDesc.IBStripCutValue       = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    PPL->mDxGfxPipelineStateDesc.PrimitiveTopologyType = D3D12Util_TranslatePrimitiveTopology(desc->prim_topology);
    PPL->mDxGfxPipelineStateDesc.NumRenderTargets      = desc->render_target_count;
    PPL->mDxGfxPipelineStateDesc.DSVFormat             = DXGIUtil_TranslatePixelFormat(desc->depth_stencil_format, false);
    PPL->mDxGfxPipelineStateDesc.SampleDesc            = sample_desc;
    PPL->mDxGfxPipelineStateDesc.CachedPSO             = cached_pso_desc;
    PPL->mDxGfxPipelineStateDesc.Flags                 = D3D12_PIPELINE_STATE_FLAG_NONE;
    for (uint32_t i = 0; i < PPL->mDxGfxPipelineStateDesc.NumRenderTargets; ++i)
    {
        PPL->mDxGfxPipelineStateDesc.RTVFormats[i] = DXGIUtil_TranslatePixelFormat(desc->color_formats[i], false);
    }
    // Create pipeline object
    HRESULT result                        = E_FAIL;
    wchar_t pipelineName[PSO_NAME_LENGTH] = {};
    size_t  psoShaderHash                 = 0;
    size_t  psoRenderHash                 = 0;
    size_t  rootSignatureNumber           = (size_t)RS->pDxRootSignature;
    if (D->pPipelineLibrary)
    {
        // Calculate graphics pso shader hash
        if (VS.BytecodeLength)
            psoShaderHash = cgpu_hash(VS.pShaderBytecode, VS.BytecodeLength, psoShaderHash);
        if (PS.BytecodeLength)
            psoShaderHash = cgpu_hash(PS.pShaderBytecode, PS.BytecodeLength, psoShaderHash);
        if (DS.BytecodeLength)
            psoShaderHash = cgpu_hash(DS.pShaderBytecode, DS.BytecodeLength, psoShaderHash);
        if (HS.BytecodeLength)
            psoShaderHash = cgpu_hash(HS.pShaderBytecode, HS.BytecodeLength, psoShaderHash);
        if (GS.BytecodeLength)
            psoShaderHash = cgpu_hash(GS.pShaderBytecode, GS.BytecodeLength, psoShaderHash);

        // Calculate graphics pso desc hash
        psoRenderHash = cgpu_hash(&PPL->mDxGfxPipelineStateDesc.BlendState, sizeof(D3D12_BLEND_DESC), psoRenderHash);
        psoRenderHash = cgpu_hash(&PPL->mDxGfxPipelineStateDesc.RasterizerState, sizeof(D3D12_RASTERIZER_DESC), psoRenderHash);
        psoRenderHash = cgpu_hash(&PPL->mDxGfxPipelineStateDesc.DepthStencilState, sizeof(D3D12_DEPTH_STENCIL_DESC), psoRenderHash);
        psoRenderHash = cgpu_hash(&PPL->mDxGfxPipelineStateDesc.IBStripCutValue, sizeof(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE), psoRenderHash);
        psoRenderHash = cgpu_hash(PPL->mDxGfxPipelineStateDesc.RTVFormats,
                                  PPL->mDxGfxPipelineStateDesc.NumRenderTargets * sizeof(DXGI_FORMAT), psoRenderHash);
        psoRenderHash = cgpu_hash(&PPL->mDxGfxPipelineStateDesc.DSVFormat, sizeof(DXGI_FORMAT), psoRenderHash);
        psoRenderHash = cgpu_hash(&PPL->mDxGfxPipelineStateDesc.SampleDesc, sizeof(DXGI_SAMPLE_DESC), psoRenderHash);
        psoRenderHash = cgpu_hash(&PPL->mDxGfxPipelineStateDesc.Flags, sizeof(D3D12_PIPELINE_STATE_FLAGS), psoRenderHash);
        for (uint32_t i = 0; i < PPL->mDxGfxPipelineStateDesc.InputLayout.NumElements; i++)
        {
            psoRenderHash = cgpu_hash(&PPL->mDxGfxPipelineStateDesc.InputLayout.pInputElementDescs[i],
                                      sizeof(D3D12_INPUT_ELEMENT_DESC), psoRenderHash);
        }
        swprintf(pipelineName, PSO_NAME_LENGTH, L"%S_S%zu_R%zu_RS%zu", "GRAPHICSPSO", psoShaderHash, psoRenderHash, rootSignatureNumber);
        result = D->pPipelineLibrary->LoadGraphicsPipeline(pipelineName, &PPL->mDxGfxPipelineStateDesc, IID_ARGS(&PPL->pDxPipelineState));
    }
    if (!SUCCEEDED(result))
    {
        CHECK_HRESULT(D->pDxDevice->CreateGraphicsPipelineState(&PPL->mDxGfxPipelineStateDesc, IID_PPV_ARGS(&PPL->pDxPipelineState)));
        // Pipeline cache
        if (D->pPipelineLibrary)
        {
            result = D->pPipelineLibrary->StorePipeline(pipelineName, PPL->pDxPipelineState);
            if (!SUCCEEDED(result))
            {
                cgpu_warn(u8"Failed to store pipeline state object to pipeline library %ls. hash: (%lld) hr:(0x%08x)", pipelineName, psoRenderHash, result);
            }
            else
            {
                cgpu_trace(u8"Succeeded to store pipeline state object to pipeline library %ls. hash: (%lld) hr:(0x%08x)", pipelineName, psoRenderHash, result);
            }
        }
    }
    D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    switch (desc->prim_topology)
    {
        case CGPU_PRIM_TOPO_POINT_LIST:
            topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
            break;
        case CGPU_PRIM_TOPO_LINE_LIST:
            topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
            break;
        case CGPU_PRIM_TOPO_LINE_STRIP:
            topology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
            break;
        case CGPU_PRIM_TOPO_TRI_LIST:
            topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            break;
        case CGPU_PRIM_TOPO_TRI_STRIP:
            topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            break;
        case CGPU_PRIM_TOPO_PATCH_LIST: {
            // TODO: Support D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST with Hull Shaders
            cgpu_assert(0 && "Unsupported primitive topology!");
        }
        default:
            break;
    }
    PPL->mDxPrimitiveTopology = topology;
    PPL->pRootSignature       = RS->pDxRootSignature;
    return &PPL->super;
}

void cgpu_free_render_pipeline_d3d12(CGPURenderPipelineId pipeline)
{
    CGPURenderPipeline_D3D12* PPL = (CGPURenderPipeline_D3D12*)pipeline;
    SAFE_RELEASE(PPL->pDxPipelineState);
    cgpu_delete_placed(PPL);
    cgpu_freeN(PPL, kD3D12PSOMemoryPoolName);
}


// CMDs
void cgpu_cmd_begin_d3d12(CGPUCommandBufferId cmd)
{
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)cmd;
    CGPUCommandPool_D3D12*   P   = (CGPUCommandPool_D3D12*)Cmd->pCmdPool;
    CHECK_HRESULT(Cmd->pDxCmdList->Reset(P->pDxCmdAlloc, NULL));

    if (Cmd->mType != CGPU_QUEUE_TYPE_TRANSFER)
    {
        ID3D12DescriptorHeap* heaps[] = {
            Cmd->pBoundHeaps[0]->pCurrentHeap,
            Cmd->pBoundHeaps[1]->pCurrentHeap,
        };
        Cmd->pDxCmdList->SetDescriptorHeaps(2, heaps);

        Cmd->mBoundHeapStartHandles[0] = Cmd->pBoundHeaps[0]->pCurrentHeap->GetGPUDescriptorHandleForHeapStart();
        Cmd->mBoundHeapStartHandles[1] = Cmd->pBoundHeaps[1]->pCurrentHeap->GetGPUDescriptorHandleForHeapStart();
    }
    // Reset CPU side data
    Cmd->pBoundRootSignature = NULL;
}