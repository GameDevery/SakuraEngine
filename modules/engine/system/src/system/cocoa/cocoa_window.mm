#include "cocoa_window.h"
#include "cocoa_event_source.h"
#include "cocoa_window_manager.h"
#include "cocoa_monitor.h"
#include "SkrCore/log.h"
#include "SkrCore/memory/memory.h"
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>
#include <ApplicationServices/ApplicationServices.h>  // For CGDisplay APIs

// Metal view similar to SDL's implementation
@interface CocoaMetalView : NSView
{
    BOOL highDPI_;
}
@property (nonatomic) BOOL highDPI;
- (instancetype)initWithFrame:(NSRect)frame highDPI:(BOOL)highDPI;
- (void)updateDrawableSize;
@end

@implementation CocoaMetalView

@synthesize highDPI = highDPI_;

+ (Class)layerClass
{
    return NSClassFromString(@"CAMetalLayer");
}

- (instancetype)initWithFrame:(NSRect)frame highDPI:(BOOL)highDPI
{
    self = [super initWithFrame:frame];
    if (self) {
        self.highDPI = highDPI;
        self.wantsLayer = YES;
        self.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        [self updateDrawableSize];
    }
    return self;
}

- (BOOL)wantsUpdateLayer
{
    return YES;
}

- (CALayer*)makeBackingLayer
{
    return [[[self class] layerClass] layer];
}

- (void)updateDrawableSize
{
    NSWindow* window = self.window;
    if (!window) return;
    
    NSSize viewSize = self.bounds.size;
    NSSize backingSize = viewSize;
    
    if (self.highDPI) {
        backingSize = [self convertSizeToBacking:viewSize];
    }
    
    CAMetalLayer* metalLayer = (CAMetalLayer*)self.layer;
    metalLayer.drawableSize = CGSizeMake(backingSize.width, backingSize.height);
    metalLayer.contentsScale = self.highDPI ? window.backingScaleFactor : 1.0;
}

- (void)viewDidMoveToWindow
{
    [super viewDidMoveToWindow];
    [self updateDrawableSize];
}

- (void)setFrameSize:(NSSize)size
{
    [super setFrameSize:size];
    [self updateDrawableSize];
}

@end

// Custom NSView for window content
@interface CocoaView : NSView
{
    skr::SystemWindow* systemWindow_;
}
@property (nonatomic, assign) skr::SystemWindow* systemWindow;
@end

@implementation CocoaView

@synthesize systemWindow = systemWindow_;

// Normal NSView - no special layer configuration

- (BOOL)wantsLayer
{
    return YES;
}

- (BOOL)wantsUpdateLayer
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)canBecomeKeyView
{
    return YES;
}

@end

// Window delegate for handling window events
@interface CocoaWindowDelegate : NSObject<NSWindowDelegate>
{
    CocoaWindow* window_;
    CocoaEventSource* event_source_;
}
- (instancetype)initWithWindow:(CocoaWindow*)window;
- (void)setEventSource:(CocoaEventSource*)source;
@end

@implementation CocoaWindowDelegate

- (instancetype)initWithWindow:(CocoaWindow*)window
{
    self = [super init];
    if (self) {
        window_ = window;
        event_source_ = nullptr; // Will be set later when connected
    }
    return self;
}

- (void)setEventSource:(CocoaEventSource*)source
{
    event_source_ = source;
}

- (void)windowDidResize:(NSNotification*)notification
{
    if (event_source_) {
        SkrSystemEvent event;
        event.type = SKR_SYSTEM_EVENT_WINDOW_RESIZED;
        event.window.window_native_handle = (uint64_t)(__bridge void*)window_->get_nswindow();
        
        NSRect contentRect = [window_->get_nswindow() contentRectForFrameRect:window_->get_nswindow().frame];
        event.window.x = (uint32_t)contentRect.size.width;
        event.window.y = (uint32_t)contentRect.size.height;
        
        event_source_->push_event(event);
    }
}

- (void)windowDidMove:(NSNotification*)notification
{
    if (event_source_) {
        SkrSystemEvent event;
        event.type = SKR_SYSTEM_EVENT_WINDOW_MOVED;
        event.window.window_native_handle = (uint64_t)(__bridge void*)window_->get_nswindow();
        
        NSRect frame = window_->get_nswindow().frame;
        event.window.x = (uint32_t)frame.origin.x;
        event.window.y = (uint32_t)frame.origin.y;
        
        event_source_->push_event(event);
    }
}

- (void)windowDidBecomeKey:(NSNotification*)notification
{
    if (event_source_) {
        SkrSystemEvent event;
        event.type = SKR_SYSTEM_EVENT_WINDOW_FOCUS_GAINED;
        event.window.window_native_handle = (uint64_t)(__bridge void*)window_->get_nswindow();
        
        event_source_->push_event(event);
    }
}

- (void)windowDidResignKey:(NSNotification*)notification
{
    if (event_source_) {
        SkrSystemEvent event;
        event.type = SKR_SYSTEM_EVENT_WINDOW_FOCUS_LOST;
        event.window.window_native_handle = (uint64_t)(__bridge void*)window_->get_nswindow();
        
        event_source_->push_event(event);
    }
}

- (void)windowDidMiniaturize:(NSNotification*)notification
{
    if (event_source_) {
        SkrSystemEvent event;
        event.type = SKR_SYSTEM_EVENT_WINDOW_MINIMIZED;
        event.window.window_native_handle = (uint64_t)(__bridge void*)window_->get_nswindow();
        
        event_source_->push_event(event);
    }
}

- (void)windowDidDeminiaturize:(NSNotification*)notification
{
    if (event_source_) {
        SkrSystemEvent event;
        event.type = SKR_SYSTEM_EVENT_WINDOW_RESTORED;
        event.window.window_native_handle = (uint64_t)(__bridge void*)window_->get_nswindow();
        
        event_source_->push_event(event);
    }
}

- (BOOL)windowShouldClose:(NSWindow*)sender
{
    if (event_source_) {
        SkrSystemEvent event;
        event.type = SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED;
        event.window.window_native_handle = (uint64_t)(__bridge void*)window_->get_nswindow();
        
        event_source_->push_event(event);
    }
    return NO; // Don't close automatically, let the app handle it
}

- (void)windowDidChangeBackingProperties:(NSNotification*)notification
{
    // DPI change notifications not currently supported in the event system
    // The window maintains the current DPI through get_pixel_ratio()
}

@end

CocoaWindow::CocoaWindow(const skr::SystemWindowCreateInfo& info) SKR_NOEXCEPT
    : window_(nil)
    , delegate_(nil)
    , view_(nil)
    , metal_view_(nil)
    , was_maximized_before_fullscreen_(false)
{
    @autoreleasepool {
        // Determine window style
        NSWindowStyleMask styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
        
        if (info.is_resizable) {
            styleMask |= NSWindowStyleMaskResizable;
        }
        if (info.is_borderless) {
            styleMask = NSWindowStyleMaskBorderless;
        }
        
        // Get the target screen for DPI-aware positioning
        NSScreen* targetScreen = nil;
        if (info.pos.has_value()) {
            // Find screen containing the specified position
            NSPoint point = NSMakePoint(info.pos->x, info.pos->y);
            for (NSScreen* screen in [NSScreen screens]) {
                NSRect frame = [screen frame];
                if (NSPointInRect(point, frame)) {
                    targetScreen = screen;
                    break;
                }
            }
        }
        if (!targetScreen) {
            targetScreen = [NSScreen mainScreen];
        }
        
        // Calculate content rect - note that size is in logical points
        // Create window at default position first, then move it
        // This approach is similar to SDL3 and avoids some macOS positioning constraints
        NSRect contentRect = NSMakeRect(
            0, 0,  // Default position, will be set later
            info.size.x, info.size.y
        );
        
        // Create window
        window_ = [[NSWindow alloc] initWithContentRect:contentRect
                                              styleMask:styleMask
                                                backing:NSBackingStoreBuffered
                                                  defer:NO
                                                 screen:targetScreen];
        
        if (!window_) {
            SKR_LOG_ERROR(u8"[CocoaWindow] Failed to create NSWindow");
            return;
        }
        
        // Set window properties
        [window_ setTitle:[NSString stringWithUTF8String:(const char*)info.title.c_str()]];
        [window_ setAcceptsMouseMovedEvents:YES];
        [window_ setReleasedWhenClosed:NO]; // We manage the lifetime
        
        // Configure DPI awareness - macOS windows are automatically DPI-aware
        // The backing scale factor will be determined by the screen
        CGFloat backingScaleFactor = [window_ backingScaleFactor];
        SKR_LOG_DEBUG(u8"[CocoaWindow] Created window with backing scale factor: %.1f", backingScaleFactor);
        
        // Create and set Metal view
        view_ = [[CocoaView alloc] initWithFrame:contentRect];
        view_.systemWindow = this;
        [window_ setContentView:view_];
        
        // Set up delegate
        delegate_ = [[CocoaWindowDelegate alloc] initWithWindow:this];
        [window_ setDelegate:delegate_];
        
        // Handle initial visibility
        [window_ makeKeyAndOrderFront:nil];
        
        // Handle always on top
        if (info.is_topmost) {
            [window_ setLevel:NSFloatingWindowLevel];
        }

        create_metal_view();
        
        // macOS windows are high-DPI aware by default
        // The rendering backend will handle layer configuration
        
        // Set window position after creation (like SDL3 does)
        if (info.pos.has_value()) {
            set_position(info.pos->x, info.pos->y);
        } else {
            // Center window on screen if no position specified
            NSRect screenFrame = [targetScreen frame];
            NSRect windowFrame = [window_ frame];
            CGFloat x = screenFrame.origin.x + (screenFrame.size.width - windowFrame.size.width) / 2;
            CGFloat y = screenFrame.origin.y + (screenFrame.size.height - windowFrame.size.height) / 2;
            [window_ setFrameOrigin:NSMakePoint(x, y)];
        }
        
        SKR_LOG_INFO(u8"[CocoaWindow] Created window: %s", info.title.c_str());
    }
}

CocoaWindow::~CocoaWindow() SKR_NOEXCEPT
{
    @autoreleasepool {
        if (metal_view_) {
            [metal_view_ removeFromSuperview];
            [metal_view_ release];
        }
        
        if (window_) {
            [window_ orderOut:nil];
            [window_ setDelegate:nil];
            [window_ close];
            [window_ release];
        }
        
        if (delegate_) {
            [delegate_ release];
        }
        
        if (view_) {
            [view_ release];
        }
    }
}

void CocoaWindow::set_title(const skr::String& title) SKR_NOEXCEPT
{
    @autoreleasepool {
        [window_ setTitle:[NSString stringWithUTF8String:(const char*)title.c_str()]];
    }
}

skr::String CocoaWindow::get_title() const SKR_NOEXCEPT
{
    @autoreleasepool {
        return skr::String((const char8_t*)[window_ title].UTF8String);
    }
}

void CocoaWindow::set_position(int32_t x, int32_t y) SKR_NOEXCEPT
{
    @autoreleasepool {
        // Use CGDisplay API for more consistent coordinate handling (like SDL3)
        CGFloat mainDisplayHeight = CGDisplayPixelsHigh(kCGDirectMainDisplay);
        
        // Convert to points (macOS uses points, not pixels for window positioning)
        NSScreen* mainScreen = [NSScreen mainScreen];
        CGFloat scaleFactor = [mainScreen backingScaleFactor];
        CGFloat heightInPoints = mainDisplayHeight / scaleFactor;
        
        // Get window frame to preserve size
        NSRect frame = [window_ frame];
        
        // Set position in logical points
        frame.origin.x = x;
        // Convert Y from top-left origin to bottom-left origin
        frame.origin.y = heightInPoints - y - frame.size.height;
        
        // Use setFrame instead of setFrameOrigin for more complete update
        [window_ setFrame:frame display:YES animate:NO];
    }
}

skr::math::int2 CocoaWindow::get_position() const SKR_NOEXCEPT
{
    @autoreleasepool {
        NSRect frame = [window_ frame];
        
        // Use CGDisplay API for consistent coordinate handling
        CGFloat mainDisplayHeight = CGDisplayPixelsHigh(kCGDirectMainDisplay);
        
        // Convert to points
        NSScreen* mainScreen = [NSScreen mainScreen];
        if (!mainScreen) {
            return {0, 0};
        }
        CGFloat scaleFactor = [mainScreen backingScaleFactor];
        CGFloat heightInPoints = mainDisplayHeight / scaleFactor;
        
        // Convert from bottom-left to top-left origin
        return {
            (int32_t)frame.origin.x,
            (int32_t)(heightInPoints - frame.origin.y - frame.size.height)
        };
    }
}

void CocoaWindow::set_size(uint32_t width, uint32_t height) SKR_NOEXCEPT
{
    @autoreleasepool {
        // Set content size, not frame size
        // This ensures the client area matches the requested size
        NSRect contentRect = [window_ contentRectForFrameRect:[window_ frame]];
        contentRect.size.width = width;
        contentRect.size.height = height;
        
        // Convert to frame rect
        NSRect frameRect = [window_ frameRectForContentRect:contentRect];
        [window_ setFrame:frameRect display:YES];
    }
}

skr::math::uint2 CocoaWindow::get_size() const SKR_NOEXCEPT
{
    @autoreleasepool {
        NSRect contentRect = [window_ contentRectForFrameRect:[window_ frame]];
        return {
            (uint32_t)contentRect.size.width,
            (uint32_t)contentRect.size.height
        };
    }
}

skr::math::uint2 CocoaWindow::get_physical_size() const SKR_NOEXCEPT
{
    @autoreleasepool {
        // If we have a Metal view, return its drawable size
        // This is what rendering backends actually need
        if (metal_view_) {
            CAMetalLayer* metalLayer = (CAMetalLayer*)[metal_view_ layer];
            CGSize drawableSize = metalLayer.drawableSize;
            return {
                (uint32_t)drawableSize.width,
                (uint32_t)drawableSize.height
            };
        }
        
        // Otherwise calculate from window content rect
        NSRect contentRect = [window_ contentRectForFrameRect:[window_ frame]];
        CGFloat backingScale = [window_ backingScaleFactor];
        
        return {
            (uint32_t)(contentRect.size.width * backingScale),
            (uint32_t)(contentRect.size.height * backingScale)
        };
    }
}

float CocoaWindow::get_pixel_ratio() const SKR_NOEXCEPT
{
    @autoreleasepool {
        return (float)[window_ backingScaleFactor];
    }
}

void CocoaWindow::show() SKR_NOEXCEPT
{
    @autoreleasepool {
        [window_ makeKeyAndOrderFront:nil];
    }
}

void CocoaWindow::hide() SKR_NOEXCEPT
{
    @autoreleasepool {
        [window_ orderOut:nil];
    }
}

void CocoaWindow::minimize() SKR_NOEXCEPT
{
    @autoreleasepool {
        [window_ miniaturize:nil];
    }
}

void CocoaWindow::maximize() SKR_NOEXCEPT
{
    @autoreleasepool {
        if (![window_ isZoomed]) {
            [window_ zoom:nil];
        }
    }
}

void CocoaWindow::restore() SKR_NOEXCEPT
{
    @autoreleasepool {
        if ([window_ isMiniaturized]) {
            [window_ deminiaturize:nil];
        } else if ([window_ isZoomed]) {
            [window_ zoom:nil];
        }
    }
}

void CocoaWindow::focus() SKR_NOEXCEPT
{
    @autoreleasepool {
        [window_ makeKeyWindow];
        [NSApp activateIgnoringOtherApps:YES];
    }
}

bool CocoaWindow::is_visible() const SKR_NOEXCEPT
{
    @autoreleasepool {
        return [window_ isVisible];
    }
}

bool CocoaWindow::is_minimized() const SKR_NOEXCEPT
{
    @autoreleasepool {
        return [window_ isMiniaturized];
    }
}

bool CocoaWindow::is_maximized() const SKR_NOEXCEPT
{
    @autoreleasepool {
        return [window_ isZoomed];
    }
}

bool CocoaWindow::is_focused() const SKR_NOEXCEPT
{
    @autoreleasepool {
        return [window_ isKeyWindow];
    }
}

void CocoaWindow::set_opacity(float opacity) SKR_NOEXCEPT
{
    @autoreleasepool {
        [window_ setAlphaValue:opacity];
    }
}

float CocoaWindow::get_opacity() const SKR_NOEXCEPT
{
    @autoreleasepool {
        return (float)[window_ alphaValue];
    }
}

void CocoaWindow::set_fullscreen(bool fullscreen, skr::SystemMonitor* monitor) SKR_NOEXCEPT
{
    @autoreleasepool {
        bool is_fullscreen = ([window_ styleMask] & NSWindowStyleMaskFullScreen) != 0;
        
        if (fullscreen != is_fullscreen) {
            if (fullscreen) {
                // Save state before fullscreen
                was_maximized_before_fullscreen_ = is_maximized();
                restore_frame_ = [window_ frame];
                
                // If a specific monitor is requested, move window to that monitor first
                if (monitor) {
                    CocoaMonitor* cocoa_monitor = static_cast<CocoaMonitor*>(monitor);
                    NSScreen* targetScreen = cocoa_monitor->get_nsscreen();
                    
                    if (targetScreen && targetScreen != [window_ screen]) {
                        // Get the target screen's frame
                        NSRect screenFrame = [targetScreen frame];
                        
                        // Calculate window position to center it on the target screen
                        NSRect windowFrame = [window_ frame];
                        NSPoint newOrigin;
                        newOrigin.x = screenFrame.origin.x + (screenFrame.size.width - windowFrame.size.width) / 2;
                        newOrigin.y = screenFrame.origin.y + (screenFrame.size.height - windowFrame.size.height) / 2;
                        
                        // Move window to target screen
                        [window_ setFrameOrigin:newOrigin];
                        
                        // Ensure the window updates its screen property
                        [window_ orderFront:nil];
                    }
                }
            }
            
            // Toggle fullscreen - window will go fullscreen on its current screen
            [window_ toggleFullScreen:nil];
        }
    }
}

bool CocoaWindow::is_fullscreen() const SKR_NOEXCEPT
{
    @autoreleasepool {
        return ([window_ styleMask] & NSWindowStyleMaskFullScreen) != 0;
    }
}

void* CocoaWindow::get_native_handle() const SKR_NOEXCEPT
{
    return (__bridge void*)window_;
}

void* CocoaWindow::get_native_display() const SKR_NOEXCEPT
{
    // Not applicable for macOS
    return nullptr;
}

void* CocoaWindow::get_native_view() const SKR_NOEXCEPT
{
    // For compatibility, return the main view
    // Rendering backends should call create_metal_view() for Metal support
    return (__bridge void*)metal_view_;
}

void* CocoaWindow::create_metal_view() SKR_NOEXCEPT
{
    @autoreleasepool {
        if (!metal_view_) {
            // Create Metal view similar to SDL's implementation
            NSRect viewFrame = [view_ bounds];
            BOOL highDPI = YES; // Always enable high DPI for Metal
            
            metal_view_ = [[CocoaMetalView alloc] initWithFrame:viewFrame highDPI:highDPI];
            [view_ addSubview:metal_view_];
            
            SKR_LOG_INFO(u8"[CocoaWindow] Created Metal view for window");
        }
        return (__bridge void*)metal_view_;
    }
}

void* CocoaWindow::get_metal_layer() SKR_NOEXCEPT
{
    @autoreleasepool {
        // Create Metal view if it doesn't exist
        create_metal_view();
        
        if (metal_view_) {
            return (__bridge void*)[metal_view_ layer];
        }
        return nullptr;
    }
}

void CocoaWindow::set_event_source(CocoaEventSource* source) SKR_NOEXCEPT
{
    @autoreleasepool {
        [delegate_ setEventSource:source];
    }
}