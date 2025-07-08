#pragma once
#include "SkrBase/math/manual/vector.hpp"
#include "SkrContainersDef/string.hpp"

namespace skr {

struct SystemWindowCreateInfo
{
    skr::String title = u8"Untitled";
    skr::Optional<uint2> pos;  // nullopt = system decides
    uint2 size = {800, 600};   // Default window size
    bool is_topmost = false;
    bool is_tooltip = false;
    bool is_borderless = false;
    bool is_resizable  = true;
};

enum class ESystemMonitorOrientation : uint8_t
{
    Unknown = 0,
    Portrait,
    Landscape,
    PortraitFlipped,
    LandscapeFlipped
};

struct SystemMonitorDisplayMode
{
    uint2 resolution;        // Width and height in pixels
    uint32_t refresh_rate;   // Refresh rate in Hz
    uint32_t bits_per_pixel; // Color depth
};

struct SystemMonitorInfo
{
    skr::String name;                          // Monitor name/identifier
    uint2 position;                            // Position in virtual desktop space
    uint2 size;                                // Size in pixels
    uint2 work_area_pos;                       // Work area position (excluding taskbar etc.)
    uint2 work_area_size;                      // Work area size
    float dpi_scale;                           // DPI scaling factor
    ESystemMonitorOrientation orientation;     // Current orientation
    bool is_primary;                           // Is this the primary monitor?
    void* native_handle;                       // Platform-specific handle
};

struct SKR_SYSTEM_API SystemMonitor
{
    virtual ~SystemMonitor() = default;
    
    // Get monitor information
    virtual SystemMonitorInfo get_info() const = 0;
    
    // Get available display modes
    virtual uint32_t get_display_mode_count() const = 0;
    virtual SystemMonitorDisplayMode get_display_mode(uint32_t index) const = 0;
    virtual SystemMonitorDisplayMode get_current_display_mode() const = 0;
};

struct SKR_SYSTEM_API SystemWindow
{
    virtual ~SystemWindow() = default;
    
    // Window properties
    virtual void set_title(const skr::String& title) = 0;
    virtual skr::String get_title() const = 0;
    
    virtual void set_position(int32_t x, int32_t y) = 0;
    virtual int2 get_position() const = 0;
    
    virtual void set_size(uint32_t width, uint32_t height) = 0;
    virtual uint2 get_size() const = 0;
    
    // Physical size and DPI
    virtual uint2 get_physical_size() const = 0;  // Get physical/drawable size in pixels
    virtual float get_pixel_ratio() const = 0;     // Get DPI scale factor (physical pixels / logical points)
    
    // Window state
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void minimize() = 0;
    virtual void maximize() = 0;
    virtual void restore() = 0;
    virtual void focus() = 0;
    
    virtual bool is_visible() const = 0;
    virtual bool is_minimized() const = 0;
    virtual bool is_maximized() const = 0;
    virtual bool is_focused() const = 0;
    
    // Opacity
    virtual void set_opacity(float opacity) = 0;
    virtual float get_opacity() const = 0;
    
    // Fullscreen
    virtual void set_fullscreen(bool fullscreen, SystemMonitor* monitor = nullptr) = 0;
    virtual bool is_fullscreen() const = 0;
    
    // Platform handle
    virtual void* get_native_handle() const = 0;
    virtual void* get_native_display() const = 0; // For X11, Wayland etc.
};

} // namespace skr