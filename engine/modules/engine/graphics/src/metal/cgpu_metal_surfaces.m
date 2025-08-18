#include "../common/common_utils.h"
#include "SkrGraphics/backend/metal/cgpu_metal.h"
#import <Cocoa/Cocoa.h>

CGPUSurfaceId cgpu_surface_from_ns_view_metal(CGPUDeviceId device, CGPUNSView* view)
{
    if (!view) {
        cgpu_assert(false && "cgpu_surface_from_ns_view_metal: view is NULL!");
        return NULL;
    }
    
    @autoreleasepool {
        // Check if the pointer is actually an NSView
        id obj = (__bridge id)view;
        if (![obj isKindOfClass:[NSView class]]) {
            cgpu_assert(false && "cgpu_surface_from_ns_view_metal: view is not an NSView!");
            return NULL;
        }
    }
    
    // Simply return the NSView pointer as the surface, like D3D12 does with HWND
    return (CGPUSurfaceId)view;
}

void cgpu_free_surface_metal(CGPUDeviceId device, CGPUSurfaceId surface)
{
    // Nothing to do, we don't own the NSView
    return;
}

const CGPUSurfacesProcTable s_tbl_metal = {
    //
    .free_surface = &cgpu_free_surface_metal,
#if defined(_MACOS)
    .from_ns_view = &cgpu_surface_from_ns_view_metal
#endif
};

const CGPUSurfacesProcTable* CGPU_MetalSurfacesProcTable()
{
    return &s_tbl_metal;
};
