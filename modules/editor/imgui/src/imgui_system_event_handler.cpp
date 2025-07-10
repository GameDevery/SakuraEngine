#include <SkrImGui/imgui_system_event_handler.hpp>
#include <SkrImGui/imgui_backend.hpp>
#include <SkrSystem/window.h>
#include <SkrCore/log.hpp>

namespace skr
{

ImGuiSystemEventHandler::ImGuiSystemEventHandler(ImGuiBackend* backend)
    : _backend(backend)
{
    SKR_ASSERT(backend && "ImGuiSystemEventHandler requires valid backend");
    _context = backend->context();
}

ImGuiSystemEventHandler::~ImGuiSystemEventHandler()
{
}

void ImGuiSystemEventHandler::handle_event(const SkrSystemEvent& event) SKR_NOEXCEPT
{
    if (!_context || !_backend->is_created())
        return;

    // Set main window handle if not set
    if (_main_window_handle == 0 && _backend->system_window())
    {
        _main_window_handle = _backend->system_window()->get_native_handle();
    }

    switch (event.type)
    {
        // Keyboard events
        case SKR_SYSTEM_EVENT_KEY_DOWN:
        case SKR_SYSTEM_EVENT_KEY_UP:
            process_keyboard_event(event.key);
            break;

        // Mouse events
        case SKR_SYSTEM_EVENT_MOUSE_MOVE:
        case SKR_SYSTEM_EVENT_MOUSE_BUTTON_DOWN:
        case SKR_SYSTEM_EVENT_MOUSE_BUTTON_UP:
        case SKR_SYSTEM_EVENT_MOUSE_WHEEL:
        case SKR_SYSTEM_EVENT_MOUSE_ENTER:
        case SKR_SYSTEM_EVENT_MOUSE_LEAVE:
            process_mouse_event(event.mouse);
            break;

        // Window events
        case SKR_SYSTEM_EVENT_WINDOW_SHOWN:
        case SKR_SYSTEM_EVENT_WINDOW_HIDDEN:
        case SKR_SYSTEM_EVENT_WINDOW_MOVED:
        case SKR_SYSTEM_EVENT_WINDOW_RESIZED:
        case SKR_SYSTEM_EVENT_WINDOW_MINIMIZED:
        case SKR_SYSTEM_EVENT_WINDOW_MAXIMIZED:
        case SKR_SYSTEM_EVENT_WINDOW_RESTORED:
        case SKR_SYSTEM_EVENT_WINDOW_MOUSE_ENTER:
        case SKR_SYSTEM_EVENT_WINDOW_MOUSE_LEAVE:
        case SKR_SYSTEM_EVENT_WINDOW_FOCUS_GAINED:
        case SKR_SYSTEM_EVENT_WINDOW_FOCUS_LOST:
        case SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED:
        case SKR_SYSTEM_EVENT_WINDOW_ENTER_FULLSCREEN:
        case SKR_SYSTEM_EVENT_WINDOW_LEAVE_FULLSCREEN:
            process_window_event(event.window);
            break;

        // Quit event
        case SKR_SYSTEM_EVENT_QUIT:
            _want_exit.trigger();
            break;

        default:
            break;
    }
}

void ImGuiSystemEventHandler::process_keyboard_event(const SkrKeyboardEvent& event)
{
    ImGuiIO& io = _context->IO;
    
    ImGuiKey key = skr_key_to_imgui_key(event.keycode);
    if (key != ImGuiKey_None)
    {
        io.AddKeyEvent(key, event.down);
    }

    // Handle modifiers
    io.AddKeyEvent(ImGuiKey_ModCtrl, (event.modifiers & (KEY_MODIFIER_LCtrl | KEY_MODIFIER_RCtrl)) != 0);
    io.AddKeyEvent(ImGuiKey_ModShift, (event.modifiers & (KEY_MODIFIER_LShift | KEY_MODIFIER_RShift)) != 0);
    io.AddKeyEvent(ImGuiKey_ModAlt, (event.modifiers & (KEY_MODIFIER_LAlt | KEY_MODIFIER_RAlt)) != 0);
    // Note: SkrSystem doesn't have a Super/GUI modifier flag

    // TODO: Handle text input for character events
}

void ImGuiSystemEventHandler::process_mouse_event(const SkrMouseEvent& event)
{
    if (event.window_native_handle != _main_window_handle)
        return;

    ImGuiIO& io = _context->IO;

    switch (event.type)
    {
        case SKR_SYSTEM_EVENT_MOUSE_MOVE:
            io.AddMousePosEvent((float)event.x, (float)event.y);
            break;

        case SKR_SYSTEM_EVENT_MOUSE_BUTTON_DOWN:
        case SKR_SYSTEM_EVENT_MOUSE_BUTTON_UP:
        {
            ImGuiMouseButton button = skr_mouse_button_to_imgui(event.button);
            if (button != -1)
            {
                io.AddMouseButtonEvent(button, event.type == SKR_SYSTEM_EVENT_MOUSE_BUTTON_DOWN);
            }
            break;
        }

        case SKR_SYSTEM_EVENT_MOUSE_WHEEL:
            io.AddMouseWheelEvent(event.wheel_x, event.wheel_y);
            break;

        case SKR_SYSTEM_EVENT_MOUSE_LEAVE:
            io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
            break;

        default:
            break;
    }
}

void ImGuiSystemEventHandler::process_window_event(const SkrWindowEvent& event)
{
    if (event.window_native_handle != _main_window_handle)
        return;

    switch (event.type)
    {
        case SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED:
            _want_exit.trigger();
            break;

        case SKR_SYSTEM_EVENT_WINDOW_RESIZED:
            _want_resize.trigger();
            break;

        case SKR_SYSTEM_EVENT_WINDOW_FOCUS_GAINED:
            _context->IO.AddFocusEvent(true);
            break;

        case SKR_SYSTEM_EVENT_WINDOW_FOCUS_LOST:
            _context->IO.AddFocusEvent(false);
            break;

        // TODO: Handle DPI/content scale changes
        default:
            break;
    }
}

ImGuiKey ImGuiSystemEventHandler::skr_key_to_imgui_key(SInputKeyCode keycode) const
{
    switch (keycode)
    {
        case KEY_CODE_Tab: return ImGuiKey_Tab;
        case KEY_CODE_Left: return ImGuiKey_LeftArrow;
        case KEY_CODE_Right: return ImGuiKey_RightArrow;
        case KEY_CODE_Up: return ImGuiKey_UpArrow;
        case KEY_CODE_Down: return ImGuiKey_DownArrow;
        case KEY_CODE_PageUp: return ImGuiKey_PageUp;
        case KEY_CODE_PageDown: return ImGuiKey_PageDown;
        case KEY_CODE_Home: return ImGuiKey_Home;
        case KEY_CODE_End: return ImGuiKey_End;
        case KEY_CODE_Insert: return ImGuiKey_Insert;
        case KEY_CODE_Del: return ImGuiKey_Delete;
        case KEY_CODE_Backspace: return ImGuiKey_Backspace;
        case KEY_CODE_SpaceBar: return ImGuiKey_Space;
        case KEY_CODE_Enter: return ImGuiKey_Enter;
        case KEY_CODE_Esc: return ImGuiKey_Escape;
        case KEY_CODE_Quote: return ImGuiKey_Apostrophe;
        case KEY_CODE_Comma: return ImGuiKey_Comma;
        case KEY_CODE_Minus: return ImGuiKey_Minus;
        case KEY_CODE_Slash: return ImGuiKey_Slash;
        case KEY_CODE_Semicolon: return ImGuiKey_Semicolon;
        case KEY_CODE_LBracket: return ImGuiKey_LeftBracket;
        case KEY_CODE_Backslash: return ImGuiKey_Backslash;
        case KEY_CODE_RBracket: return ImGuiKey_RightBracket;
        case KEY_CODE_CapsLock: return ImGuiKey_CapsLock;
        case KEY_CODE_Printscreen: return ImGuiKey_PrintScreen;
        case KEY_CODE_Pause: return ImGuiKey_Pause;
        case KEY_CODE_Numpad0: return ImGuiKey_Keypad0;
        case KEY_CODE_Numpad1: return ImGuiKey_Keypad1;
        case KEY_CODE_Numpad2: return ImGuiKey_Keypad2;
        case KEY_CODE_Numpad3: return ImGuiKey_Keypad3;
        case KEY_CODE_Numpad4: return ImGuiKey_Keypad4;
        case KEY_CODE_Numpad5: return ImGuiKey_Keypad5;
        case KEY_CODE_Numpad6: return ImGuiKey_Keypad6;
        case KEY_CODE_Numpad7: return ImGuiKey_Keypad7;
        case KEY_CODE_Numpad8: return ImGuiKey_Keypad8;
        case KEY_CODE_Numpad9: return ImGuiKey_Keypad9;
        case KEY_CODE_LShift: return ImGuiKey_LeftShift;
        case KEY_CODE_LCtrl: return ImGuiKey_LeftCtrl;
        case KEY_CODE_LAlt: return ImGuiKey_LeftAlt;
        case KEY_CODE_LSystem: return ImGuiKey_LeftSuper;
        case KEY_CODE_RShift: return ImGuiKey_RightShift;
        case KEY_CODE_RCtrl: return ImGuiKey_RightCtrl;
        case KEY_CODE_RAlt: return ImGuiKey_RightAlt;
        case KEY_CODE_RSystem: return ImGuiKey_RightSuper;
        case KEY_CODE_App: return ImGuiKey_Menu;
        case KEY_CODE_0: return ImGuiKey_0;
        case KEY_CODE_1: return ImGuiKey_1;
        case KEY_CODE_2: return ImGuiKey_2;
        case KEY_CODE_3: return ImGuiKey_3;
        case KEY_CODE_4: return ImGuiKey_4;
        case KEY_CODE_5: return ImGuiKey_5;
        case KEY_CODE_6: return ImGuiKey_6;
        case KEY_CODE_7: return ImGuiKey_7;
        case KEY_CODE_8: return ImGuiKey_8;
        case KEY_CODE_9: return ImGuiKey_9;
        case KEY_CODE_A: return ImGuiKey_A;
        case KEY_CODE_B: return ImGuiKey_B;
        case KEY_CODE_C: return ImGuiKey_C;
        case KEY_CODE_D: return ImGuiKey_D;
        case KEY_CODE_E: return ImGuiKey_E;
        case KEY_CODE_F: return ImGuiKey_F;
        case KEY_CODE_G: return ImGuiKey_G;
        case KEY_CODE_H: return ImGuiKey_H;
        case KEY_CODE_I: return ImGuiKey_I;
        case KEY_CODE_J: return ImGuiKey_J;
        case KEY_CODE_K: return ImGuiKey_K;
        case KEY_CODE_L: return ImGuiKey_L;
        case KEY_CODE_M: return ImGuiKey_M;
        case KEY_CODE_N: return ImGuiKey_N;
        case KEY_CODE_O: return ImGuiKey_O;
        case KEY_CODE_P: return ImGuiKey_P;
        case KEY_CODE_Q: return ImGuiKey_Q;
        case KEY_CODE_R: return ImGuiKey_R;
        case KEY_CODE_S: return ImGuiKey_S;
        case KEY_CODE_T: return ImGuiKey_T;
        case KEY_CODE_U: return ImGuiKey_U;
        case KEY_CODE_V: return ImGuiKey_V;
        case KEY_CODE_W: return ImGuiKey_W;
        case KEY_CODE_X: return ImGuiKey_X;
        case KEY_CODE_Y: return ImGuiKey_Y;
        case KEY_CODE_Z: return ImGuiKey_Z;
        case KEY_CODE_F1: return ImGuiKey_F1;
        case KEY_CODE_F2: return ImGuiKey_F2;
        case KEY_CODE_F3: return ImGuiKey_F3;
        case KEY_CODE_F4: return ImGuiKey_F4;
        case KEY_CODE_F5: return ImGuiKey_F5;
        case KEY_CODE_F6: return ImGuiKey_F6;
        case KEY_CODE_F7: return ImGuiKey_F7;
        case KEY_CODE_F8: return ImGuiKey_F8;
        case KEY_CODE_F9: return ImGuiKey_F9;
        case KEY_CODE_F10: return ImGuiKey_F10;
        case KEY_CODE_F11: return ImGuiKey_F11;
        case KEY_CODE_F12: return ImGuiKey_F12;
        default: return ImGuiKey_None;
    }
}

ImGuiMouseButton ImGuiSystemEventHandler::skr_mouse_button_to_imgui(InputMouseButtonFlags button) const
{
    if (button & InputMouseLeftButton)
        return ImGuiMouseButton_Left;
    if (button & InputMouseRightButton)
        return ImGuiMouseButton_Right;
    if (button & InputMouseMiddleButton)
        return ImGuiMouseButton_Middle;
    if (button & InputMouseButton4)
        return 3; // ImGuiMouseButton_4
    if (button & InputMouseButton5)
        return 4; // ImGuiMouseButton_5
    return -1;
}

} // namespace skr