#ifdef __APPLE__

#include "cocoa_helper.h"
#import <Cocoa/Cocoa.h>

namespace skr {
namespace cocoa {

void* get_content_view_from_window(void* nswindow)
{
    if (!nswindow) return nullptr;
    
    NSWindow* window = (__bridge NSWindow*)nswindow;
    NSView* contentView = [window contentView];
    return (__bridge void*)contentView;
}

} // namespace cocoa
} // namespace skr

#endif // __APPLE__