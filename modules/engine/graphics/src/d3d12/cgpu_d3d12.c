// clang-format off
#include "SkrGraphics/backend/d3d12/cgpu_d3d12.h"
#include "../common/common_utils.h"
#include "d3d12_utils.h"

void cgpu_query_instance_features_d3d12(CGPUInstanceId instance, struct CGPUInstanceFeatures* features)
{
    CGPUInstance_D3D12* I = (CGPUInstance_D3D12*)instance;
    (void)I;
    features->specialization_constant = false;
}

void cgpu_enum_adapters_d3d12(CGPUInstanceId instance, CGPUAdapterId* const adapters, uint32_t* adapters_num)
{
    cgpu_assert(instance != CGPU_NULLPTR && "fatal: null instance!");
    CGPUInstance_D3D12* I = (CGPUInstance_D3D12*)instance;
    *adapters_num = I->mAdaptersCount;
    if (!adapters)
    {
        return;
    }
    else
    {
        for (uint32_t i = 0u; i < *adapters_num; i++)
            adapters[i] = &(I->pAdapters[i].super);
    }
}

const CGPUAdapterDetail* cgpu_query_adapter_detail_d3d12(const CGPUAdapterId adapter)
{
    const CGPUAdapter_D3D12* A = (CGPUAdapter_D3D12*)adapter;
    return &A->adapter_detail;
}

uint32_t cgpu_query_queue_count_d3d12(const CGPUAdapterId adapter, const ECGPUQueueType type)
{
    // queues are virtual in d3d12.
    /*
    switch(type)
    {
        case CGPU_QUEUE_TYPE_GRAPHICS: return 1;
        case CGPU_QUEUE_TYPE_COMPUTE: return 2;
        case CGPU_QUEUE_TYPE_TRANSFER: return 2;
        default: return 0;
    }
    */
    // return UINT32_MAX;
    return 8; // Avoid returning unreasonable large number
}

void cgpu_query_video_memory_info_d3d12(const CGPUDeviceId device, uint64_t* total, uint64_t* used_bytes)
{
    const CGPUAdapter_D3D12*     A    = (CGPUAdapter_D3D12*)device->adapter;
    DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
    COM_CALL(QueryVideoMemoryInfo, A->pDxActiveGPU, CGPU_SINGLE_GPU_NODE_INDEX, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);
    *total      = info.Budget;
    *used_bytes = info.CurrentUsage;
}

void cgpu_query_shared_memory_info_d3d12(const CGPUDeviceId device, uint64_t* total, uint64_t* used_bytes)
{
    const CGPUAdapter_D3D12*     A    = (CGPUAdapter_D3D12*)device->adapter;
    DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
    COM_CALL(QueryVideoMemoryInfo, A->pDxActiveGPU, CGPU_SINGLE_GPU_NODE_INDEX, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &info);
    *total      = info.Budget;
    *used_bytes = info.CurrentUsage;
}

// API Objects APIs
CGPUFenceId cgpu_create_fence_d3d12(CGPUDeviceId device)
{
    CGPUDevice_D3D12* D = (CGPUDevice_D3D12*)device;
    // create a Fence and cgpu_assert that it is valid
    CGPUFence_D3D12* F = cgpu_calloc(1, sizeof(CGPUFence_D3D12));
    cgpu_assert(F);

    CHECK_HRESULT(COM_CALL(CreateFence, D->pDxDevice, 0, D3D12_FENCE_FLAG_NONE, IID_ARGS(ID3D12Fence, &F->pDxFence)));
    F->mFenceValue = 1;

    F->pDxWaitIdleFenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    return &F->super;
}

void cgpu_wait_fences_d3d12(const CGPUFenceId* fences, uint32_t fence_count)
{
    const CGPUFence_D3D12** Fences = (const CGPUFence_D3D12**)fences;
    for (uint32_t i = 0; i < fence_count; ++i)
    {
        ECGPUFenceStatus fenceStatus = cgpu_query_fence_status(fences[i]);
        uint64_t         fenceValue  = Fences[i]->mFenceValue - 1;
        if (fenceStatus == CGPU_FENCE_STATUS_INCOMPLETE)
        {
            COM_CALL(SetEventOnCompletion, Fences[i]->pDxFence, fenceValue, Fences[i]->pDxWaitIdleFenceEvent);
            WaitForSingleObject(Fences[i]->pDxWaitIdleFenceEvent, INFINITE);
        }
    }
}

ECGPUFenceStatus cgpu_query_fence_status_d3d12(CGPUFenceId fence)
{
    ECGPUFenceStatus status = CGPU_FENCE_STATUS_COMPLETE;
    CGPUFence_D3D12* F      = (CGPUFence_D3D12*)fence;
    const uint64_t Value = COM_CALL(GetCompletedValue, F->pDxFence);
    if (Value < F->mFenceValue - 1)
        status = CGPU_FENCE_STATUS_INCOMPLETE;
    else
        status = CGPU_FENCE_STATUS_COMPLETE;
    return status;
}

void cgpu_free_fence_d3d12(CGPUFenceId fence)
{
    CGPUFence_D3D12* F = (CGPUFence_D3D12*)fence;
    COM_CALL(Release, F->pDxFence);
    CloseHandle(F->pDxWaitIdleFenceEvent);
    cgpu_free(F);
}

CGPUSemaphoreId cgpu_create_semaphore_d3d12(CGPUDeviceId device)
{
    return (CGPUSemaphoreId)cgpu_create_fence(device);
}

void cgpu_free_semaphore_d3d12(CGPUSemaphoreId semaphore)
{
    return cgpu_free_fence((CGPUFenceId)semaphore);
}

CGPURootSignaturePoolId cgpu_create_root_signature_pool_d3d12(CGPUDeviceId device, const struct CGPURootSignaturePoolDescriptor* desc)
{
    return CGPUUtil_CreateRootSignaturePool(desc);
}

void cgpu_free_root_signature_pool_d3d12(CGPURootSignaturePoolId pool)
{
    CGPUUtil_FreeRootSignaturePool(pool);
}

inline static D3D12_QUERY_TYPE D3D12Util_ToD3D12QueryType(ECGPUQueryType type)
{
    switch (type)
    {
        case CGPU_QUERY_TYPE_TIMESTAMP:
            return D3D12_QUERY_TYPE_TIMESTAMP;
        case CGPU_QUERY_TYPE_PIPELINE_STATISTICS:
            return D3D12_QUERY_TYPE_PIPELINE_STATISTICS;
        case CGPU_QUERY_TYPE_OCCLUSION:
            return D3D12_QUERY_TYPE_OCCLUSION;
        default:
            cgpu_assert(false && "Invalid query heap type");
            return D3D12_QUERY_TYPE_OCCLUSION;
    }
}

inline static D3D12_QUERY_HEAP_TYPE D3D12Util_ToD3D12QueryHeapType(ECGPUQueryType type)
{
    switch (type)
    {
        case CGPU_QUERY_TYPE_TIMESTAMP:
            return D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        case CGPU_QUERY_TYPE_PIPELINE_STATISTICS:
            return D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS;
        case CGPU_QUERY_TYPE_OCCLUSION:
            return D3D12_QUERY_HEAP_TYPE_OCCLUSION;
        default:
            cgpu_assert(false && "Invalid query heap type");
            return D3D12_QUERY_HEAP_TYPE_OCCLUSION;
    }
}

CGPUQueryPoolId cgpu_create_query_pool_d3d12(CGPUDeviceId device, const struct CGPUQueryPoolDescriptor* desc)
{
    CGPUDevice_D3D12*    D          = (CGPUDevice_D3D12*)device;
    CGPUQueryPool_D3D12* pQueryPool = cgpu_calloc(1, sizeof(CGPUQueryPool_D3D12));
    pQueryPool->mType               = D3D12Util_ToD3D12QueryType(desc->type);
    pQueryPool->super.count         = desc->query_count;

    D3D12_QUERY_HEAP_DESC Desc = {};
    Desc.Count                 = desc->query_count;
    Desc.NodeMask              = CGPU_SINGLE_GPU_NODE_MASK;
    Desc.Type                  = D3D12Util_ToD3D12QueryHeapType(desc->type);
    COM_CALL(CreateQueryHeap, D->pDxDevice, &Desc, IID_ARGS(ID3D12QueryHeap, &pQueryPool->pDxQueryHeap));

    return &pQueryPool->super;
}

void cgpu_free_query_pool_d3d12(CGPUQueryPoolId pool)
{
    CGPUQueryPool_D3D12* QP = (CGPUQueryPool_D3D12*)pool;
    COM_CALL(Release, QP->pDxQueryHeap);
    cgpu_free(QP);
}

// Queue APIs
CGPUQueueId cgpu_get_queue_d3d12(CGPUDeviceId device, ECGPUQueueType type, uint32_t index)
{
    CGPUDevice_D3D12* D = (CGPUDevice_D3D12*)device;
    CGPUQueue_D3D12*  Q = cgpu_calloc(1, sizeof(CGPUQueue_D3D12));
    Q->pCommandQueue    = D->ppCommandQueues[type][index];
    Q->pFence           = (CGPUFence_D3D12*)cgpu_create_fence_d3d12(device);
    return &Q->super;
}

void cgpu_submit_queue_d3d12(CGPUQueueId queue, const struct CGPUQueueSubmitDescriptor* desc)
{
    uint32_t                  CmdCount = desc->cmds_count;
    CGPUCommandBuffer_D3D12** Cmds     = (CGPUCommandBuffer_D3D12**)desc->cmds;
    CGPUQueue_D3D12*          Q        = (CGPUQueue_D3D12*)queue;
    CGPUFence_D3D12*          F        = (CGPUFence_D3D12*)desc->signal_fence;

    // cgpu_assert that given cmd list and given params are valid
    cgpu_assert(CmdCount > 0);
    cgpu_assert(Cmds);
    // execute given command list
    cgpu_assert(Q->pCommandQueue);

    ID3D12CommandList* cmds[CmdCount];
    for (uint32_t i = 0; i < CmdCount; ++i)
    {
        cmds[i] = (ID3D12CommandList*)Cmds[i]->pDxCmdList;
    }
    // Wait semaphores
    CGPUFence_D3D12** WaitSemaphores = (CGPUFence_D3D12**)desc->wait_semaphores;
    for (uint32_t i = 0; i < desc->wait_semaphore_count; ++i)
        COM_CALL(Wait, Q->pCommandQueue, WaitSemaphores[i]->pDxFence, WaitSemaphores[i]->mFenceValue - 1);
    // Execute
    COM_CALL(ExecuteCommandLists, Q->pCommandQueue, CmdCount, cmds);
    // Signal fences
    if (F)
        D3D12Util_SignalFence(Q, F->pDxFence, F->mFenceValue++);
    // Signal Semaphores
    CGPUFence_D3D12** SignalSemaphores = (CGPUFence_D3D12**)desc->signal_semaphores;
    for (uint32_t i = 0; i < desc->signal_semaphore_count; i++)
        D3D12Util_SignalFence(Q, SignalSemaphores[i]->pDxFence, SignalSemaphores[i]->mFenceValue++);
}

void cgpu_wait_queue_idle_d3d12(CGPUQueueId queue)
{
    CGPUQueue_D3D12* Q = (CGPUQueue_D3D12*)queue;
    D3D12Util_SignalFence(Q, Q->pFence->pDxFence, Q->pFence->mFenceValue++);

    uint64_t fenceValue = Q->pFence->mFenceValue - 1;
    if (COM_CALL(GetCompletedValue, Q->pFence->pDxFence) < Q->pFence->mFenceValue - 1)
    {
        COM_CALL(SetEventOnCompletion, Q->pFence->pDxFence, fenceValue, Q->pFence->pDxWaitIdleFenceEvent);
        WaitForSingleObject(Q->pFence->pDxWaitIdleFenceEvent, INFINITE);
    }
}

void cgpu_queue_present_d3d12(CGPUQueueId queue, const struct CGPUQueuePresentDescriptor* desc)
{
    CGPUSwapChain_D3D12* S  = (CGPUSwapChain_D3D12*)desc->swapchain;
    if (FAILED(COM_CALL(Present, S->pDxSwapChain, S->mDxSyncInterval, S->mFlags)))
    {
        cgpu_error(u8"Failed to present swapchain render target!");
#if defined(_WIN32)
        ID3D12Device* device = NULL;
        COM_CALL(GetDevice, S->pDxSwapChain, IID_ARGS(ID3D12Device, &device));
        D3D12Util_ReportGPUCrash(device);
        COM_CALL(Release, device);
        ((CGPUDevice*)queue->device)->is_lost = true;
#endif
    }
}

float cgpu_queue_get_timestamp_period_ns_d3d12(CGPUQueueId queue)
{
    CGPUQueue_D3D12* Q    = (CGPUQueue_D3D12*)queue;
    UINT64           freq = 0;
    // ticks per second
    COM_CALL(GetTimestampFrequency, Q->pCommandQueue, &freq);
    // ns per tick
    const double ms_period = 1e9 / (double)freq;
    return (float)ms_period;
}

void cgpu_free_queue_d3d12(CGPUQueueId queue)
{
    CGPUQueue_D3D12* Q = (CGPUQueue_D3D12*)queue;
    cgpu_assert(queue && "D3D12 ERROR: FREE NULL QUEUE!");
    cgpu_free_fence_d3d12(&Q->pFence->super);
    cgpu_free(Q);
}

// Command Objects
inline static ID3D12CommandAllocator* D3D12Util_AllocateTransientCommandAllocator(CGPUCommandPool_D3D12* E, CGPUQueueId queue)
{
    CGPUDevice_D3D12* D = (CGPUDevice_D3D12*)queue->device;

    D3D12_COMMAND_LIST_TYPE type =
        queue->type == CGPU_QUEUE_TYPE_TRANSFER ?
        D3D12_COMMAND_LIST_TYPE_COPY :
        queue->type == CGPU_QUEUE_TYPE_COMPUTE ?
        D3D12_COMMAND_LIST_TYPE_COMPUTE : D3D12_COMMAND_LIST_TYPE_DIRECT;

    if (SUCCEEDED(COM_CALL(CreateCommandAllocator, D->pDxDevice, type, IID_ARGS(ID3D12CommandAllocator, &E->pDxCmdAlloc))))
    {
        return E->pDxCmdAlloc;
    }
    return CGPU_NULLPTR;
}

inline static void D3D12Util_FreeTransientCommandAllocator(ID3D12CommandAllocator* allocator) 
{ 
    COM_CALL(Release, allocator); 
}

CGPUCommandPoolId cgpu_create_command_pool_d3d12(CGPUQueueId queue, const CGPUCommandPoolDescriptor* desc)
{
    CGPUCommandPool_D3D12* P = cgpu_calloc(1, sizeof(CGPUCommandPool_D3D12));
    P->pDxCmdAlloc = D3D12Util_AllocateTransientCommandAllocator(P, queue);
    return &P->super;
}

void cgpu_reset_command_pool_d3d12(CGPUCommandPoolId pool)
{
    CGPUCommandPool_D3D12* P = (CGPUCommandPool_D3D12*)pool;
    CHECK_HRESULT(COM_CALL(Reset, P->pDxCmdAlloc));
}

CGPUCommandBufferId cgpu_create_command_buffer_d3d12(CGPUCommandPoolId pool, const struct CGPUCommandBufferDescriptor* desc)
{
    // initialize to zero
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)cgpu_calloc(1, sizeof(CGPUCommandBuffer_D3D12));
    CGPUCommandPool_D3D12*   P   = (CGPUCommandPool_D3D12*)pool;
    CGPUQueue_D3D12*         Q   = (CGPUQueue_D3D12*)P->super.queue;
    CGPUDevice_D3D12*        D   = (CGPUDevice_D3D12*)Q->super.device;
    cgpu_assert(Cmd);

    // set command pool of new command
    Cmd->mNodeIndex = CGPU_SINGLE_GPU_NODE_INDEX;
    Cmd->mType      = Q->super.type;

    Cmd->pBoundHeaps[0] = D->pCbvSrvUavHeaps[Cmd->mNodeIndex];
    Cmd->pBoundHeaps[1] = D->pSamplerHeaps[Cmd->mNodeIndex];
    Cmd->pCmdPool       = P;

    uint32_t nodeMask = Cmd->mNodeIndex;

    ID3D12PipelineState* initialState = NULL;
    CHECK_HRESULT(COM_CALL(CreateCommandList, D->pDxDevice, 
        nodeMask, gDx12CmdTypeTranslator[Cmd->mType], P->pDxCmdAlloc, initialState, IID_ARGS(ID3D12CommandList, &Cmd->pDxCmdList)));

    // Command lists are addd in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    CHECK_HRESULT(COM_CALL(Close, Cmd->pDxCmdList));
    return &Cmd->super;
}

void cgpu_free_command_buffer_d3d12(CGPUCommandBufferId cmd)
{
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)cmd;
    COM_CALL(Release, Cmd->pDxCmdList);
    cgpu_free(Cmd);
}

void cgpu_free_command_pool_d3d12(CGPUCommandPoolId pool)
{
    CGPUCommandPool_D3D12* P = (CGPUCommandPool_D3D12*)pool;
    cgpu_assert(pool && "D3D12 ERROR: FREE NULL COMMAND POOL!");
    cgpu_assert(P->pDxCmdAlloc && "D3D12 ERROR: FREE NULL pDxCmdAlloc!");

    D3D12Util_FreeTransientCommandAllocator(P->pDxCmdAlloc);
    cgpu_free(P);
}

// CMDs
void cgpu_cmd_begin_query_d3d12(CGPUCommandBufferId cmd, CGPUQueryPoolId pool, const struct CGPUQueryDescriptor* desc)
{
    CGPUCommandBuffer_D3D12* CMD = (CGPUCommandBuffer_D3D12*)cmd;
    CGPUQueryPool_D3D12* pQueryPool = (CGPUQueryPool_D3D12*)pool;
    D3D12_QUERY_TYPE type = pQueryPool->mType;
    switch (type)
    {
        case D3D12_QUERY_TYPE_TIMESTAMP:
            COM_CALL(EndQuery, CMD->pDxCmdList, pQueryPool->pDxQueryHeap, type, desc->index);
            break;
        case D3D12_QUERY_TYPE_OCCLUSION:
        case D3D12_QUERY_TYPE_PIPELINE_STATISTICS:
        case D3D12_QUERY_TYPE_BINARY_OCCLUSION:
        case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0:
        case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM1:
        case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM2:
        case D3D12_QUERY_TYPE_SO_STATISTICS_STREAM3:
        default:
            break;
    }
}

void cgpu_cmd_end_query_d3d12(CGPUCommandBufferId cmd, CGPUQueryPoolId pool, const struct CGPUQueryDescriptor* desc)
{
    cgpu_cmd_begin_query(cmd, pool, desc);
}

void cgpu_cmd_reset_query_pool_d3d12(CGPUCommandBufferId cmd, CGPUQueryPoolId pool, uint32_t start_query, uint32_t query_count)
{

}

void cgpu_cmd_resolve_query_d3d12(CGPUCommandBufferId cmd, CGPUQueryPoolId pool, CGPUBufferId readback, uint32_t start_query, uint32_t query_count)
{
    CGPUCommandBuffer_D3D12* CMD = (CGPUCommandBuffer_D3D12*)cmd;
    CGPUQueryPool_D3D12* pQueryPool = (CGPUQueryPool_D3D12*)pool;
    CGPUBuffer_D3D12* pReadbackBuffer = (CGPUBuffer_D3D12*)readback;
    COM_CALL(ResolveQueryData, CMD->pDxCmdList, 
        pQueryPool->pDxQueryHeap, pQueryPool->mType, start_query, query_count,
        pReadbackBuffer->pDxResource, start_query * 8
    );
}

void cgpu_cmd_end_d3d12(CGPUCommandBufferId cmd)
{
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)cmd;
    cgpu_assert(Cmd->pDxCmdList);
    CHECK_HRESULT(COM_CALL(Close, Cmd->pDxCmdList));
}

// Compute CMDs
CGPUComputePassEncoderId cgpu_cmd_begin_compute_pass_d3d12(CGPUCommandBufferId cmd, const struct CGPUComputePassDescriptor* desc)
{
    // DO NOTHING NOW
    return (CGPUComputePassEncoderId)cmd;
}

void cgpu_compute_encoder_bind_descriptor_set_d3d12(CGPUComputePassEncoderId encoder, CGPUDescriptorSetId set)
{
    CGPUCommandBuffer_D3D12*       Cmd = (CGPUCommandBuffer_D3D12*)encoder;
    const CGPUDescriptorSet_D3D12* Set = (CGPUDescriptorSet_D3D12*)set;
    D3D12_GPU_DESCRIPTOR_HANDLE HeapToBind = { Cmd->mBoundHeapStartHandles[0].ptr + Set->mCbvSrvUavHandle };
    COM_CALL(SetComputeRootDescriptorTable, Cmd->pDxCmdList, set->index, HeapToBind);
}

inline static bool D3D12Util_ResetRootSignature(CGPUCommandBuffer_D3D12* pCmd, ECGPUPipelineType type, ID3D12RootSignature* pRootSignature)
{
    // Set root signature if the current one differs from pRootSignature
    if (pCmd->pBoundRootSignature != pRootSignature)
    {
        pCmd->pBoundRootSignature = pRootSignature;
        if (type == CGPU_PIPELINE_TYPE_GRAPHICS)
            COM_CALL(SetGraphicsRootSignature, pCmd->pDxCmdList, pRootSignature);
        else
            COM_CALL(SetComputeRootSignature, pCmd->pDxCmdList, pRootSignature);
    }
    return true;
}

void cgpu_compute_encoder_bind_pipeline_d3d12(CGPUComputePassEncoderId encoder, CGPUComputePipelineId pipeline)
{
    CGPUCommandBuffer_D3D12*   Cmd = (CGPUCommandBuffer_D3D12*)encoder;
    CGPUComputePipeline_D3D12* PPL = (CGPUComputePipeline_D3D12*)pipeline;
    D3D12Util_ResetRootSignature(Cmd, CGPU_PIPELINE_TYPE_COMPUTE, PPL->pRootSignature);
    COM_CALL(SetPipelineState, Cmd->pDxCmdList, PPL->pDxPipelineState);
}

void cgpu_compute_encoder_push_constants_d3d12(CGPUComputePassEncoderId encoder, CGPURootSignatureId rs, const char8_t* name, const void* data)
{
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)encoder;
    CGPURootSignature_D3D12* RS  = (CGPURootSignature_D3D12*)rs;
    D3D12Util_ResetRootSignature(Cmd, CGPU_PIPELINE_TYPE_GRAPHICS, RS->pDxRootSignature);
    if (RS->super.pipeline_type == CGPU_PIPELINE_TYPE_GRAPHICS)
    {
        COM_CALL(SetGraphicsRoot32BitConstants, Cmd->pDxCmdList,
            RS->mRootParamIndex, RS->mRootConstantParam.Constants.Num32BitValues, data, 0);
    }
    else if (RS->super.pipeline_type == CGPU_PIPELINE_TYPE_COMPUTE)
    {
        COM_CALL(SetComputeRoot32BitConstants, Cmd->pDxCmdList,
            RS->mRootParamIndex, RS->mRootConstantParam.Constants.Num32BitValues, data, 0);
    }
}

// Render CMDs

void cgpu_render_encoder_bind_vertex_buffers_d3d12(CGPURenderPassEncoderId encoder, uint32_t buffer_count,
                                                   const CGPUBufferId* buffers, const uint32_t* strides, const uint32_t* offsets)
{
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)encoder;

    const CGPUBuffer_D3D12** Buffers = (const CGPUBuffer_D3D12**)buffers;
    SKR_DECLARE_ZERO(D3D12_VERTEX_BUFFER_VIEW, views[CGPU_MAX_VERTEX_ATTRIBS]);
    for (uint32_t i = 0; i < buffer_count; ++i)
    {
        cgpu_assert(D3D12_GPU_VIRTUAL_ADDRESS_NULL != Buffers[i]->mDxGpuAddress);

        views[i].BufferLocation =
        (Buffers[i]->mDxGpuAddress + (offsets ? offsets[i] : 0));
        views[i].SizeInBytes =
        (UINT)(Buffers[i]->super.info->size - (offsets ? offsets[i] : 0));
        views[i].StrideInBytes = (UINT)strides[i];
    }
    COM_CALL(IASetVertexBuffers, Cmd->pDxCmdList, 0, buffer_count, views);
}

void cgpu_render_encoder_bind_index_buffer_d3d12(CGPURenderPassEncoderId encoder, CGPUBufferId buffer,
                                                 uint32_t index_stride, uint64_t offset)
{
    CGPUCommandBuffer_D3D12* Cmd    = (CGPUCommandBuffer_D3D12*)encoder;
    const CGPUBuffer_D3D12*  Buffer = (const CGPUBuffer_D3D12*)buffer;
    cgpu_assert(Cmd);
    cgpu_assert(buffer);
    cgpu_assert(CGPU_NULLPTR != Cmd->pDxCmdList);
    cgpu_assert(CGPU_NULLPTR != Buffer->pDxResource);

    SKR_DECLARE_ZERO(D3D12_INDEX_BUFFER_VIEW, view);
    view.BufferLocation = Buffer->mDxGpuAddress + offset;
    view.Format =
        (sizeof(uint16_t) == index_stride) ?
        DXGI_FORMAT_R16_UINT :
        ((sizeof(uint8_t) == index_stride) ? DXGI_FORMAT_R8_UINT : DXGI_FORMAT_R32_UINT);
    view.SizeInBytes = (UINT)(Buffer->super.info->size - offset);
    COM_CALL(IASetIndexBuffer, Cmd->pDxCmdList, &view);
}

void cgpu_compute_encoder_dispatch_d3d12(CGPUComputePassEncoderId encoder, uint32_t X, uint32_t Y, uint32_t Z)
{
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)encoder;
    COM_CALL(Dispatch, Cmd->pDxCmdList, X, Y, Z);
}

void cgpu_cmd_end_compute_pass_d3d12(CGPUCommandBufferId cmd, CGPUComputePassEncoderId encoder)
{
    // DO NOTHING NOW
}

CGPURenderPassEncoderId cgpu_cmd_begin_render_pass_d3d12(CGPUCommandBufferId cmd, const struct CGPURenderPassDescriptor* desc)
{
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)cmd;
#ifdef __ID3D12GraphicsCommandList4_FWD_DEFINED__
    ID3D12GraphicsCommandList4* CmdList4 = (ID3D12GraphicsCommandList4*)Cmd->pDxCmdList;
    SKR_DECLARE_ZERO(D3D12_CLEAR_VALUE, clearValues[CGPU_MAX_MRT_COUNT]);
    SKR_DECLARE_ZERO(D3D12_CLEAR_VALUE, clearDepth);
    SKR_DECLARE_ZERO(D3D12_CLEAR_VALUE, clearStencil);
    SKR_DECLARE_ZERO(D3D12_RENDER_PASS_RENDER_TARGET_DESC, renderPassRenderTargetDescs[CGPU_MAX_MRT_COUNT]);
    SKR_DECLARE_ZERO(D3D12_RENDER_PASS_DEPTH_STENCIL_DESC, renderPassDepthStencilDesc);
    uint32_t colorTargetCount = 0;
    // color
    for (uint32_t i = 0; i < desc->render_target_count; i++)
    {
        CGPUTextureView_D3D12* TV = (CGPUTextureView_D3D12*)desc->color_attachments[i].view;

        clearValues[i].Format  = DXGIUtil_TranslatePixelFormat(TV->super.info.format, false);
        clearValues[i].Color[0] = desc->color_attachments[i].clear_color.r;
        clearValues[i].Color[1] = desc->color_attachments[i].clear_color.g;
        clearValues[i].Color[2] = desc->color_attachments[i].clear_color.b;
        clearValues[i].Color[3] = desc->color_attachments[i].clear_color.a;
        D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE BeginningAccessType = gDx12PassBeginOpTranslator[desc->color_attachments[i].load_action];
        CGPUTextureView_D3D12* TV_Resolve = (CGPUTextureView_D3D12*)desc->color_attachments[i].resolve_view;
        if (desc->sample_count != CGPU_SAMPLE_COUNT_1 && TV_Resolve)
        {
            CGPUTexture_D3D12* T = (CGPUTexture_D3D12*)TV->super.info.texture;
            CGPUTexture_D3D12* T_Resolve = (CGPUTexture_D3D12*)TV_Resolve->super.info.texture;
            const CGPUTextureInfo* pTexInfo = T->super.info;
            const CGPUTextureInfo* pResolveTexInfo = T_Resolve->super.info;
            
            D3D12_RENDER_PASS_BEGINNING_ACCESS BeginningAccess = { .Type = BeginningAccessType, .Clear = { .ClearValue = clearValues[i] } };
            D3D12_RENDER_PASS_ENDING_ACCESS_TYPE EndingAccessType = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
            D3D12_RENDER_PASS_ENDING_ACCESS EndingAccess = { .Type = EndingAccessType, .Resolve = { 0 } };

            renderPassRenderTargetDescs[colorTargetCount].cpuDescriptor = TV->mDxRtvDsvDescriptorHandle;
            renderPassRenderTargetDescs[colorTargetCount].BeginningAccess = BeginningAccess;
            renderPassRenderTargetDescs[colorTargetCount].EndingAccess = EndingAccess;

            D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_PARAMETERS* pResolve = &renderPassRenderTargetDescs[colorTargetCount].EndingAccess.Resolve;
            pResolve->ResolveMode = D3D12_RESOLVE_MODE_AVERAGE; // TODO: int->MODE_MAX
            pResolve->Format = clearValues[i].Format;
            pResolve->pSrcResource = T->pDxResource;
            pResolve->pDstResource = T_Resolve->pDxResource;
            // Cmd->mSubResolveResource[i].SrcRect = { 0, 0, 0, 0 };
            // RenderDoc has a bug, it will crash if SrcRect is zero
            D3D12_RECT SrcRect = { 0, 0, (LONG)pTexInfo->width, (LONG)pTexInfo->height };
            Cmd->mSubResolveResource[i].SrcRect = SrcRect;
            Cmd->mSubResolveResource[i].DstX = 0;
            Cmd->mSubResolveResource[i].DstY = 0;
            Cmd->mSubResolveResource[i].SrcSubresource = 0;
            Cmd->mSubResolveResource[i].DstSubresource = CALC_SUBRESOURCE_INDEX(
                0, 0, 0,
                pResolveTexInfo->mip_levels, pResolveTexInfo->array_size_minus_one + 1
            );
            pResolve->PreserveResolveSource  = false;
            pResolve->SubresourceCount       = 1;
            pResolve->pSubresourceParameters = &Cmd->mSubResolveResource[i];
        }
        else
        {
            // Load & Store action
            D3D12_RENDER_PASS_BEGINNING_ACCESS BeginningAccess = { .Type = BeginningAccessType, .Clear = { .ClearValue = clearValues[i] } };
            D3D12_RENDER_PASS_ENDING_ACCESS_TYPE EndingAccessType = gDx12PassEndOpTranslator[desc->color_attachments[i].store_action];
            D3D12_RENDER_PASS_ENDING_ACCESS EndingAccess = { .Type = EndingAccessType, .Resolve = { 0 } };
            renderPassRenderTargetDescs[colorTargetCount].cpuDescriptor   = TV->mDxRtvDsvDescriptorHandle;
            renderPassRenderTargetDescs[colorTargetCount].BeginningAccess = BeginningAccess;
            renderPassRenderTargetDescs[colorTargetCount].EndingAccess    = EndingAccess;
        }
        colorTargetCount++;
    }
    // depth stencil
    D3D12_RENDER_PASS_DEPTH_STENCIL_DESC* pRenderPassDepthStencilDesc = CGPU_NULLPTR;
    if (desc->depth_stencil != CGPU_NULLPTR && desc->depth_stencil->view != CGPU_NULLPTR)
    {
        CGPUTextureView_D3D12*                  DTV             = (CGPUTextureView_D3D12*)desc->depth_stencil->view;
        D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE dBeginingAccess = gDx12PassBeginOpTranslator[desc->depth_stencil->depth_load_action];
        D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE sBeginingAccess = gDx12PassBeginOpTranslator[desc->depth_stencil->stencil_load_action];
        D3D12_RENDER_PASS_ENDING_ACCESS_TYPE    dEndingAccess   = gDx12PassEndOpTranslator[desc->depth_stencil->depth_store_action];
        D3D12_RENDER_PASS_ENDING_ACCESS_TYPE    sEndingAccess   = gDx12PassEndOpTranslator[desc->depth_stencil->stencil_store_action];
        clearDepth.Format                                       = DXGIUtil_TranslatePixelFormat(desc->depth_stencil->view->info.format, false);
        clearDepth.DepthStencil.Depth                           = desc->depth_stencil->clear_depth;
        clearStencil.Format                                     = DXGIUtil_TranslatePixelFormat(desc->depth_stencil->view->info.format, false);
        clearStencil.DepthStencil.Stencil                       = desc->depth_stencil->clear_stencil;
        renderPassDepthStencilDesc.cpuDescriptor                = DTV->mDxRtvDsvDescriptorHandle;
        
        D3D12_RENDER_PASS_BEGINNING_ACCESS DepthBeginningAccess = { dBeginingAccess, { clearDepth } };
        renderPassDepthStencilDesc.DepthBeginningAccess = DepthBeginningAccess;
        
        D3D12_RENDER_PASS_ENDING_ACCESS DepthEndingAccess = { dEndingAccess, { 0 } };
        renderPassDepthStencilDesc.DepthEndingAccess = DepthEndingAccess;
        
        D3D12_RENDER_PASS_BEGINNING_ACCESS StencilBeginningAccess = { sBeginingAccess, { clearStencil } };
        renderPassDepthStencilDesc.StencilBeginningAccess = StencilBeginningAccess;

        D3D12_RENDER_PASS_ENDING_ACCESS StencilEndingAccess = { sEndingAccess, { 0 } };
        renderPassDepthStencilDesc.StencilEndingAccess = StencilEndingAccess;

        pRenderPassDepthStencilDesc                             = &renderPassDepthStencilDesc;
    }
    D3D12_RENDER_PASS_RENDER_TARGET_DESC* pRenderPassRenderTargetDesc = renderPassRenderTargetDescs;
    COM_CALL(BeginRenderPass, CmdList4, colorTargetCount,
                              pRenderPassRenderTargetDesc, pRenderPassDepthStencilDesc,
                              D3D12_RENDER_PASS_FLAG_NONE);
    return (CGPURenderPassEncoderId)&Cmd->super;
#endif
    cgpu_warn(u8"ID3D12GraphicsCommandList4 is not defined!");
    return (CGPURenderPassEncoderId)&Cmd->super;
}

void cgpu_cmd_end_render_pass_d3d12(CGPUCommandBufferId cmd, CGPURenderPassEncoderId encoder)
{
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)cmd;
#ifdef __ID3D12GraphicsCommandList4_FWD_DEFINED__
    ID3D12GraphicsCommandList4* CmdList4 = (ID3D12GraphicsCommandList4*)Cmd->pDxCmdList;
    COM_CALL(EndRenderPass, CmdList4);
    return;
#endif
    cgpu_warn(u8"ID3D12GraphicsCommandList4 is not defined!");
}

CGPURenderPassEncoderId cgpu_cmd_begin_render_pass_d3d12_fallback(CGPUCommandBufferId cmd, const struct CGPURenderPassDescriptor* desc)
{
    SKR_UNIMPLEMENTED_FUNCTION();

    CGPUCommandBuffer_D3D12*    Cmd      = (CGPUCommandBuffer_D3D12*)cmd;
    ID3D12GraphicsCommandList*  pCmdList = Cmd->pDxCmdList;
    D3D12_CPU_DESCRIPTOR_HANDLE Rtvs[CGPU_MAX_MRT_COUNT];
    D3D12_CPU_DESCRIPTOR_HANDLE Dtv;
    for (uint32_t i = 0; i < desc->render_target_count; i++)
    {
        CGPUTextureView_D3D12* RTV = (CGPUTextureView_D3D12*)desc->color_attachments[i].view;
        Rtvs[i] = RTV->mDxRtvDsvDescriptorHandle;
    }
    if (desc->depth_stencil && desc->depth_stencil->view)
    {
        CGPUTextureView_D3D12* DTV = (CGPUTextureView_D3D12*)desc->depth_stencil->view;
        Dtv = DTV->mDxRtvDsvDescriptorHandle;
    }

    COM_CALL(OMSetRenderTargets, pCmdList, desc->render_target_count, Rtvs, FALSE, desc->depth_stencil ? &Dtv : CGPU_NULLPTR);

    return (CGPURenderPassEncoderId)&Cmd->super;
}

void cgpu_cmd_end_render_pass_d3d12_fallback(CGPUCommandBufferId cmd, CGPURenderPassEncoderId encoder)
{
    SKR_UNIMPLEMENTED_FUNCTION();
    // CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)cmd;
    // ID3D12GraphicsCommandList* pCmdList = Cmd->pDxCmdList;
}

void cgpu_render_encoder_bind_descriptor_set_d3d12(CGPURenderPassEncoderId encoder, CGPUDescriptorSetId set)
{
    CGPUCommandBuffer_D3D12*       Cmd = (CGPUCommandBuffer_D3D12*)encoder;
    const CGPUDescriptorSet_D3D12* Set = (CGPUDescriptorSet_D3D12*)set;
    CGPURootSignature_D3D12*       RS  = (CGPURootSignature_D3D12*)Set->super.root_signature;
    SKR_ASSERT(RS);
    D3D12Util_ResetRootSignature(Cmd, CGPU_PIPELINE_TYPE_GRAPHICS, RS->pDxRootSignature);
    if (Set->mCbvSrvUavHandle != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE HeapToBind = { Cmd->mBoundHeapStartHandles[0].ptr + Set->mCbvSrvUavHandle };
        COM_CALL(SetGraphicsRootDescriptorTable, Cmd->pDxCmdList, set->index, HeapToBind);
    }
    else if (Set->mSamplerHandle != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE HeapToBind = { Cmd->mBoundHeapStartHandles[1].ptr + Set->mSamplerHandle };
        COM_CALL(SetGraphicsRootDescriptorTable, Cmd->pDxCmdList, set->index, HeapToBind);
    }
}

void cgpu_render_encoder_set_viewport_d3d12(CGPURenderPassEncoderId encoder, float x, float y, float width, float height, float min_depth, float max_depth)
{
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)encoder;
    D3D12_VIEWPORT           viewport;
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width    = width;
    viewport.Height   = height;
    viewport.MinDepth = min_depth;
    viewport.MaxDepth = max_depth;
    COM_CALL(RSSetViewports, Cmd->pDxCmdList, 1, &viewport);
}

void cgpu_render_encoder_set_scissor_d3d12(CGPURenderPassEncoderId encoder, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)encoder;
    D3D12_RECT               scissor;
    scissor.left   = x;
    scissor.top    = y;
    scissor.right  = x + width;
    scissor.bottom = y + height;
    COM_CALL(RSSetScissorRects, Cmd->pDxCmdList, 1, &scissor);
}

void cgpu_render_encoder_bind_pipeline_d3d12(CGPURenderPassEncoderId encoder, CGPURenderPipelineId pipeline)
{
    CGPUCommandBuffer_D3D12*  Cmd = (CGPUCommandBuffer_D3D12*)encoder;
    CGPURenderPipeline_D3D12* PPL = (CGPURenderPipeline_D3D12*)pipeline;
    D3D12Util_ResetRootSignature(Cmd, CGPU_PIPELINE_TYPE_GRAPHICS, PPL->pRootSignature);
    COM_CALL(IASetPrimitiveTopology, Cmd->pDxCmdList, PPL->mDxPrimitiveTopology);
    COM_CALL(SetPipelineState, Cmd->pDxCmdList, PPL->pDxPipelineState);
}

void cgpu_render_encoder_push_constants_d3d12(CGPURenderPassEncoderId encoder, CGPURootSignatureId rs, const char8_t* name, const void* data)
{
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)encoder;
    CGPURootSignature_D3D12* RS  = (CGPURootSignature_D3D12*)rs;
    D3D12Util_ResetRootSignature(Cmd, CGPU_PIPELINE_TYPE_GRAPHICS, RS->pDxRootSignature);
    if (RS->super.pipeline_type == CGPU_PIPELINE_TYPE_GRAPHICS)
    {
        COM_CALL(SetGraphicsRoot32BitConstants, Cmd->pDxCmdList, 
                RS->mRootParamIndex, RS->mRootConstantParam.Constants.Num32BitValues, data, 0);
    }
    else if (RS->super.pipeline_type == CGPU_PIPELINE_TYPE_COMPUTE)
    {
        COM_CALL(SetComputeRoot32BitConstants, Cmd->pDxCmdList,
            RS->mRootParamIndex, RS->mRootConstantParam.Constants.Num32BitValues, data, 0);
    }
}

void cgpu_render_encoder_draw_d3d12(CGPURenderPassEncoderId encoder, uint32_t vertex_count, uint32_t first_vertex)
{
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)encoder;
    COM_CALL(DrawInstanced, Cmd->pDxCmdList, (UINT)vertex_count, (UINT)1, (UINT)first_vertex, (UINT)0);
}

void cgpu_render_encoder_draw_instanced_d3d12(CGPURenderPassEncoderId encoder, uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count, uint32_t first_instance)
{
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)encoder;
    COM_CALL(DrawInstanced, Cmd->pDxCmdList, (UINT)vertex_count, (UINT)instance_count, (UINT)first_vertex, (UINT)first_instance);
}

void cgpu_render_encoder_draw_indexed_d3d12(CGPURenderPassEncoderId encoder, uint32_t index_count, uint32_t first_index, uint32_t first_vertex)
{
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)encoder;
    COM_CALL(DrawIndexedInstanced, Cmd->pDxCmdList, (UINT)index_count, (UINT)1, (UINT)first_index, (UINT)first_vertex, (UINT)0);
}

void cgpu_render_encoder_draw_indexed_instanced_d3d12(CGPURenderPassEncoderId encoder, uint32_t index_count, uint32_t first_index, uint32_t instance_count, uint32_t first_instance, uint32_t first_vertex)
{
    CGPUCommandBuffer_D3D12* Cmd = (CGPUCommandBuffer_D3D12*)encoder;
    COM_CALL(DrawIndexedInstanced, Cmd->pDxCmdList, (UINT)index_count, (UINT)instance_count, (UINT)first_index, (UINT)first_vertex, (UINT)first_instance);
}
