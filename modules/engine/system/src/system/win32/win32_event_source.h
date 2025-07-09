#pragma once
#include "SkrSystem/system.h"
#include "SkrSystem/input.h"
#include "SkrContainers/map.hpp"
#include "SkrContainers/concurrent_queue.hpp"
#include <Windows.h>

namespace skr {

class Win32SystemApp;

class Win32EventSource : public ISystemEventSource
{
public:
    Win32EventSource(Win32SystemApp* app) SKR_NOEXCEPT;
    ~Win32EventSource() SKR_NOEXCEPT override;

    bool poll_event(SkrSystemEvent& event) SKR_NOEXCEPT override;
    
    void set_ime(IME* ime) SKR_NOEXCEPT { ime_ = ime; }

private:
    friend struct Win32Window;
    struct WindowMSG
    {
        HWND hwnd;
        UINT message;
        WPARAM wParam;
        LPARAM lParam;
    };
    skr::ConcurrentQueue<WindowMSG> window_messages_;

    // Process a Windows message and convert to system event
    bool process_message(MSG& msg, SkrSystemEvent& out_event);
    
    // Helper functions
    SInputKeyCode translate_vk_to_keycode(WPARAM vk) const;
    SKeyModifier get_modifier_flags() const;
    InputMouseButtonFlags translate_mouse_button(UINT msg, WPARAM wParam) const;
    
    Win32SystemApp* app_ = nullptr;
    IME* ime_ = nullptr;
    
    // Track mouse enter/leave state per window
    skr::Map<HWND, bool> mouse_tracking_;
};

} // namespace skr