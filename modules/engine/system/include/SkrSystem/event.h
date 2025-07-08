#pragma once
#include "input.h"

SKR_EXTERN_C_BEGIN

typedef enum ESkrSystemEventType
{
    SKR_SYSTEM_EVENT_INVALID = 0,
    SKR_SYSTEM_EVENT_QUIT = 0x100, // user-requested quit

    SKR_SYSTEM_EVENT_WINDOW_SHOWN = 0x202,     /**< Window has been shown */
    SKR_SYSTEM_EVENT_WINDOW_HIDDEN,            /**< Window has been hidden */
    SKR_SYSTEM_EVENT_WINDOW_MOVED,             /**< Window has been moved to [x, y] */
    SKR_SYSTEM_EVENT_WINDOW_RESIZED,           /**< Window has been resized to [x, y] */
    SKR_SYSTEM_EVENT_WINDOW_MINIMIZED,         /**< Window has been minimized */
    SKR_SYSTEM_EVENT_WINDOW_MAXIMIZED,         /**< Window has been maximized */
    SKR_SYSTEM_EVENT_WINDOW_RESTORED,          /**< Window has been restored to normal size and position */
    SKR_SYSTEM_EVENT_WINDOW_MOUSE_ENTER,       /**< Window has gained mouse focus */
    SKR_SYSTEM_EVENT_WINDOW_MOUSE_LEAVE,       /**< Window has lost mouse focus */
    SKR_SYSTEM_EVENT_WINDOW_FOCUS_GAINED,      /**< Window has gained keyboard focus */
    SKR_SYSTEM_EVENT_WINDOW_FOCUS_LOST,        /**< Window has lost keyboard focus */
    SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED,   /**< The window manager requests that the window be closed */
    SKR_SYSTEM_EVENT_WINDOW_ENTER_FULLSCREEN,  /**< The window has entered fullscreen mode */
    SKR_SYSTEM_EVENT_WINDOW_LEAVE_FULLSCREEN,  /**< The window has left fullscreen mode */

    SKR_SYSTEM_EVENT_KEY_DOWN = 0x300,         /**< Key pressed */
    SKR_SYSTEM_EVENT_KEY_UP,                   /**< Key released */

} ESkrSystemEventType;

typedef struct SkrQuitEvent
{
    ESkrSystemEventType type;
} SkrQuitEvent;

typedef struct SkrKeyboardEvent
{
    ESkrSystemEventType type;
    uint64_t window_native_handle;
    SInputKeyCode keycode;      // Virtual key code 
    uint32_t scancode;          // Hardware scancode
    SKeyModifier modifiers;     // Ctrl, Shift, Alt modifiers
    bool down;                  // true = pressed, false = released
    bool repeat;                // true if this is a key repeat

} SkrKeyboardEvent;

typedef struct SkrWindowEvent
{
    ESkrSystemEventType type;
    uint64_t window_native_handle;
    uint32_t x;
    uint32_t y;
} SkrWindowEvent;

typedef union SkrSystemEvent
{
    ESkrSystemEventType type;
    SkrQuitEvent quit;
    SkrKeyboardEvent key;
    SkrWindowEvent window;

    uint8_t storage[64];
} SkrSystemEvent;

SKR_STATIC_ASSERT(
    sizeof(SkrSystemEvent) == sizeof(((SkrSystemEvent*)0)->storage), 
    "SkrSystemEvent size must match SkrSystemEvent storage size"
);

SKR_EXTERN_C_END

#ifdef __cplusplus

template <ESkrSystemEventType e>
struct EventType;

template <> struct EventType<SKR_SYSTEM_EVENT_QUIT> { using Type = SkrQuitEvent; };
template <> struct EventType<SKR_SYSTEM_EVENT_WINDOW_SHOWN> { using Type = SkrWindowEvent; };
template <> struct EventType<SKR_SYSTEM_EVENT_WINDOW_HIDDEN> { using Type = SkrWindowEvent; };
template <> struct EventType<SKR_SYSTEM_EVENT_WINDOW_RESIZED> { using Type = SkrWindowEvent; };
template <> struct EventType<SKR_SYSTEM_EVENT_WINDOW_MINIMIZED> { using Type = SkrWindowEvent; };
template <> struct EventType<SKR_SYSTEM_EVENT_WINDOW_MAXIMIZED> { using Type = SkrWindowEvent; };
template <> struct EventType<SKR_SYSTEM_EVENT_WINDOW_RESTORED> { using Type = SkrWindowEvent; };
template <> struct EventType<SKR_SYSTEM_EVENT_WINDOW_MOUSE_ENTER> { using Type = SkrWindowEvent; };
template <> struct EventType<SKR_SYSTEM_EVENT_WINDOW_MOUSE_LEAVE> { using Type = SkrWindowEvent; };
template <> struct EventType<SKR_SYSTEM_EVENT_WINDOW_FOCUS_GAINED> { using Type = SkrWindowEvent; };
template <> struct EventType<SKR_SYSTEM_EVENT_WINDOW_FOCUS_LOST> { using Type = SkrWindowEvent; };
template <> struct EventType<SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED> { using Type = SkrWindowEvent; };
template <> struct EventType<SKR_SYSTEM_EVENT_WINDOW_ENTER_FULLSCREEN> { using Type = SkrWindowEvent; };
template <> struct EventType<SKR_SYSTEM_EVENT_WINDOW_LEAVE_FULLSCREEN> { using Type = SkrWindowEvent; };
template <> struct EventType<SKR_SYSTEM_EVENT_KEY_DOWN> { using Type = SkrKeyboardEvent; };
template <> struct EventType<SKR_SYSTEM_EVENT_KEY_UP> { using Type = SkrKeyboardEvent; };

#endif