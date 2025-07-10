#include "cocoa_monitor.h"
#include "SkrCore/log.h"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreVideo/CoreVideo.h>
#include <string>

CocoaMonitor::CocoaMonitor(NSScreen* screen) SKR_NOEXCEPT
    : screen_(screen)
{
}

skr::SystemMonitorInfo CocoaMonitor::get_info() const SKR_NOEXCEPT
{
    @autoreleasepool {
        skr::SystemMonitorInfo info = {};
        
        if (!screen_) {
            SKR_LOG_ERROR(u8"[CocoaMonitor] Invalid NSScreen");
            return info;
        }
        
        // Get screen frame and visible frame
        NSRect frame = [screen_ frame];
        NSRect visibleFrame = [screen_ visibleFrame];
        
        // Position and size
        info.position.x = (int32_t)frame.origin.x;
        info.position.y = (int32_t)frame.origin.y;
        info.size.x = (uint32_t)frame.size.width;
        info.size.y = (uint32_t)frame.size.height;
        
        // Work area (visible frame excluding dock and menu bar)
        info.work_area_pos.x = (int32_t)visibleFrame.origin.x;
        info.work_area_pos.y = (int32_t)visibleFrame.origin.y;
        info.work_area_size.x = (uint32_t)visibleFrame.size.width;
        info.work_area_size.y = (uint32_t)visibleFrame.size.height;
        
        // DPI scale factor (for HiDPI/Retina displays)
        CGFloat backingScaleFactor = [screen_ backingScaleFactor];
        info.dpi_scale = (float)backingScaleFactor;
        
        // Monitor name
        // Get display ID and name
        NSDictionary* deviceDescription = [screen_ deviceDescription];
        NSNumber* screenNumber = [deviceDescription objectForKey:@"NSScreenNumber"];
        CGDirectDisplayID displayID = (CGDirectDisplayID)[screenNumber unsignedIntValue];
        
        // Try to get the display name
        NSString* displayName = nil;
        
        // Try using CGDisplayCopyDisplayName (macOS 10.15+)
        if (@available(macOS 10.15, *)) {
            // CGDisplayCopyDisplayName is not available in public API
            CFStringRef cfName = nullptr;
            if (cfName) {
                displayName = (__bridge_transfer NSString*)cfName;
            }
        }
        
        // Fallback to localized name or generic name
        if (!displayName) {
            displayName = [screen_ localizedName];
        }
        
        if (displayName) {
            info.name = skr::String((const char8_t*)[displayName UTF8String]);
        } else {
            info.name = skr::format(u8"Display {}", displayID);
        }
        
        // Determine orientation based on dimensions
        // While macOS doesn't expose rotation in public API, we can infer from dimensions
        if (info.size.x > info.size.y) {
            info.orientation = skr::ESystemMonitorOrientation::Landscape;
        } else if (info.size.y > info.size.x) {
            info.orientation = skr::ESystemMonitorOrientation::Portrait;
        } else {
            // Square display
            info.orientation = skr::ESystemMonitorOrientation::Landscape;
        }
        
        // Check if this is the primary monitor
        info.is_primary = (screen_ == [NSScreen mainScreen]);
        
        // Native handle (NSScreen pointer)
        info.native_handle = (__bridge void*)screen_;
        
        return info;
    }
}

uint32_t CocoaMonitor::get_display_mode_count() const SKR_NOEXCEPT
{
    @autoreleasepool {
        if (!screen_) {
            return 0;
        }
        
        // Get display ID
        NSDictionary* deviceDescription = [screen_ deviceDescription];
        NSNumber* screenNumber = [deviceDescription objectForKey:@"NSScreenNumber"];
        CGDirectDisplayID displayID = (CGDirectDisplayID)[screenNumber unsignedIntValue];
        
        // Get all display modes
        CFArrayRef modes = CGDisplayCopyAllDisplayModes(displayID, nullptr);
        if (!modes) {
            return 0;
        }
        
        CFIndex count = CFArrayGetCount(modes);
        CFRelease(modes);
        
        return (uint32_t)count;
    }
}

skr::SystemMonitorDisplayMode CocoaMonitor::get_display_mode(uint32_t index) const SKR_NOEXCEPT
{
    @autoreleasepool {
        skr::SystemMonitorDisplayMode mode = {};
        
        if (!screen_) {
            return mode;
        }
        
        // Get display ID
        NSDictionary* deviceDescription = [screen_ deviceDescription];
        NSNumber* screenNumber = [deviceDescription objectForKey:@"NSScreenNumber"];
        CGDirectDisplayID displayID = (CGDirectDisplayID)[screenNumber unsignedIntValue];
        
        // Get all display modes
        CFArrayRef modes = CGDisplayCopyAllDisplayModes(displayID, nullptr);
        if (!modes) {
            return mode;
        }
        
        CFIndex count = CFArrayGetCount(modes);
        if (index >= count) {
            CFRelease(modes);
            return mode;
        }
        
        // Get the specific mode
        CGDisplayModeRef displayMode = (CGDisplayModeRef)CFArrayGetValueAtIndex(modes, index);
        
        // Fill mode information
        mode.resolution.x = (uint32_t)CGDisplayModeGetWidth(displayMode);
        mode.resolution.y = (uint32_t)CGDisplayModeGetHeight(displayMode);
        
        // Get refresh rate
        double refreshRate = CGDisplayModeGetRefreshRate(displayMode);
        if (refreshRate == 0) {
            // Some displays (especially external displays) don't report refresh rate through CGDisplayModeGetRefreshRate
            // Try to get it through CVDisplayLink
            CVDisplayLinkRef displayLink;
            if (CVDisplayLinkCreateWithCGDisplay(displayID, &displayLink) == kCVReturnSuccess) {
                const CVTime time = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(displayLink);
                if (time.timeValue != 0 && time.timeScale != 0) {
                    refreshRate = (double)time.timeScale / (double)time.timeValue;
                } else {
                    // Fallback to common refresh rate
                    refreshRate = 60.0;
                }
                CVDisplayLinkRelease(displayLink);
            } else {
                // Fallback to common refresh rate
                refreshRate = 60.0;
            }
        }
        mode.refresh_rate = (uint32_t)refreshRate;
        
        // Bits per pixel
        CFStringRef pixelEncoding = CGDisplayModeCopyPixelEncoding(displayMode);
        if (pixelEncoding) {
            // Common pixel formats
            if (CFStringCompare(pixelEncoding, CFSTR(IO32BitDirectPixels), 0) == kCFCompareEqualTo) {
                mode.bits_per_pixel = 32;
            } else if (CFStringCompare(pixelEncoding, CFSTR(IO16BitDirectPixels), 0) == kCFCompareEqualTo) {
                mode.bits_per_pixel = 16;
            } else if (CFStringCompare(pixelEncoding, CFSTR(IO8BitIndexedPixels), 0) == kCFCompareEqualTo) {
                mode.bits_per_pixel = 8;
            } else {
                mode.bits_per_pixel = 32; // Default
            }
            CFRelease(pixelEncoding);
        } else {
            mode.bits_per_pixel = 32; // Default
        }
        
        CFRelease(modes);
        return mode;
    }
}

skr::SystemMonitorDisplayMode CocoaMonitor::get_current_display_mode() const SKR_NOEXCEPT
{
    @autoreleasepool {
        skr::SystemMonitorDisplayMode mode = {};
        
        if (!screen_) {
            return mode;
        }
        
        // Get display ID
        NSDictionary* deviceDescription = [screen_ deviceDescription];
        NSNumber* screenNumber = [deviceDescription objectForKey:@"NSScreenNumber"];
        CGDirectDisplayID displayID = (CGDirectDisplayID)[screenNumber unsignedIntValue];
        
        // Get current display mode
        CGDisplayModeRef currentMode = CGDisplayCopyDisplayMode(displayID);
        if (!currentMode) {
            return mode;
        }
        
        // Fill mode information
        mode.resolution.x = (uint32_t)CGDisplayModeGetWidth(currentMode);
        mode.resolution.y = (uint32_t)CGDisplayModeGetHeight(currentMode);
        
        // Get refresh rate
        double refreshRate = CGDisplayModeGetRefreshRate(currentMode);
        if (refreshRate == 0) {
            // Some displays (especially external displays) don't report refresh rate through CGDisplayModeGetRefreshRate
            // Try to get it through CVDisplayLink
            CVDisplayLinkRef displayLink;
            if (CVDisplayLinkCreateWithCGDisplay(displayID, &displayLink) == kCVReturnSuccess) {
                const CVTime time = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(displayLink);
                if (time.timeValue != 0 && time.timeScale != 0) {
                    refreshRate = (double)time.timeScale / (double)time.timeValue;
                } else {
                    // Fallback to common refresh rate
                    refreshRate = 60.0;
                }
                CVDisplayLinkRelease(displayLink);
            } else {
                // Fallback to common refresh rate
                refreshRate = 60.0;
            }
        }
        mode.refresh_rate = (uint32_t)refreshRate;
        
        // Bits per pixel
        CFStringRef pixelEncoding = CGDisplayModeCopyPixelEncoding(currentMode);
        if (pixelEncoding) {
            // Common pixel formats
            if (CFStringCompare(pixelEncoding, CFSTR(IO32BitDirectPixels), 0) == kCFCompareEqualTo) {
                mode.bits_per_pixel = 32;
            } else if (CFStringCompare(pixelEncoding, CFSTR(IO16BitDirectPixels), 0) == kCFCompareEqualTo) {
                mode.bits_per_pixel = 16;
            } else if (CFStringCompare(pixelEncoding, CFSTR(IO8BitIndexedPixels), 0) == kCFCompareEqualTo) {
                mode.bits_per_pixel = 8;
            } else {
                mode.bits_per_pixel = 32; // Default
            }
            CFRelease(pixelEncoding);
        } else {
            mode.bits_per_pixel = 32; // Default
        }
        
        CGDisplayModeRelease(currentMode);
        return mode;
    }
}