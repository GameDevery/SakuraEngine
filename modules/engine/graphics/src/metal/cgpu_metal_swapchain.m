#include "../common/common_utils.h"
#include "SkrGraphics/backend/metal/cgpu_metal.h"
#include "SkrGraphics/backend/metal/cgpu_metal_types.h"
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>

// Forward declaration for CocoaMetalView
@interface CocoaMetalView : NSView
- (instancetype)initWithFrame:(NSRect)frame;
@end

@implementation CocoaMetalView

+ (Class)layerClass
{
    return [CAMetalLayer class];
}

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        self.wantsLayer = YES;
        self.layer = [CAMetalLayer layer];
    }
    return self;
}

- (CALayer*)makeBackingLayer
{
    return [CAMetalLayer layer];
}

@end

CGPUSwapChainId cgpu_create_swapchain_metal(CGPUDeviceId device, const CGPUSwapChainDescriptor* desc)
{
    cgpu_assert(device && desc && desc->surface && "Invalid parameters for swapchain creation");
    
    CGPUDevice_Metal* mtlDevice = (CGPUDevice_Metal*)device;
    const uint32_t imageCount = desc->image_count;
    
    // Allocate swapchain with space for texture array
    const size_t totalSize = sizeof(CGPUSwapChain_Metal) + 
                           sizeof(CGPUTexture_Metal) * imageCount +
                           sizeof(CGPUTextureId) * imageCount +
                           sizeof(CGPUTextureInfo) * imageCount;
    
    CGPUSwapChain_Metal* swapchain = (CGPUSwapChain_Metal*)cgpu_calloc_aligned(1, totalSize, _Alignof(CGPUSwapChain_Metal));
    if (!swapchain) {
        cgpu_assert(false && "Failed to allocate memory for swapchain");
        return NULL;
    }
    
    // Setup memory layout
    swapchain->pBackBufferTextures = (CGPUTexture_Metal*)(swapchain + 1);
    CGPUTextureId* backbufferIds = (CGPUTextureId*)(swapchain->pBackBufferTextures + imageCount);
    CGPUTextureInfo* backbufferInfos = (CGPUTextureInfo*)(backbufferIds + imageCount);
    
    @autoreleasepool {
        NSView* nsView = (__bridge NSView*)desc->surface;
        CAMetalLayer* metalLayer = nil;
        NSView* metalView = nil;
        bool ownsView = false;
        
        // Check if view already has Metal layer
        if ([nsView.layer isKindOfClass:[CAMetalLayer class]]) {
            metalLayer = (CAMetalLayer*)nsView.layer;
            metalView = nsView;
        }
        else {
            // Create Metal view as subview
            NSRect frame = nsView.bounds;
            CocoaMetalView* newMetalView = [[CocoaMetalView alloc] initWithFrame:frame];
            if (!newMetalView) {
                cgpu_free_aligned(swapchain, _Alignof(CGPUSwapChain_Metal));
                cgpu_assert(false && "Failed to create Metal view");
                return NULL;
            }
            
            [newMetalView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
            [nsView addSubview:newMetalView];
            
            metalView = newMetalView;
            metalLayer = (CAMetalLayer*)newMetalView.layer;
            ownsView = true;
        }
        
        // Configure Metal layer
        metalLayer.device = mtlDevice->pDevice;
        metalLayer.pixelFormat = MetalUtil_TranslatePixelFormat(desc->format);
        metalLayer.framebufferOnly = YES;
        metalLayer.drawableSize = CGSizeMake(desc->width, desc->height);
        metalLayer.displaySyncEnabled = desc->enable_vsync;
        
        // Configure drawable count
        if (@available(macOS 10.13.2, *)) {
            metalLayer.maximumDrawableCount = imageCount;
        }
        
        // Store references
        swapchain->pView = metalView;
        swapchain->pLayer = metalLayer;
        swapchain->mOwnsView = ownsView;
        
        // Retain layer
        CFRetain((__bridge CFTypeRef)metalLayer);
    }
    
    // Initialize base structure
    swapchain->super.device = device;
    swapchain->super.buffer_count = imageCount;
    swapchain->super.back_buffers = (const CGPUTextureId*)backbufferIds;
    swapchain->mCurrentBackBufferIndex = 0;
    
    // Create synchronization semaphore
    swapchain->mImageAcquiredSemaphore = dispatch_semaphore_create(imageCount - 1);
    
    // Initialize texture structures
    for (uint32_t i = 0; i < imageCount; ++i) {
        CGPUTexture_Metal* texture = &swapchain->pBackBufferTextures[i];
        CGPUTextureInfo* info = &backbufferInfos[i];
        
        // Setup texture info
        info->unique_id = UINT64_MAX; // Special ID for swapchain textures
        info->width = desc->width;
        info->height = desc->height;
        info->depth = 1;
        info->array_size_minus_one = 0;
        info->mip_levels = 1;
        info->sample_count = CGPU_SAMPLE_COUNT_1;
        info->format = desc->format;
        info->aspect_mask = CGPU_TVA_COLOR;
        info->node_index = CGPU_SINGLE_GPU_NODE_INDEX;
        info->owns_image = true;
        info->is_cube = false;
        info->is_allocation_dedicated = false;
        info->is_restrict_dedicated = false;
        info->is_aliasing = false;
        info->is_tiled = false;
        info->is_imported = false;
        info->can_alias = false;
        info->can_export = false;
        
        // Initialize texture - drawable will be set during acquire
        texture->super.info = info;
        texture->super.device = device;
        texture->pTexture = nil;
        
        backbufferIds[i] = &texture->super;
    }
    
    return &swapchain->super;
}

uint32_t cgpu_acquire_next_image_metal(CGPUSwapChainId swapchain, const CGPUAcquireNextDescriptor* desc)
{
    CGPUSwapChain_Metal* mtlSwapchain = (CGPUSwapChain_Metal*)swapchain;
    
    @autoreleasepool {
        // Wait on semaphore to ensure we don't exceed drawable count
        dispatch_semaphore_wait(mtlSwapchain->mImageAcquiredSemaphore, DISPATCH_TIME_FOREVER);
        
        // Release previous drawable if exists
        if (mtlSwapchain->pCurrentDrawable) {
            mtlSwapchain->pCurrentDrawable = nil;
        }
        
        // Get next drawable
        id<CAMetalDrawable> drawable = [mtlSwapchain->pLayer nextDrawable];
        if (!drawable) {
            // Signal semaphore back since we failed
            dispatch_semaphore_signal(mtlSwapchain->mImageAcquiredSemaphore);
            cgpu_assert(false && "Failed to acquire next drawable");
            return UINT32_MAX;
        }
        
        // Store drawable and update texture
        mtlSwapchain->pCurrentDrawable = drawable;
        uint32_t index = mtlSwapchain->mCurrentBackBufferIndex;
        CGPUTexture_Metal* texture = &mtlSwapchain->pBackBufferTextures[index];
        texture->pTexture = drawable.texture;
        
        // Signal fence/semaphore if requested
        if (desc) {
            if (desc->signal_semaphore) {
                CGPUSemaphore_Metal* semaphore = (CGPUSemaphore_Metal*)desc->signal_semaphore;
                // Signal immediately since acquire is synchronous
                [semaphore->pMTLEvent setSignaledValue:semaphore->mFenceValue++];
            }
            
            if (desc->fence) {
                CGPUFence_Metal* fence = (CGPUFence_Metal*)desc->fence;
                // Signal immediately since acquire is synchronous
                [fence->pMTLEvent setSignaledValue:fence->mFenceValue++];
            }
        }
        
        // Update index for next acquire
        mtlSwapchain->mCurrentBackBufferIndex = (index + 1) % swapchain->buffer_count;
        
        return index;
    }
}

void cgpu_queue_present_metal(CGPUQueueId queue, const CGPUQueuePresentDescriptor* desc)
{
    CGPUSwapChain_Metal* S = (CGPUSwapChain_Metal*)desc->swapchain;
    
    @autoreleasepool {
        if (!S->pCurrentDrawable) {
            cgpu_error("No drawable to present");
            return;
        }
        
        // Wait semaphores are handled by the previous submit_queue call, not here
        // Just present the drawable immediately
        [S->pCurrentDrawable present];
        
        // Signal completion
        dispatch_semaphore_signal(S->mImageAcquiredSemaphore);
        
        // Clear current drawable reference
        S->pCurrentDrawable = nil;
    }
}

void cgpu_free_swapchain_metal(CGPUSwapChainId swapchain)
{
    if (!swapchain) return;
    
    CGPUSwapChain_Metal* mtlSwapchain = (CGPUSwapChain_Metal*)swapchain;
    
    @autoreleasepool {
        // Clear current drawable
        if (mtlSwapchain->pCurrentDrawable) {
            mtlSwapchain->pCurrentDrawable = nil;
        }
        
        // Release layer
        if (mtlSwapchain->pLayer) {
            CFRelease((__bridge CFTypeRef)mtlSwapchain->pLayer);
            mtlSwapchain->pLayer = nil;
        }
        
        // Remove view if we created it
        if (mtlSwapchain->mOwnsView && mtlSwapchain->pView) {
            [mtlSwapchain->pView removeFromSuperview];
        }
        
        // Release synchronization objects
        if (mtlSwapchain->mImageAcquiredSemaphore) {
            // Drain semaphore
            for (uint32_t i = 0; i < swapchain->buffer_count - 1; ++i) {
                dispatch_semaphore_signal(mtlSwapchain->mImageAcquiredSemaphore);
            }
            mtlSwapchain->mImageAcquiredSemaphore = nil;
        }
    }
    
    cgpu_free_aligned(mtlSwapchain, _Alignof(CGPUSwapChain_Metal));
}