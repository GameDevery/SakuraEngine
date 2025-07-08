#include "win32_monitor.h"
#include "SkrCore/log.h"
#include <ShellScalingApi.h>

namespace skr {

Win32Monitor::Win32Monitor(HMONITOR hMonitor) SKR_NOEXCEPT
    : hMonitor_(hMonitor)
{
    cache_monitor_info();
    enumerate_display_modes();
}

void Win32Monitor::cache_monitor_info()
{
    monitor_info_.cbSize = sizeof(monitor_info_);
    if (!GetMonitorInfoW(hMonitor_, &monitor_info_))
    {
        SKR_LOG_ERROR(u8"Failed to get monitor info");
        return;
    }
    
    // Get DPI
    UINT dpi_x = 96, dpi_y = 96;
    HRESULT hr = GetDpiForMonitor(hMonitor_, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
    if (SUCCEEDED(hr))
    {
        dpi_x_ = dpi_x;
        dpi_y_ = dpi_y;
    }
    
    // Get current display settings
    DEVMODEW dm = {};
    dm.dmSize = sizeof(dm);
    if (EnumDisplaySettingsW(monitor_info_.szDevice, ENUM_CURRENT_SETTINGS, &dm))
    {
        current_mode_.resolution = {dm.dmPelsWidth, dm.dmPelsHeight};
        current_mode_.refresh_rate = dm.dmDisplayFrequency;
        current_mode_.bits_per_pixel = dm.dmBitsPerPel;
    }
}

void Win32Monitor::enumerate_display_modes()
{
    display_modes_.clear();
    
    DEVMODEW dm = {};
    dm.dmSize = sizeof(dm);
    
    // Enumerate all display modes
    for (DWORD i = 0; EnumDisplaySettingsW(monitor_info_.szDevice, i, &dm); ++i)
    {
        // Filter out low resolutions and color depths
        if (dm.dmBitsPerPel >= 24 && dm.dmPelsWidth >= 640 && dm.dmPelsHeight >= 480)
        {
            SystemMonitorDisplayMode mode;
            mode.resolution = {dm.dmPelsWidth, dm.dmPelsHeight};
            mode.refresh_rate = dm.dmDisplayFrequency;
            mode.bits_per_pixel = dm.dmBitsPerPel;
            
            // Check if this mode already exists (different refresh rates)
            bool exists = false;
            for (const auto& existing : display_modes_)
            {
                if (existing.resolution.x == mode.resolution.x && 
                    existing.resolution.y == mode.resolution.y &&
                    existing.refresh_rate == mode.refresh_rate &&
                    existing.bits_per_pixel == mode.bits_per_pixel)
                {
                    exists = true;
                    break;
                }
            }
            
            if (!exists)
            {
                display_modes_.add(mode);
            }
        }
    }
    
    // Sort modes by resolution, then refresh rate
    display_modes_.sort([](const SystemMonitorDisplayMode& a, const SystemMonitorDisplayMode& b) {
        if (a.resolution.x != b.resolution.x) return a.resolution.x > b.resolution.x;
        if (a.resolution.y != b.resolution.y) return a.resolution.y > b.resolution.y;
        if (a.refresh_rate != b.refresh_rate) return a.refresh_rate > b.refresh_rate;
        return a.bits_per_pixel > b.bits_per_pixel;
    });
}

SystemMonitorInfo Win32Monitor::get_info() const
{
    SystemMonitorInfo info;
    
    // Convert device name from wide string
    int name_len = WideCharToMultiByte(CP_UTF8, 0, monitor_info_.szDevice, -1, nullptr, 0, nullptr, nullptr);
    skr::Vector<char> utf8_name;
    utf8_name.resize_default(name_len);
    WideCharToMultiByte(CP_UTF8, 0, monitor_info_.szDevice, -1, utf8_name.data(), name_len, nullptr, nullptr);
    info.name = (const char8_t*)utf8_name.data();
    
    // Work area (excluding taskbar)
    info.work_area_pos.x = monitor_info_.rcWork.left;
    info.work_area_pos.y = monitor_info_.rcWork.top;
    info.work_area_size.x = monitor_info_.rcWork.right - monitor_info_.rcWork.left;
    info.work_area_size.y = monitor_info_.rcWork.bottom - monitor_info_.rcWork.top;
    
    // Position and size
    info.position.x = monitor_info_.rcMonitor.left;
    info.position.y = monitor_info_.rcMonitor.top;
    info.size.x = monitor_info_.rcMonitor.right - monitor_info_.rcMonitor.left;
    info.size.y = monitor_info_.rcMonitor.bottom - monitor_info_.rcMonitor.top;
    
    // Note: size field is already set above as pixel dimensions
    // Physical size in mm would require additional fields
    
    // DPI scale factor
    info.dpi_scale = dpi_x_ / 96.0f;
    
    // Flags
    info.is_primary = (monitor_info_.dwFlags & MONITORINFOF_PRIMARY) != 0;
    
    // Windows doesn't provide orientation info easily
    info.orientation = ESystemMonitorOrientation::Landscape;
    if (info.size.y > info.size.x)
    {
        info.orientation = ESystemMonitorOrientation::Portrait;
    }
    
    // Native handle
    info.native_handle = hMonitor_;
    
    return info;
}

uint32_t Win32Monitor::get_display_mode_count() const
{
    return static_cast<uint32_t>(display_modes_.size());
}

SystemMonitorDisplayMode Win32Monitor::get_display_mode(uint32_t index) const
{
    if (index < display_modes_.size())
    {
        return display_modes_[index];
    }
    return {};
}

SystemMonitorDisplayMode Win32Monitor::get_current_display_mode() const
{
    return current_mode_;
}

} // namespace skr