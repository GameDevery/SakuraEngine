#include "SkrCore/platform/input.h"
#include "SDL2/SDL_scancode.h"

#include "SDL2/SDL_keyboard.h"
#include "SDL2/SDL_mouse.h"

#define KEY_CODE_TRANS(k, sdlk) \
    case (k):                   \
        return (sdlk);

inline static SDL_Scancode KeyCodeTranslator(EKeyCode keycode)
{
    switch (keycode)
    {
        KEY_CODE_TRANS(KEY_CODE_Invalid, SDL_SCANCODE_UNKNOWN)
        KEY_CODE_TRANS(KEY_CODE_Backspace, SDL_SCANCODE_BACKSPACE)
        KEY_CODE_TRANS(KEY_CODE_Tab, SDL_SCANCODE_TAB)
        KEY_CODE_TRANS(KEY_CODE_Clear, SDL_SCANCODE_CLEAR)
        KEY_CODE_TRANS(KEY_CODE_Enter, SDL_SCANCODE_KP_ENTER)
        // combined
        KEY_CODE_TRANS(KEY_CODE_Shift, SDL_SCANCODE_UNKNOWN)
        KEY_CODE_TRANS(KEY_CODE_Ctrl, SDL_SCANCODE_UNKNOWN)
        KEY_CODE_TRANS(KEY_CODE_Alt, SDL_SCANCODE_UNKNOWN)
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
        KEY_CODE_TRANS(KEY_CODE_Print, SDL_SCANCODE_UNKNOWN)
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
        KEY_CODE_TRANS(KEY_CODE_LSystem, SDL_SCANCODE_UNKNOWN)
        KEY_CODE_TRANS(KEY_CODE_RSystem, SDL_SCANCODE_UNKNOWN)
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
            return SDL_SCANCODE_UNKNOWN;
    }
}

SKR_CORE_API bool skr_key_down(EKeyCode key_code)
{
    const Uint8* keystate = SDL_GetKeyboardState(NULL);
    switch (key_code)
    {
        case KEY_CODE_Shift:
            return keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT];
        case KEY_CODE_Alt:
            return keystate[SDL_SCANCODE_LALT] || keystate[SDL_SCANCODE_RALT];
        case KEY_CODE_Ctrl:
            return keystate[SDL_SCANCODE_LCTRL] || keystate[SDL_SCANCODE_RCTRL];
        default:
            return keystate[KeyCodeTranslator(key_code)];
    }
}

Uint32 MouseKeyCodeTranslator(EMouseKey keycode)
{
    switch (keycode)
    {
        KEY_CODE_TRANS(MOUSE_KEY_None, 0)
        KEY_CODE_TRANS(MOUSE_KEY_LB, SDL_BUTTON_LMASK)
        KEY_CODE_TRANS(MOUSE_KEY_RB, SDL_BUTTON_RMASK)
        KEY_CODE_TRANS(MOUSE_KEY_MB, SDL_BUTTON_MMASK)
        KEY_CODE_TRANS(MOUSE_KEY_X1B, SDL_BUTTON_X1MASK)
        KEY_CODE_TRANS(MOUSE_KEY_X2B, SDL_BUTTON_X2MASK)
        default:
            return 0;
    }
}

SKR_CORE_API bool skr_mouse_key_down(EMouseKey key_code)
{
    int x, y;
    Uint32 button = SDL_GetGlobalMouseState(&x, &y);
    if (button & MouseKeyCodeTranslator(key_code))
        return true;
    return false;
}

SKR_CORE_API bool skr_set_cursor_pos(int32_t x, int32_t y)
{
    SDL_WarpMouseInWindow(NULL, x, y);
    return true;
}

SDL_SystemCursor MouseCursorTranslator(EMouseCursor cursor)
{
    switch (cursor)
    {
        KEY_CODE_TRANS(MOUSE_CURSOR_Arrow, SDL_SYSTEM_CURSOR_ARROW)
        KEY_CODE_TRANS(MOUSE_CURSOR_TextInput, SDL_SYSTEM_CURSOR_IBEAM)
        KEY_CODE_TRANS(MOUSE_CURSOR_ResizeAll, SDL_SYSTEM_CURSOR_SIZEALL)
        KEY_CODE_TRANS(MOUSE_CURSOR_ResizeEW, SDL_SYSTEM_CURSOR_SIZEWE)
        KEY_CODE_TRANS(MOUSE_CURSOR_ResizeNS, SDL_SYSTEM_CURSOR_SIZENS)
        KEY_CODE_TRANS(MOUSE_CURSOR_ResizeNESW, SDL_SYSTEM_CURSOR_SIZENESW)
        KEY_CODE_TRANS(MOUSE_CURSOR_ResizeNWSE, SDL_SYSTEM_CURSOR_SIZENWSE)
        KEY_CODE_TRANS(MOUSE_CURSOR_Hand, SDL_SYSTEM_CURSOR_HAND)
        KEY_CODE_TRANS(MOUSE_CURSOR_NotAllowed, SDL_SYSTEM_CURSOR_NO)
        KEY_CODE_TRANS(MOUSE_CURSOR_Wait, SDL_SYSTEM_CURSOR_WAIT)
        KEY_CODE_TRANS(MOUSE_CURSOR_WaitArrow, SDL_SYSTEM_CURSOR_WAITARROW)
        default:
            return 0;
    }
}

SKR_CORE_API bool skr_set_cursor(EMouseCursor cursor)
{
    if (cursor == MOUSE_CURSOR_Hide)
    {
        SDL_ShowCursor(0);
        return true;
    }
    else
    {
        SDL_Cursor* sdlCursor;
        sdlCursor = SDL_GetCursor();
        if (sdlCursor)
            SDL_FreeCursor(sdlCursor);
        sdlCursor = SDL_CreateSystemCursor(MouseCursorTranslator(cursor));
        if (sdlCursor)
        {
            SDL_SetCursor(sdlCursor);
            return true;
        }
        return false;
    }
}

void skr_cursor_pos(int32_t* x, int32_t* y, ECursorCoordinate corrdinate)
{
    if (corrdinate == CURSOR_COORDINATE_WINDOW)
    {
        SDL_GetMouseState(x, y);
    }
    else if (corrdinate == CURSOR_COORDINATE_SCREEN)
    {
        SDL_GetGlobalMouseState(x, y);
    }
}