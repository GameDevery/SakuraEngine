#include "win32_event_source.h"
#include "win32_window.h"
#include "win32_window_manager.h"
#include "win32_ime.h"
#include "SkrSystem/system.h"
#include "SkrCore/log.h"
#include <windowsx.h>

namespace skr {

Win32EventSource::Win32EventSource(SystemApp* app) SKR_NOEXCEPT
    : app_(app)
{
}

Win32EventSource::~Win32EventSource() SKR_NOEXCEPT
{
}

bool Win32EventSource::poll_event(SkrSystemEvent& event) SKR_NOEXCEPT
{
    // Check for mouse tracking without consuming messages
    MSG msg;
    if (PeekMessageW(&msg, nullptr, 0, 0, PM_NOREMOVE))
    {
        if (msg.message == WM_MOUSEMOVE && msg.hwnd)
        {
            // Check if we're already tracking this window
            if (!mouse_tracking_.contains(msg.hwnd))
            {
                mouse_tracking_.add(msg.hwnd, true);
                
                // Register for mouse leave notification
                TRACKMOUSEEVENT tme = {};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = msg.hwnd;
                TrackMouseEvent(&tme);
                
                // Generate mouse enter event
                uint32_t dpi = Win32WindowManager::get_dpi_for_window(msg.hwnd);
                float dpi_scale = dpi / 96.0f;
                
                event.type = SKR_SYSTEM_EVENT_MOUSE_ENTER;
                event.mouse.type = SKR_SYSTEM_EVENT_MOUSE_ENTER;
                event.mouse.window_native_handle = msg.hwnd;
                event.mouse.x = static_cast<int32_t>(GET_X_LPARAM(msg.lParam) / dpi_scale);
                event.mouse.y = static_cast<int32_t>(GET_Y_LPARAM(msg.lParam) / dpi_scale);
                event.mouse.button = static_cast<InputMouseButtonFlags>(0);
                event.mouse.modifiers = get_modifier_flags();
                event.mouse.clicks = 0;
                event.mouse.wheel_x = 0.0f;
                event.mouse.wheel_y = 0.0f;
                
                // Don't consume the message, let it be processed as move event next time
                return true;
            }
        }
    }
    
    // Process Windows messages normally
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        // Standard message preprocessing
        TranslateMessage(&msg);
        
        // Let IME process the message first
        if (ime_ && msg.hwnd)
        {
            Win32IME* win32_ime = static_cast<Win32IME*>(ime_);
            LRESULT result = 0;
            if (win32_ime->process_message(msg.hwnd, msg.message, msg.wParam, msg.lParam, result))
            {
                // IME handled the message, continue to next
                continue;
            }
        }
        
        // Try to convert to SKR event
        if (process_message(msg, event))
        {
            // Dispatch the message for default processing
            DispatchMessageW(&msg);
            return true;
        }
        
        // Not an event we care about, just dispatch it
        DispatchMessageW(&msg);
    }
    
    // Process Window Messages
    WindowMSG window_msg;
    if (window_messages_.try_dequeue(window_msg))
    {
        MSG _win_msg = { window_msg.hwnd, window_msg.message, window_msg.wParam, window_msg.lParam };
        process_message(_win_msg, event);
        return true;
    }

    return false;
}

bool Win32EventSource::wait_events(uint32_t timeout_ms) SKR_NOEXCEPT
{
    // Windows message pump with timeout
    timeout_ms = (timeout_ms == 0) ? INFINITE : timeout_ms;
    auto wait_result = MsgWaitForMultipleObjects(0, nullptr, FALSE, timeout_ms, QS_ALLINPUT);
    
    if (wait_result == WAIT_OBJECT_0)
    {
        // Messages are available, will be processed by poll_event
        return true;
    }
    
    return false;
}

bool Win32EventSource::process_message(MSG& msg, SkrSystemEvent& out_event)
{
    HWND hwnd = msg.hwnd;
    UINT message = msg.message;
    WPARAM wParam = msg.wParam;
    LPARAM lParam = msg.lParam;
    
    out_event = {};
    out_event.window.window_native_handle = hwnd;
    
    // Get DPI scale for converting mouse coordinates
    uint32_t dpi = Win32WindowManager::get_dpi_for_window(hwnd);
    float dpi_scale = dpi / 96.0f;
    
    switch (message)
    {
        // Window events
        case WM_CLOSE:
            out_event.type = SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED;
            return true;
            
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED)
            {
                out_event.type = SKR_SYSTEM_EVENT_WINDOW_MINIMIZED;
            }
            else if (wParam == SIZE_MAXIMIZED)
            {
                out_event.type = SKR_SYSTEM_EVENT_WINDOW_MAXIMIZED;
            }
            else if (wParam == SIZE_RESTORED)
            {
                out_event.type = SKR_SYSTEM_EVENT_WINDOW_RESTORED;
            }
            return true;
            
        case WM_SIZING:
            {
                out_event.type = SKR_SYSTEM_EVENT_WINDOW_RESIZED;
                out_event.window.x = LOWORD(lParam);  // width
                out_event.window.y = HIWORD(lParam);  // height
            }
            return true;
            
        case WM_MOVE:
            out_event.type = SKR_SYSTEM_EVENT_WINDOW_MOVED;
            out_event.window.x = static_cast<int16_t>(LOWORD(lParam));  // x
            out_event.window.y = static_cast<int16_t>(HIWORD(lParam));  // y
            return true;
            
        case WM_SETFOCUS:
            out_event.type = SKR_SYSTEM_EVENT_WINDOW_FOCUS_GAINED;
            return true;
            
        case WM_KILLFOCUS:
            out_event.type = SKR_SYSTEM_EVENT_WINDOW_FOCUS_LOST;
            return true;
            
        case WM_SHOWWINDOW:
            out_event.type = wParam ? SKR_SYSTEM_EVENT_WINDOW_SHOWN : SKR_SYSTEM_EVENT_WINDOW_HIDDEN;
            return true;
            
        // Keyboard events
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            bool is_repeat = (lParam & 0x40000000) != 0;
            
            out_event.type = SKR_SYSTEM_EVENT_KEY_DOWN;
            out_event.key.type = SKR_SYSTEM_EVENT_KEY_DOWN;
            out_event.key.window_native_handle = hwnd;
            out_event.key.keycode = translate_vk_to_keycode(wParam);
            out_event.key.scancode = (lParam >> 16) & 0xFF;
            out_event.key.modifiers = get_modifier_flags();
            out_event.key.down = true;
            out_event.key.repeat = is_repeat;
            return true;
        }
            
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            out_event.type = SKR_SYSTEM_EVENT_KEY_UP;
            out_event.key.type = SKR_SYSTEM_EVENT_KEY_UP;
            out_event.key.window_native_handle = hwnd;
            out_event.key.keycode = translate_vk_to_keycode(wParam);
            out_event.key.scancode = (lParam >> 16) & 0xFF;
            out_event.key.modifiers = get_modifier_flags();
            out_event.key.down = false;
            out_event.key.repeat = false;
            return true;
        }
            
        // Mouse events
        case WM_MOUSEMOVE:
        {
            // Simply generate the move event
            out_event.type = SKR_SYSTEM_EVENT_MOUSE_MOVE;
            out_event.mouse.type = SKR_SYSTEM_EVENT_MOUSE_MOVE;
            out_event.mouse.window_native_handle = hwnd;
            out_event.mouse.x = static_cast<int32_t>(GET_X_LPARAM(lParam) / dpi_scale);
            out_event.mouse.y = static_cast<int32_t>(GET_Y_LPARAM(lParam) / dpi_scale);
            out_event.mouse.button = static_cast<InputMouseButtonFlags>(0);
            out_event.mouse.modifiers = get_modifier_flags();
            out_event.mouse.clicks = 0;
            out_event.mouse.wheel_x = 0.0f;
            out_event.mouse.wheel_y = 0.0f;
            return true;
        }
            
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
        {
            SetCapture(hwnd);
            out_event.type = SKR_SYSTEM_EVENT_MOUSE_BUTTON_DOWN;
            out_event.mouse.type = SKR_SYSTEM_EVENT_MOUSE_BUTTON_DOWN;
            out_event.mouse.window_native_handle = hwnd;
            out_event.mouse.x = static_cast<int32_t>(GET_X_LPARAM(lParam) / dpi_scale);
            out_event.mouse.y = static_cast<int32_t>(GET_Y_LPARAM(lParam) / dpi_scale);
            out_event.mouse.button = translate_mouse_button(message, wParam);
            out_event.mouse.modifiers = get_modifier_flags();
            out_event.mouse.clicks = 1;
            out_event.mouse.wheel_x = 0.0f;
            out_event.mouse.wheel_y = 0.0f;
            return true;
        }
            
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        {
            ReleaseCapture();
            out_event.type = SKR_SYSTEM_EVENT_MOUSE_BUTTON_UP;
            out_event.mouse.type = SKR_SYSTEM_EVENT_MOUSE_BUTTON_UP;
            out_event.mouse.window_native_handle = hwnd;
            out_event.mouse.x = static_cast<int32_t>(GET_X_LPARAM(lParam) / dpi_scale);
            out_event.mouse.y = static_cast<int32_t>(GET_Y_LPARAM(lParam) / dpi_scale);
            out_event.mouse.button = translate_mouse_button(message, wParam);
            out_event.mouse.modifiers = get_modifier_flags();
            out_event.mouse.clicks = 1;
            out_event.mouse.wheel_x = 0.0f;
            out_event.mouse.wheel_y = 0.0f;
            return true;
        }
            
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDBLCLK:
        {
            out_event.type = SKR_SYSTEM_EVENT_MOUSE_BUTTON_DOWN;
            out_event.mouse.type = SKR_SYSTEM_EVENT_MOUSE_BUTTON_DOWN;
            out_event.mouse.window_native_handle = hwnd;
            out_event.mouse.x = static_cast<int32_t>(GET_X_LPARAM(lParam) / dpi_scale);
            out_event.mouse.y = static_cast<int32_t>(GET_Y_LPARAM(lParam) / dpi_scale);
            out_event.mouse.button = translate_mouse_button(message, wParam);
            out_event.mouse.modifiers = get_modifier_flags();
            out_event.mouse.clicks = 2; // Double click
            out_event.mouse.wheel_x = 0.0f;
            out_event.mouse.wheel_y = 0.0f;
            return true;
        }
            
        case WM_MOUSEWHEEL:
        {
            out_event.type = SKR_SYSTEM_EVENT_MOUSE_WHEEL;
            out_event.mouse.type = SKR_SYSTEM_EVENT_MOUSE_WHEEL;
            out_event.mouse.window_native_handle = hwnd;
            
            // For wheel events, the position is in screen coordinates
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hwnd, &pt);
            
            // Convert physical pixels to logical points
            out_event.mouse.x = static_cast<int32_t>(pt.x / dpi_scale);
            out_event.mouse.y = static_cast<int32_t>(pt.y / dpi_scale);
            out_event.mouse.button = static_cast<InputMouseButtonFlags>(0);
            out_event.mouse.modifiers = get_modifier_flags();
            out_event.mouse.clicks = 0;
            out_event.mouse.wheel_x = 0.0f;
            out_event.mouse.wheel_y = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            return true;
        }
            
        case WM_MOUSEHWHEEL:
        {
            out_event.type = SKR_SYSTEM_EVENT_MOUSE_WHEEL;
            out_event.mouse.type = SKR_SYSTEM_EVENT_MOUSE_WHEEL;
            out_event.mouse.window_native_handle = hwnd;
            
            // For wheel events, the position is in screen coordinates
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hwnd, &pt);
            
            // Convert physical pixels to logical points
            out_event.mouse.x = static_cast<int32_t>(pt.x / dpi_scale);
            out_event.mouse.y = static_cast<int32_t>(pt.y / dpi_scale);
            out_event.mouse.button = static_cast<InputMouseButtonFlags>(0);
            out_event.mouse.modifiers = get_modifier_flags();
            out_event.mouse.clicks = 0;
            out_event.mouse.wheel_x = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            out_event.mouse.wheel_y = 0.0f;
            return true;
        }
            
        // Mouse enter/leave tracking
        case WM_MOUSELEAVE:
        {
            // Remove tracking for this window
            mouse_tracking_.remove(hwnd);
            
            out_event.type = SKR_SYSTEM_EVENT_MOUSE_LEAVE;
            out_event.mouse.type = SKR_SYSTEM_EVENT_MOUSE_LEAVE;
            out_event.mouse.window_native_handle = hwnd;
            out_event.mouse.x = -1;  // Position not available on leave
            out_event.mouse.y = -1;
            out_event.mouse.button = static_cast<InputMouseButtonFlags>(0);
            out_event.mouse.modifiers = get_modifier_flags();
            out_event.mouse.clicks = 0;
            out_event.mouse.wheel_x = 0.0f;
            out_event.mouse.wheel_y = 0.0f;
            return true;
        }
            
        // Application events
        case WM_QUIT:
            out_event.type = SKR_SYSTEM_EVENT_QUIT;
            return true;
    }
    
    return false;
}

SInputKeyCode Win32EventSource::translate_vk_to_keycode(WPARAM vk) const
{
    // Map virtual key codes to engine key codes
    switch (vk)
    {
        // Letters
        case 'A': return KEY_CODE_A;
        case 'B': return KEY_CODE_B;
        case 'C': return KEY_CODE_C;
        case 'D': return KEY_CODE_D;
        case 'E': return KEY_CODE_E;
        case 'F': return KEY_CODE_F;
        case 'G': return KEY_CODE_G;
        case 'H': return KEY_CODE_H;
        case 'I': return KEY_CODE_I;
        case 'J': return KEY_CODE_J;
        case 'K': return KEY_CODE_K;
        case 'L': return KEY_CODE_L;
        case 'M': return KEY_CODE_M;
        case 'N': return KEY_CODE_N;
        case 'O': return KEY_CODE_O;
        case 'P': return KEY_CODE_P;
        case 'Q': return KEY_CODE_Q;
        case 'R': return KEY_CODE_R;
        case 'S': return KEY_CODE_S;
        case 'T': return KEY_CODE_T;
        case 'U': return KEY_CODE_U;
        case 'V': return KEY_CODE_V;
        case 'W': return KEY_CODE_W;
        case 'X': return KEY_CODE_X;
        case 'Y': return KEY_CODE_Y;
        case 'Z': return KEY_CODE_Z;
        
        // Numbers
        case '0': return KEY_CODE_0;
        case '1': return KEY_CODE_1;
        case '2': return KEY_CODE_2;
        case '3': return KEY_CODE_3;
        case '4': return KEY_CODE_4;
        case '5': return KEY_CODE_5;
        case '6': return KEY_CODE_6;
        case '7': return KEY_CODE_7;
        case '8': return KEY_CODE_8;
        case '9': return KEY_CODE_9;
        
        // Function keys
        case VK_F1: return KEY_CODE_F1;
        case VK_F2: return KEY_CODE_F2;
        case VK_F3: return KEY_CODE_F3;
        case VK_F4: return KEY_CODE_F4;
        case VK_F5: return KEY_CODE_F5;
        case VK_F6: return KEY_CODE_F6;
        case VK_F7: return KEY_CODE_F7;
        case VK_F8: return KEY_CODE_F8;
        case VK_F9: return KEY_CODE_F9;
        case VK_F10: return KEY_CODE_F10;
        case VK_F11: return KEY_CODE_F11;
        case VK_F12: return KEY_CODE_F12;
        
        // Navigation
        case VK_LEFT: return KEY_CODE_Left;
        case VK_RIGHT: return KEY_CODE_Right;
        case VK_UP: return KEY_CODE_Up;
        case VK_DOWN: return KEY_CODE_Down;
        case VK_HOME: return KEY_CODE_Home;
        case VK_END: return KEY_CODE_End;
        case VK_PRIOR: return KEY_CODE_PageUp;
        case VK_NEXT: return KEY_CODE_PageDown;
        case VK_INSERT: return KEY_CODE_Insert;
        case VK_DELETE: return KEY_CODE_Del;
        
        // Control keys
        case VK_RETURN: return KEY_CODE_Enter;
        case VK_ESCAPE: return KEY_CODE_Esc;
        case VK_SPACE: return KEY_CODE_SpaceBar;
        case VK_BACK: return KEY_CODE_Backspace;
        case VK_TAB: return KEY_CODE_Tab;
        case VK_CAPITAL: return KEY_CODE_CapsLock;
        case VK_LSHIFT: return KEY_CODE_LShift;
        case VK_RSHIFT: return KEY_CODE_RShift;
        case VK_LCONTROL: return KEY_CODE_LCtrl;
        case VK_RCONTROL: return KEY_CODE_RCtrl;
        case VK_LMENU: return KEY_CODE_LAlt;
        case VK_RMENU: return KEY_CODE_RAlt;
        case VK_LWIN: return KEY_CODE_LSystem;
        case VK_RWIN: return KEY_CODE_RSystem;
        case VK_APPS: return KEY_CODE_App;
        
        // Numpad
        case VK_NUMPAD0: return KEY_CODE_Numpad0;
        case VK_NUMPAD1: return KEY_CODE_Numpad1;
        case VK_NUMPAD2: return KEY_CODE_Numpad2;
        case VK_NUMPAD3: return KEY_CODE_Numpad3;
        case VK_NUMPAD4: return KEY_CODE_Numpad4;
        case VK_NUMPAD5: return KEY_CODE_Numpad5;
        case VK_NUMPAD6: return KEY_CODE_Numpad6;
        case VK_NUMPAD7: return KEY_CODE_Numpad7;
        case VK_NUMPAD8: return KEY_CODE_Numpad8;
        case VK_NUMPAD9: return KEY_CODE_Numpad9;
        case VK_MULTIPLY: return KEY_CODE_Multiply;
        case VK_ADD: return KEY_CODE_Add;
        case VK_SUBTRACT: return KEY_CODE_Subtract;
        case VK_DECIMAL: return KEY_CODE_Decimal;
        case VK_DIVIDE: return KEY_CODE_Divide;
        case VK_NUMLOCK: return KEY_CODE_Numlock;
        
        // Punctuation and symbols
        case VK_OEM_MINUS: return KEY_CODE_Minus;
        case VK_OEM_PLUS: return KEY_CODE_Plus;
        case VK_OEM_4: return KEY_CODE_LBracket;
        case VK_OEM_6: return KEY_CODE_RBracket;
        case VK_OEM_5: return KEY_CODE_Backslash;
        case VK_OEM_1: return KEY_CODE_Semicolon;
        case VK_OEM_7: return KEY_CODE_Quote;
        case VK_OEM_COMMA: return KEY_CODE_Comma;
        case VK_OEM_PERIOD: return KEY_CODE_Dot;
        case VK_OEM_2: return KEY_CODE_Slash;
        case VK_OEM_3: return KEY_CODE_Wave;
        
        // Other
        case VK_PAUSE: return KEY_CODE_Pause;
        case VK_SCROLL: return KEY_CODE_Scrolllock;
        case VK_PRINT: return KEY_CODE_Print;
        
        default: return KEY_CODE_Invalid;
    }
}

SKeyModifier Win32EventSource::get_modifier_flags() const
{
    uint32_t flags = 0;
    
    if (GetKeyState(VK_LSHIFT) & 0x8000)
        flags |= KEY_MODIFIER_LShift;
    if (GetKeyState(VK_RSHIFT) & 0x8000)
        flags |= KEY_MODIFIER_RShift;
    if (GetKeyState(VK_LCONTROL) & 0x8000)
        flags |= KEY_MODIFIER_LCtrl;
    if (GetKeyState(VK_RCONTROL) & 0x8000)
        flags |= KEY_MODIFIER_RCtrl;
    if (GetKeyState(VK_LMENU) & 0x8000)
        flags |= KEY_MODIFIER_LAlt;
    if (GetKeyState(VK_RMENU) & 0x8000)
        flags |= KEY_MODIFIER_RAlt;
    // Note: GUI/Windows key, Caps Lock, Num Lock are not in EKeyModifier
    
    return static_cast<SKeyModifier>(flags);
}

InputMouseButtonFlags Win32EventSource::translate_mouse_button(UINT msg, WPARAM wParam) const
{
    switch (msg)
    {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
            return InputMouseLeftButton;
            
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
            return InputMouseRightButton;
            
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK:
            return InputMouseMiddleButton;
            
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_XBUTTONDBLCLK:
            return (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? InputMouseButton4 : InputMouseButton5;
            
        default:
            return InputMouseLeftButton;
    }
}

} // namespace skr