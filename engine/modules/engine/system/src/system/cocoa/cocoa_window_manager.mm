#include "cocoa_window_manager.h"
#include "cocoa_window.h"
#include "cocoa_monitor.h"
#include "SkrCore/log.h"
#include "SkrCore/memory/memory.h"

CocoaWindowManager* CocoaWindowManager::instance = nullptr;

// NSScreen notification observer
@interface ScreenChangeObserver : NSObject
{
    CocoaWindowManager* manager_;
}
- (instancetype)initWithManager:(CocoaWindowManager*)manager;
- (void)screenParametersChanged:(NSNotification*)notification;
@end

@implementation ScreenChangeObserver

- (instancetype)initWithManager:(CocoaWindowManager*)manager
{
    self = [super init];
    if (self) {
        manager_ = manager;
        
        // Register for screen change notifications
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(screenParametersChanged:)
                                                     name:NSApplicationDidChangeScreenParametersNotification
                                                   object:nil];
    }
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super dealloc];
}

- (void)screenParametersChanged:(NSNotification*)notification
{
    manager_->refresh_monitors();
}

@end

CocoaWindowManager::CocoaWindowManager() SKR_NOEXCEPT
{
    instance = this;
    
    // Initialize NSApplication if not already done
    if (!NSApp) {
        [NSApplication sharedApplication];
    }
    
    // Set activation policy to regular app (shows in dock)
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    
    // Initialize monitors
    initialize_monitors();
    
    // Set up screen change observer
    [[ScreenChangeObserver alloc] initWithManager:this];
}

CocoaWindowManager::~CocoaWindowManager() SKR_NOEXCEPT
{
    // Clean up all windows
    destroy_all_windows();
    
    // Clean up monitors
    for (auto* monitor : monitors_) {
        SkrDelete(monitor);
    }
    monitors_.clear();
    
    instance = nullptr;
}

uint32_t CocoaWindowManager::get_monitor_count() const SKR_NOEXCEPT
{
    return (uint32_t)monitors_.size();
}

skr::SystemMonitor* CocoaWindowManager::get_monitor(uint32_t index) const SKR_NOEXCEPT
{
    if (index >= monitors_.size()) {
        SKR_LOG_ERROR(u8"[CocoaWindowManager] Invalid monitor index: %u", index);
        return nullptr;
    }
    return monitors_[index];
}

skr::SystemMonitor* CocoaWindowManager::get_primary_monitor() const SKR_NOEXCEPT
{
    NSScreen* mainScreen = [NSScreen mainScreen];
    if (!mainScreen) {
        return nullptr;
    }
    
    for (auto* monitor : monitors_) {
        CocoaMonitor* cocoa_monitor = static_cast<CocoaMonitor*>(monitor);
        if (cocoa_monitor->get_nsscreen() == mainScreen) {
            return monitor;
        }
    }
    
    return nullptr;
}

skr::SystemMonitor* CocoaWindowManager::get_monitor_from_point(int32_t x, int32_t y) const SKR_NOEXCEPT
{
    // Convert to Cocoa coordinate system (bottom-left origin)
    NSPoint point = NSMakePoint(x, y);
    
    // Find screen containing point
    for (NSScreen* screen in [NSScreen screens]) {
        NSRect frame = [screen frame];
        if (NSPointInRect(point, frame)) {
            // Find our monitor wrapper
            for (auto* monitor : monitors_) {
                CocoaMonitor* cocoa_monitor = static_cast<CocoaMonitor*>(monitor);
                if (cocoa_monitor->get_nsscreen() == screen) {
                    return monitor;
                }
            }
        }
    }
    
    return nullptr;
}

skr::SystemMonitor* CocoaWindowManager::get_monitor_from_window(skr::SystemWindow* window) const SKR_NOEXCEPT
{
    if (!window) {
        return nullptr;
    }
    
    CocoaWindow* cocoa_window = static_cast<CocoaWindow*>(window);
    NSWindow* nsWindow = cocoa_window->get_nswindow();
    if (!nsWindow) {
        return nullptr;
    }
    
    NSScreen* screen = [nsWindow screen];
    if (!screen) {
        // Window might be off-screen, use main screen
        screen = [NSScreen mainScreen];
    }
    
    // Find our monitor wrapper
    for (auto* monitor : monitors_) {
        CocoaMonitor* cocoa_monitor = static_cast<CocoaMonitor*>(monitor);
        if (cocoa_monitor->get_nsscreen() == screen) {
            return monitor;
        }
    }
    
    return nullptr;
}

void CocoaWindowManager::refresh_monitors() SKR_NOEXCEPT
{
    @autoreleasepool {
        NSArray<NSScreen*>* screens = [NSScreen screens];
        
        // Update existing monitors and add new ones
        skr::Vector<CocoaMonitor*> new_monitors;
        
        for (NSScreen* screen in screens) {
            bool found = false;
            
            // Check if we already have this screen
            for (auto* monitor : monitors_) {
                CocoaMonitor* cocoa_monitor = static_cast<CocoaMonitor*>(monitor);
                if (cocoa_monitor->get_nsscreen() == screen) {
                    // Update existing monitor
                    cocoa_monitor->update_screen(screen);
                    new_monitors.add(cocoa_monitor);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                // Add new monitor
                CocoaMonitor* new_monitor = SkrNew<CocoaMonitor>(screen);
                new_monitors.add(new_monitor);
            }
        }
        
        // Delete monitors that no longer exist
        for (auto* monitor : monitors_) {
            if (!new_monitors.contains(static_cast<CocoaMonitor*>(monitor))) {
                SkrDelete(monitor);
            }
        }
        
        // Update our list
        monitors_ = std::move(new_monitors);
        
        SKR_LOG_INFO(u8"[CocoaWindowManager] Refreshed monitors, count: %u", (uint32_t)monitors_.size());
    }
}

skr::SystemWindow* CocoaWindowManager::create_window_internal(const skr::SystemWindowCreateInfo& info) SKR_NOEXCEPT
{
    @autoreleasepool {
        CocoaWindow* window = SkrNew<CocoaWindow>(info);
        
        // Add to our window map for fast lookup
        NSWindow* nsWindow = window->get_nswindow();
        if (nsWindow) {
            window_map_.add(nsWindow, window);
            
            // Connect event source if available
            if (event_source_) {
                window->set_event_source(event_source_);
            }
        }
        
        return window;
    }
}

void CocoaWindowManager::destroy_window_internal(skr::SystemWindow* window) SKR_NOEXCEPT
{
    if (!window) {
        return;
    }
    
    @autoreleasepool {
        CocoaWindow* cocoa_window = static_cast<CocoaWindow*>(window);
        NSWindow* nsWindow = cocoa_window->get_nswindow();
        
        // Remove from our map
        if (nsWindow) {
            window_map_.remove(nsWindow);
        }
        
        // Delete the window
        SkrDelete(cocoa_window);
    }
}

void CocoaWindowManager::initialize_monitors() SKR_NOEXCEPT
{
    @autoreleasepool {
        NSArray<NSScreen*>* screens = [NSScreen screens];
        
        monitors_.clear();
        monitors_.reserve(screens.count);
        
        for (NSScreen* screen in screens) {
            CocoaMonitor* monitor = SkrNew<CocoaMonitor>(screen);
            monitors_.add(monitor);
        }
        
        SKR_LOG_INFO(u8"[CocoaWindowManager] Initialized with %u monitors", (uint32_t)monitors_.size());
    }
}

NSScreen* CocoaWindowManager::get_nsscreen_from_monitor(skr::SystemMonitor* monitor) const SKR_NOEXCEPT
{
    if (!monitor) {
        return nil;
    }
    
    CocoaMonitor* cocoa_monitor = static_cast<CocoaMonitor*>(monitor);
    return cocoa_monitor->get_nsscreen();
}

void CocoaWindowManager::connect_event_source(CocoaEventSource* source) SKR_NOEXCEPT
{
    event_source_ = source;
    
    // Connect event source to all existing windows
    for (const auto& [nsWindow, window] : window_map_) {
        window->set_event_source(source);
    }
}