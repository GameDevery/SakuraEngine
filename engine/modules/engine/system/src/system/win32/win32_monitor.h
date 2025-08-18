#pragma once
#include "SkrSystem/window.h"
#include <Windows.h>

namespace skr {

class Win32Monitor : public SystemMonitor
{
public:
    Win32Monitor(HMONITOR hMonitor) SKR_NOEXCEPT;
    ~Win32Monitor() SKR_NOEXCEPT override = default;

    // SystemMonitor interface
    SystemMonitorInfo get_info() const override;
    uint32_t get_display_mode_count() const override;
    SystemMonitorDisplayMode get_display_mode(uint32_t index) const override;
    SystemMonitorDisplayMode get_current_display_mode() const override;
    
    // Win32 specific
    HMONITOR get_handle() const { return hMonitor_; }
    
private:
    void cache_monitor_info();
    void enumerate_display_modes();
    
private:
    HMONITOR hMonitor_;
    MONITORINFOEXW monitor_info_ = {};
    skr::Vector<SystemMonitorDisplayMode> display_modes_;
    SystemMonitorDisplayMode current_mode_ = {};
    
    // DPI info
    uint32_t dpi_x_ = 96;
    uint32_t dpi_y_ = 96;
};

} // namespace skr