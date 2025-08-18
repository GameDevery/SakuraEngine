#pragma once
#include "SkrSystem/window.h"

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#else
// Forward declarations for C++
typedef struct objc_object NSScreen;
#endif

class CocoaMonitor : public skr::SystemMonitor {
public:
    explicit CocoaMonitor(NSScreen* screen) SKR_NOEXCEPT;
    ~CocoaMonitor() SKR_NOEXCEPT override = default;

    // SystemMonitor implementation
    skr::SystemMonitorInfo get_info() const SKR_NOEXCEPT override;
    uint32_t get_display_mode_count() const SKR_NOEXCEPT override;
    skr::SystemMonitorDisplayMode get_display_mode(uint32_t index) const SKR_NOEXCEPT override;
    skr::SystemMonitorDisplayMode get_current_display_mode() const SKR_NOEXCEPT override;

    // Cocoa specific
    NSScreen* get_nsscreen() const SKR_NOEXCEPT { return screen_; }
    void update_screen(NSScreen* screen) SKR_NOEXCEPT { screen_ = screen; }

private:
    NSScreen* screen_;
};