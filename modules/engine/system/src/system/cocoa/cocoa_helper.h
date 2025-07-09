#pragma once

#ifdef __APPLE__

namespace skr {
namespace cocoa {

// Get the content NSView from an NSWindow
void* get_content_view_from_window(void* nswindow);

} // namespace cocoa
} // namespace skr

#endif // __APPLE__