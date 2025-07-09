#include "sdl3_event_source.h"
#include "sdl3_ime.h"
#include "sdl3_window_manager.h"
#include "SkrSystem/system.h"
#include "SkrCore/memory/memory.h"
#include "SkrCore/log.h"

namespace skr {

#define KEY_CODE_TRANS(k, sdlk) \
    case (sdlk):                \
        return (k);

EKeyCode TranslateSDLKeyCode(SDL_Scancode keycode);

inline static SKeyModifier TranslateSDLModifiers(Uint16 sdl_mod)
{
    uint16_t mods = KEY_MODIFIER_None;
    
    if (sdl_mod & SDL_KMOD_LSHIFT) mods |= KEY_MODIFIER_LShift;
    if (sdl_mod & SDL_KMOD_RSHIFT) mods |= KEY_MODIFIER_RShift;
    if (sdl_mod & SDL_KMOD_LCTRL) mods |= KEY_MODIFIER_LCtrl;
    if (sdl_mod & SDL_KMOD_RCTRL) mods |= KEY_MODIFIER_RCtrl;
    if (sdl_mod & SDL_KMOD_LALT) mods |= KEY_MODIFIER_LAlt;
    if (sdl_mod & SDL_KMOD_RALT) mods |= KEY_MODIFIER_RAlt;

    return static_cast<SKeyModifier>(mods);
}

inline static InputMouseButtonFlags TranslateSDLMouseButton(Uint8 button)
{
    switch (button)
    {
        case SDL_BUTTON_LEFT:
            return InputMouseLeftButton;
        case SDL_BUTTON_RIGHT:
            return InputMouseRightButton;
        case SDL_BUTTON_MIDDLE:
            return InputMouseMiddleButton;
        case SDL_BUTTON_X1:
            return InputMouseButton4;
        case SDL_BUTTON_X2:
            return InputMouseButton5;
        default:
            return InputMouseLeftButton;
    }
}

inline static ESkrSystemEventType TranslateSDLEventType(SDL_EventType type)
{
    switch (type)
    {
        case SDL_EVENT_QUIT:
            return SKR_SYSTEM_EVENT_QUIT;
            
        // Window events
        case SDL_EVENT_WINDOW_SHOWN:
            return SKR_SYSTEM_EVENT_WINDOW_SHOWN;
        case SDL_EVENT_WINDOW_HIDDEN:
            return SKR_SYSTEM_EVENT_WINDOW_HIDDEN;
        case SDL_EVENT_WINDOW_MOVED:
            return SKR_SYSTEM_EVENT_WINDOW_MOVED;
        case SDL_EVENT_WINDOW_RESIZED:
            return SKR_SYSTEM_EVENT_WINDOW_RESIZED;
        case SDL_EVENT_WINDOW_MINIMIZED:
            return SKR_SYSTEM_EVENT_WINDOW_MINIMIZED;
        case SDL_EVENT_WINDOW_MAXIMIZED:
            return SKR_SYSTEM_EVENT_WINDOW_MAXIMIZED;
        case SDL_EVENT_WINDOW_RESTORED:
            return SKR_SYSTEM_EVENT_WINDOW_RESTORED;
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            return SKR_SYSTEM_EVENT_WINDOW_FOCUS_GAINED;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            return SKR_SYSTEM_EVENT_WINDOW_FOCUS_LOST;
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            return SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED;
        case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
            return SKR_SYSTEM_EVENT_WINDOW_ENTER_FULLSCREEN;
        case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
            return SKR_SYSTEM_EVENT_WINDOW_LEAVE_FULLSCREEN;
            
        // Keyboard events
        case SDL_EVENT_KEY_DOWN:
            return SKR_SYSTEM_EVENT_KEY_DOWN;
        case SDL_EVENT_KEY_UP:
            return SKR_SYSTEM_EVENT_KEY_UP;
            
        // Mouse events
        case SDL_EVENT_MOUSE_MOTION:
            return SKR_SYSTEM_EVENT_MOUSE_MOVE;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            return SKR_SYSTEM_EVENT_MOUSE_BUTTON_DOWN;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            return SKR_SYSTEM_EVENT_MOUSE_BUTTON_UP;
        case SDL_EVENT_MOUSE_WHEEL:
            return SKR_SYSTEM_EVENT_MOUSE_WHEEL;
            
        default:
            return SKR_SYSTEM_EVENT_INVALID;
    }
}

inline static SkrSystemEvent TranslateSDLEvent(const SDL_Event& e)
{
    SkrSystemEvent result = {};
    const auto type = TranslateSDLEventType((SDL_EventType)e.type);
    switch (e.type)
    {
        case SDL_EVENT_QUIT:
            result.quit.type = SKR_SYSTEM_EVENT_QUIT;
            break;
            
        // Window events
        case SDL_EVENT_WINDOW_SHOWN:
        case SDL_EVENT_WINDOW_HIDDEN:
        case SDL_EVENT_WINDOW_MOVED:
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_MINIMIZED:
        case SDL_EVENT_WINDOW_MAXIMIZED:
        case SDL_EVENT_WINDOW_RESTORED:
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
        case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
            result.window.type = type;
            result.window.window_native_handle = e.window.windowID;
            result.window.x = e.window.data1;
            result.window.y = e.window.data2;
            break;
            
        // SDL window mouse enter/leave are translated to mouse events
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
            result.mouse.type = SKR_SYSTEM_EVENT_MOUSE_ENTER;
            result.mouse.window_native_handle = e.window.windowID;
            // Position will be updated by next mouse move event
            result.mouse.x = 0;
            result.mouse.y = 0;
            result.mouse.button = static_cast<InputMouseButtonFlags>(0);
            result.mouse.modifiers = TranslateSDLModifiers(SDL_GetModState());
            result.mouse.clicks = 0;
            result.mouse.wheel_x = 0.0f;
            result.mouse.wheel_y = 0.0f;
            break;
            
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            result.mouse.type = SKR_SYSTEM_EVENT_MOUSE_LEAVE;
            result.mouse.window_native_handle = e.window.windowID;
            result.mouse.x = 0;
            result.mouse.y = 0;
            result.mouse.button = static_cast<InputMouseButtonFlags>(0);
            result.mouse.modifiers = TranslateSDLModifiers(SDL_GetModState());
            result.mouse.clicks = 0;
            result.mouse.wheel_x = 0.0f;
            result.mouse.wheel_y = 0.0f;
            break;
            
        // Keyboard events
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            result.key.type = type;
            result.key.window_native_handle = e.key.windowID;
            result.key.keycode = TranslateSDLKeyCode(e.key.scancode);
            result.key.scancode = e.key.scancode;
            result.key.modifiers = TranslateSDLModifiers(e.key.mod);
            result.key.down = e.key.down;
            result.key.repeat = e.key.repeat;
            break;
            
        // Mouse events
        case SDL_EVENT_MOUSE_MOTION:
            result.mouse.type = SKR_SYSTEM_EVENT_MOUSE_MOVE;
            result.mouse.window_native_handle = e.motion.windowID;
            result.mouse.x = e.motion.x;  // SDL3 already provides logical coordinates
            result.mouse.y = e.motion.y;
            result.mouse.button = static_cast<InputMouseButtonFlags>(0);  // No button for motion
            result.mouse.modifiers = TranslateSDLModifiers(SDL_GetModState());
            result.mouse.clicks = 0;
            result.mouse.wheel_x = 0.0f;
            result.mouse.wheel_y = 0.0f;
            break;
            
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            result.mouse.type = (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? 
                SKR_SYSTEM_EVENT_MOUSE_BUTTON_DOWN : SKR_SYSTEM_EVENT_MOUSE_BUTTON_UP;
            result.mouse.window_native_handle = e.button.windowID;
            result.mouse.x = e.button.x;  // SDL3 already provides logical coordinates
            result.mouse.y = e.button.y;
            result.mouse.button = TranslateSDLMouseButton(e.button.button);
            result.mouse.modifiers = TranslateSDLModifiers(SDL_GetModState());
            result.mouse.clicks = e.button.clicks;
            result.mouse.wheel_x = 0.0f;
            result.mouse.wheel_y = 0.0f;
            break;
            
        case SDL_EVENT_MOUSE_WHEEL:
            result.mouse.type = SKR_SYSTEM_EVENT_MOUSE_WHEEL;
            result.mouse.window_native_handle = e.wheel.windowID;
            result.mouse.x = static_cast<int32_t>(e.wheel.mouse_x);  // Mouse position
            result.mouse.y = static_cast<int32_t>(e.wheel.mouse_y);
            result.mouse.button = static_cast<InputMouseButtonFlags>(0);
            result.mouse.modifiers = TranslateSDLModifiers(SDL_GetModState());
            result.mouse.clicks = 0;
            result.mouse.wheel_x = e.wheel.x;  // Horizontal scroll amount
            result.mouse.wheel_y = e.wheel.y;  // Vertical scroll amount
            break;
            
        default:
            result.type = SKR_SYSTEM_EVENT_INVALID;
            break;
    }
    
    return result;
}

SDL3EventSource::SDL3EventSource(SystemApp* app) SKR_NOEXCEPT
    : app_(app)
{

}

SDL3EventSource::~SDL3EventSource() SKR_NOEXCEPT
{
    // SDL cleanup is handled by SystemApp
}

bool SDL3EventSource::poll_event(SkrSystemEvent& event) SKR_NOEXCEPT
{
    SDL_Event sdl_event;
    
    while (SDL_PollEvent(&sdl_event))
    {
        // Forward IME-related events to IME system
        if (should_forward_to_ime(sdl_event))
        {
            if (ime_)
            {
                ime_->process_event(sdl_event);
            }
            // Continue to next event as IME events are handled via callbacks
            continue;
        }
        
        // Translate other events
        event = translate_sdl_event(sdl_event);
        if (event.type != SKR_SYSTEM_EVENT_INVALID)
        {
            return true;
        }
    }
    
    return false;
}

bool SDL3EventSource::should_forward_to_ime(const SDL_Event& sdl_event) const
{
    switch (sdl_event.type)
    {
        case SDL_EVENT_TEXT_EDITING:
        case SDL_EVENT_TEXT_INPUT:
        case SDL_EVENT_TEXT_EDITING_CANDIDATES:
            return true;
        default:
            return false;
    }
}

bool SDL3EventSource::wait_events(uint32_t timeout_ms) SKR_NOEXCEPT
{
    SDL_Event sdl_event;
    
    // SDL3 uses different wait API
    bool result = false;
    if (timeout_ms == 0)
    {
        // Infinite wait
        result = SDL_WaitEvent(&sdl_event) != 0;
    }
    else
    {
        // Wait with timeout
        result = SDL_WaitEventTimeout(&sdl_event, static_cast<Sint32>(timeout_ms)) != 0;
    }
    
    if (result)
    {
        // Push the event back to be processed by poll_event
        SDL_PushEvent(&sdl_event);
    }
    
    return result;
}

SkrSystemEvent SDL3EventSource::translate_sdl_event(const SDL_Event& e) const
{
    return TranslateSDLEvent(e);
}

EKeyCode TranslateSDLKeyCode(SDL_Scancode keycode)
{
    switch (keycode)
    {
        KEY_CODE_TRANS(KEY_CODE_Invalid, SDL_SCANCODE_UNKNOWN)
        KEY_CODE_TRANS(KEY_CODE_Backspace, SDL_SCANCODE_BACKSPACE)
        KEY_CODE_TRANS(KEY_CODE_Tab, SDL_SCANCODE_TAB)
        KEY_CODE_TRANS(KEY_CODE_Clear, SDL_SCANCODE_CLEAR)
        KEY_CODE_TRANS(KEY_CODE_Enter, SDL_SCANCODE_KP_ENTER)
        // combined
        KEY_CODE_TRANS(KEY_CODE_Pause, SDL_SCANCODE_PAUSE)
        KEY_CODE_TRANS(KEY_CODE_CapsLock, SDL_SCANCODE_CAPSLOCK)
        KEY_CODE_TRANS(KEY_CODE_Esc, SDL_SCANCODE_ESCAPE)
        KEY_CODE_TRANS(KEY_CODE_SpaceBar, SDL_SCANCODE_SPACE)
        KEY_CODE_TRANS(KEY_CODE_PageUp, SDL_SCANCODE_PAGEUP)
        KEY_CODE_TRANS(KEY_CODE_PageDown, SDL_SCANCODE_PAGEDOWN)
        KEY_CODE_TRANS(KEY_CODE_End, SDL_SCANCODE_END)
        KEY_CODE_TRANS(KEY_CODE_Home, SDL_SCANCODE_HOME)
        KEY_CODE_TRANS(KEY_CODE_Left, SDL_SCANCODE_LEFT)
        KEY_CODE_TRANS(KEY_CODE_Up, SDL_SCANCODE_UP)
        KEY_CODE_TRANS(KEY_CODE_Right, SDL_SCANCODE_RIGHT)
        KEY_CODE_TRANS(KEY_CODE_Down, SDL_SCANCODE_DOWN)
        KEY_CODE_TRANS(KEY_CODE_Select, SDL_SCANCODE_SELECT)
        KEY_CODE_TRANS(KEY_CODE_Execute, SDL_SCANCODE_EXECUTE)
        KEY_CODE_TRANS(KEY_CODE_Printscreen, SDL_SCANCODE_PRINTSCREEN)
        KEY_CODE_TRANS(KEY_CODE_Insert, SDL_SCANCODE_INSERT)
        KEY_CODE_TRANS(KEY_CODE_Del, SDL_SCANCODE_DELETE)
        KEY_CODE_TRANS(KEY_CODE_Help, SDL_SCANCODE_HELP)
        KEY_CODE_TRANS(KEY_CODE_0, SDL_SCANCODE_0)
        KEY_CODE_TRANS(KEY_CODE_1, SDL_SCANCODE_1)
        KEY_CODE_TRANS(KEY_CODE_2, SDL_SCANCODE_2)
        KEY_CODE_TRANS(KEY_CODE_3, SDL_SCANCODE_3)
        KEY_CODE_TRANS(KEY_CODE_4, SDL_SCANCODE_4)
        KEY_CODE_TRANS(KEY_CODE_5, SDL_SCANCODE_5)
        KEY_CODE_TRANS(KEY_CODE_6, SDL_SCANCODE_6)
        KEY_CODE_TRANS(KEY_CODE_7, SDL_SCANCODE_7)
        KEY_CODE_TRANS(KEY_CODE_8, SDL_SCANCODE_8)
        KEY_CODE_TRANS(KEY_CODE_9, SDL_SCANCODE_9)
        KEY_CODE_TRANS(KEY_CODE_A, SDL_SCANCODE_A)
        KEY_CODE_TRANS(KEY_CODE_B, SDL_SCANCODE_B)
        KEY_CODE_TRANS(KEY_CODE_C, SDL_SCANCODE_C)
        KEY_CODE_TRANS(KEY_CODE_D, SDL_SCANCODE_D)
        KEY_CODE_TRANS(KEY_CODE_E, SDL_SCANCODE_E)
        KEY_CODE_TRANS(KEY_CODE_F, SDL_SCANCODE_F)
        KEY_CODE_TRANS(KEY_CODE_G, SDL_SCANCODE_G)
        KEY_CODE_TRANS(KEY_CODE_H, SDL_SCANCODE_H)
        KEY_CODE_TRANS(KEY_CODE_I, SDL_SCANCODE_I)
        KEY_CODE_TRANS(KEY_CODE_J, SDL_SCANCODE_J)
        KEY_CODE_TRANS(KEY_CODE_K, SDL_SCANCODE_K)
        KEY_CODE_TRANS(KEY_CODE_L, SDL_SCANCODE_L)
        KEY_CODE_TRANS(KEY_CODE_M, SDL_SCANCODE_M)
        KEY_CODE_TRANS(KEY_CODE_N, SDL_SCANCODE_N)
        KEY_CODE_TRANS(KEY_CODE_O, SDL_SCANCODE_O)
        KEY_CODE_TRANS(KEY_CODE_P, SDL_SCANCODE_P)
        KEY_CODE_TRANS(KEY_CODE_Q, SDL_SCANCODE_Q)
        KEY_CODE_TRANS(KEY_CODE_R, SDL_SCANCODE_R)
        KEY_CODE_TRANS(KEY_CODE_S, SDL_SCANCODE_S)
        KEY_CODE_TRANS(KEY_CODE_T, SDL_SCANCODE_T)
        KEY_CODE_TRANS(KEY_CODE_U, SDL_SCANCODE_U)
        KEY_CODE_TRANS(KEY_CODE_V, SDL_SCANCODE_V)
        KEY_CODE_TRANS(KEY_CODE_W, SDL_SCANCODE_W)
        KEY_CODE_TRANS(KEY_CODE_X, SDL_SCANCODE_X)
        KEY_CODE_TRANS(KEY_CODE_Y, SDL_SCANCODE_Y)
        KEY_CODE_TRANS(KEY_CODE_Z, SDL_SCANCODE_Z)
        KEY_CODE_TRANS(KEY_CODE_App, SDL_SCANCODE_APPLICATION)
        KEY_CODE_TRANS(KEY_CODE_Sleep, SDL_SCANCODE_SLEEP)
        KEY_CODE_TRANS(KEY_CODE_Numpad0, SDL_SCANCODE_KP_0)
        KEY_CODE_TRANS(KEY_CODE_Numpad1, SDL_SCANCODE_KP_1)
        KEY_CODE_TRANS(KEY_CODE_Numpad2, SDL_SCANCODE_KP_2)
        KEY_CODE_TRANS(KEY_CODE_Numpad3, SDL_SCANCODE_KP_3)
        KEY_CODE_TRANS(KEY_CODE_Numpad4, SDL_SCANCODE_KP_4)
        KEY_CODE_TRANS(KEY_CODE_Numpad5, SDL_SCANCODE_KP_5)
        KEY_CODE_TRANS(KEY_CODE_Numpad6, SDL_SCANCODE_KP_6)
        KEY_CODE_TRANS(KEY_CODE_Numpad7, SDL_SCANCODE_KP_7)
        KEY_CODE_TRANS(KEY_CODE_Numpad8, SDL_SCANCODE_KP_8)
        KEY_CODE_TRANS(KEY_CODE_Numpad9, SDL_SCANCODE_KP_9)
        KEY_CODE_TRANS(KEY_CODE_Multiply, SDL_SCANCODE_KP_MULTIPLY)
        KEY_CODE_TRANS(KEY_CODE_Add, SDL_SCANCODE_KP_PLUS)
        KEY_CODE_TRANS(KEY_CODE_Separator, SDL_SCANCODE_SEPARATOR)
        KEY_CODE_TRANS(KEY_CODE_Subtract, SDL_SCANCODE_KP_MINUS)
        KEY_CODE_TRANS(KEY_CODE_Decimal, SDL_SCANCODE_KP_DECIMAL)
        KEY_CODE_TRANS(KEY_CODE_Divide, SDL_SCANCODE_KP_DIVIDE)
        KEY_CODE_TRANS(KEY_CODE_F1, SDL_SCANCODE_F1)
        KEY_CODE_TRANS(KEY_CODE_F2, SDL_SCANCODE_F2)
        KEY_CODE_TRANS(KEY_CODE_F3, SDL_SCANCODE_F3)
        KEY_CODE_TRANS(KEY_CODE_F4, SDL_SCANCODE_F4)
        KEY_CODE_TRANS(KEY_CODE_F5, SDL_SCANCODE_F5)
        KEY_CODE_TRANS(KEY_CODE_F6, SDL_SCANCODE_F6)
        KEY_CODE_TRANS(KEY_CODE_F7, SDL_SCANCODE_F7)
        KEY_CODE_TRANS(KEY_CODE_F8, SDL_SCANCODE_F8)
        KEY_CODE_TRANS(KEY_CODE_F9, SDL_SCANCODE_F9)
        KEY_CODE_TRANS(KEY_CODE_F10, SDL_SCANCODE_F10)
        KEY_CODE_TRANS(KEY_CODE_F11, SDL_SCANCODE_F11)
        KEY_CODE_TRANS(KEY_CODE_F12, SDL_SCANCODE_F12)
        KEY_CODE_TRANS(KEY_CODE_F13, SDL_SCANCODE_F13)
        KEY_CODE_TRANS(KEY_CODE_F14, SDL_SCANCODE_F14)
        KEY_CODE_TRANS(KEY_CODE_F15, SDL_SCANCODE_F15)
        KEY_CODE_TRANS(KEY_CODE_F16, SDL_SCANCODE_F16)
        KEY_CODE_TRANS(KEY_CODE_F17, SDL_SCANCODE_F17)
        KEY_CODE_TRANS(KEY_CODE_F18, SDL_SCANCODE_F18)
        KEY_CODE_TRANS(KEY_CODE_F19, SDL_SCANCODE_F19)
        KEY_CODE_TRANS(KEY_CODE_F20, SDL_SCANCODE_F20)
        KEY_CODE_TRANS(KEY_CODE_F21, SDL_SCANCODE_F21)
        KEY_CODE_TRANS(KEY_CODE_F22, SDL_SCANCODE_F22)
        KEY_CODE_TRANS(KEY_CODE_F23, SDL_SCANCODE_F23)
        KEY_CODE_TRANS(KEY_CODE_F24, SDL_SCANCODE_F24)
        KEY_CODE_TRANS(KEY_CODE_Numlock, SDL_SCANCODE_NUMLOCKCLEAR)
        KEY_CODE_TRANS(KEY_CODE_Scrolllock, SDL_SCANCODE_SCROLLLOCK)

        KEY_CODE_TRANS(KEY_CODE_LShift, SDL_SCANCODE_LSHIFT)
        KEY_CODE_TRANS(KEY_CODE_RShift, SDL_SCANCODE_RSHIFT)
        KEY_CODE_TRANS(KEY_CODE_LCtrl, SDL_SCANCODE_LCTRL)
        KEY_CODE_TRANS(KEY_CODE_RCtrl, SDL_SCANCODE_RCTRL)
        KEY_CODE_TRANS(KEY_CODE_LAlt, SDL_SCANCODE_LALT)
        KEY_CODE_TRANS(KEY_CODE_RAlt, SDL_SCANCODE_RALT)

        KEY_CODE_TRANS(KEY_CODE_Semicolon, SDL_SCANCODE_SEMICOLON)
        KEY_CODE_TRANS(KEY_CODE_Plus, SDL_SCANCODE_EQUALS)
        KEY_CODE_TRANS(KEY_CODE_Comma, SDL_SCANCODE_COMMA)
        KEY_CODE_TRANS(KEY_CODE_Minus, SDL_SCANCODE_MINUS)
        KEY_CODE_TRANS(KEY_CODE_Dot, SDL_SCANCODE_PERIOD)
        KEY_CODE_TRANS(KEY_CODE_Slash, SDL_SCANCODE_SLASH)
        KEY_CODE_TRANS(KEY_CODE_Wave, SDL_SCANCODE_GRAVE)
        KEY_CODE_TRANS(KEY_CODE_LBracket, SDL_SCANCODE_LEFTBRACKET)
        KEY_CODE_TRANS(KEY_CODE_Backslash, SDL_SCANCODE_BACKSLASH)
        KEY_CODE_TRANS(KEY_CODE_RBracket, SDL_SCANCODE_RIGHTBRACKET)
        KEY_CODE_TRANS(KEY_CODE_Quote, SDL_SCANCODE_APOSTROPHE)

    default:
        return KEY_CODE_Invalid;
    }
}

} // namespace skr