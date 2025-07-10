#include "cocoa_event_source.h"
#include "cocoa_ime.h"
#include "cocoa_window.h"
#include "cocoa_window_manager.h"
#include "SkrSystem/system.h"
#include "SkrSystem/event.h"
#include "SkrCore/log.h"
#include "SkrCore/memory/memory.h"

// Helper function to translate macOS key codes to SInputKeyCode
static SInputKeyCode TranslateCocoaKeyCode(unsigned short keyCode)
{
    // Based on macOS virtual key codes
    switch (keyCode) {
        case 0x00: return KEY_CODE_A;
        case 0x01: return KEY_CODE_S;
        case 0x02: return KEY_CODE_D;
        case 0x03: return KEY_CODE_F;
        case 0x04: return KEY_CODE_H;
        case 0x05: return KEY_CODE_G;
        case 0x06: return KEY_CODE_Z;
        case 0x07: return KEY_CODE_X;
        case 0x08: return KEY_CODE_C;
        case 0x09: return KEY_CODE_V;
        case 0x0B: return KEY_CODE_B;
        case 0x0C: return KEY_CODE_Q;
        case 0x0D: return KEY_CODE_W;
        case 0x0E: return KEY_CODE_E;
        case 0x0F: return KEY_CODE_R;
        case 0x10: return KEY_CODE_Y;
        case 0x11: return KEY_CODE_T;
        case 0x12: return KEY_CODE_1;
        case 0x13: return KEY_CODE_2;
        case 0x14: return KEY_CODE_3;
        case 0x15: return KEY_CODE_4;
        case 0x16: return KEY_CODE_6;
        case 0x17: return KEY_CODE_5;
        case 0x18: return KEY_CODE_Plus;
        case 0x19: return KEY_CODE_9;
        case 0x1A: return KEY_CODE_7;
        case 0x1B: return KEY_CODE_Minus;
        case 0x1C: return KEY_CODE_8;
        case 0x1D: return KEY_CODE_0;
        case 0x1E: return KEY_CODE_RBracket;
        case 0x1F: return KEY_CODE_O;
        case 0x20: return KEY_CODE_U;
        case 0x21: return KEY_CODE_LBracket;
        case 0x22: return KEY_CODE_I;
        case 0x23: return KEY_CODE_P;
        case 0x24: return KEY_CODE_Enter;
        case 0x25: return KEY_CODE_L;
        case 0x26: return KEY_CODE_J;
        case 0x27: return KEY_CODE_Quote;
        case 0x28: return KEY_CODE_K;
        case 0x29: return KEY_CODE_Semicolon;
        case 0x2A: return KEY_CODE_Backslash;
        case 0x2B: return KEY_CODE_Comma;
        case 0x2C: return KEY_CODE_Slash;
        case 0x2D: return KEY_CODE_N;
        case 0x2E: return KEY_CODE_M;
        case 0x2F: return KEY_CODE_Dot;
        case 0x30: return KEY_CODE_Tab;
        case 0x31: return KEY_CODE_SpaceBar;
        case 0x32: return KEY_CODE_Wave;
        case 0x33: return KEY_CODE_Backspace;
        case 0x35: return KEY_CODE_Esc;
        case 0x36: return KEY_CODE_RSystem;
        case 0x37: return KEY_CODE_LSystem;
        case 0x38: return KEY_CODE_LShift;
        case 0x39: return KEY_CODE_CapsLock;
        case 0x3A: return KEY_CODE_LAlt;
        case 0x3B: return KEY_CODE_LCtrl;
        case 0x3C: return KEY_CODE_RShift;
        case 0x3D: return KEY_CODE_RAlt;
        case 0x3E: return KEY_CODE_RCtrl;
        case 0x3F: return KEY_CODE_Invalid; // Function key
        case 0x40: return KEY_CODE_F17;
        case 0x41: return KEY_CODE_Decimal;
        case 0x43: return KEY_CODE_Multiply;
        case 0x45: return KEY_CODE_Add;
        case 0x47: return KEY_CODE_Numlock;
        case 0x48: return KEY_CODE_Invalid; // Volume up
        case 0x49: return KEY_CODE_Invalid; // Volume down
        case 0x4A: return KEY_CODE_Invalid; // Mute
        case 0x4B: return KEY_CODE_Divide;
        case 0x4C: return KEY_CODE_Enter;
        case 0x4E: return KEY_CODE_Subtract;
        case 0x4F: return KEY_CODE_F18;
        case 0x50: return KEY_CODE_F19;
        case 0x51: return KEY_CODE_Plus;
        case 0x52: return KEY_CODE_Numpad0;
        case 0x53: return KEY_CODE_Numpad1;
        case 0x54: return KEY_CODE_Numpad2;
        case 0x55: return KEY_CODE_Numpad3;
        case 0x56: return KEY_CODE_Numpad4;
        case 0x57: return KEY_CODE_Numpad5;
        case 0x58: return KEY_CODE_Numpad6;
        case 0x59: return KEY_CODE_Numpad7;
        case 0x5A: return KEY_CODE_F20;
        case 0x5B: return KEY_CODE_Numpad8;
        case 0x5C: return KEY_CODE_Numpad9;
        case 0x60: return KEY_CODE_F5;
        case 0x61: return KEY_CODE_F6;
        case 0x62: return KEY_CODE_F7;
        case 0x63: return KEY_CODE_F3;
        case 0x64: return KEY_CODE_F8;
        case 0x65: return KEY_CODE_F9;
        case 0x67: return KEY_CODE_F11;
        case 0x69: return KEY_CODE_F13;
        case 0x6A: return KEY_CODE_F16;
        case 0x6B: return KEY_CODE_F14;
        case 0x6D: return KEY_CODE_F10;
        case 0x6F: return KEY_CODE_F12;
        case 0x71: return KEY_CODE_F15;
        case 0x72: return KEY_CODE_Insert;
        case 0x73: return KEY_CODE_Home;
        case 0x74: return KEY_CODE_PageUp;
        case 0x75: return KEY_CODE_Del;
        case 0x76: return KEY_CODE_F4;
        case 0x77: return KEY_CODE_End;
        case 0x78: return KEY_CODE_F2;
        case 0x79: return KEY_CODE_PageDown;
        case 0x7A: return KEY_CODE_F1;
        case 0x7B: return KEY_CODE_Left;
        case 0x7C: return KEY_CODE_Right;
        case 0x7D: return KEY_CODE_Down;
        case 0x7E: return KEY_CODE_Up;
        default: return KEY_CODE_Invalid;
    }
}

// Helper function to translate macOS modifier flags
static uint32_t TranslateCocoaModifiers(NSEventModifierFlags flags)
{
    uint32_t modifiers = 0;
    
    if (flags & NSEventModifierFlagShift)
        modifiers |= KEY_MODIFIER_LShift;
    if (flags & NSEventModifierFlagControl)
        modifiers |= KEY_MODIFIER_LCtrl;
    if (flags & NSEventModifierFlagOption)
        modifiers |= KEY_MODIFIER_LAlt;
    if (flags & NSEventModifierFlagCommand)
        modifiers |= KEY_MODIFIER_LAlt; // Command key maps to Alt
    if (flags & NSEventModifierFlagCapsLock)
        // No caps lock modifier in enum
    if (flags & NSEventModifierFlagNumericPad)
        // No num lock modifier in enum
    
    return modifiers;
}

// Helper function to translate mouse button
static InputMouseButtonFlags TranslateCocoaMouseButton(NSInteger button)
{
    switch (button) {
        case 0: return InputMouseLeftButton;
        case 1: return InputMouseRightButton;
        case 2: return InputMouseMiddleButton;
        case 3: return InputMouseButton4;
        case 4: return InputMouseButton5;
        default: return InputMouseLeftButton;
    }
}

CocoaEventSource::CocoaEventSource(skr::SystemApp* app) SKR_NOEXCEPT
    : app_(app)
    , ime_(nullptr)
{
    queue_lock_ = [[NSLock alloc] init];
    
    // Ensure NSApplication is initialized
    @autoreleasepool {
        if (!NSApp) {
            [NSApplication sharedApplication];
        }
        
        // Set activation policy to regular app
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        // Activate the app (bring to front)
        [NSApp activateIgnoringOtherApps:YES];
    }
}

CocoaEventSource::~CocoaEventSource() SKR_NOEXCEPT
{
    [queue_lock_ release];
}

bool CocoaEventSource::poll_event(SkrSystemEvent& event) SKR_NOEXCEPT
{
    // First check our custom event queue
    [queue_lock_ lock];
    if (!event_queue_.is_empty()) {
        event = event_queue_[0];
        event_queue_.remove_at(0);
        [queue_lock_ unlock];
        return true;
    }
    [queue_lock_ unlock];
    
    // Poll NSApplication event queue
    @autoreleasepool {
        NSEvent* ns_event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                untilDate:[NSDate distantPast]
                                                   inMode:NSDefaultRunLoopMode
                                                  dequeue:YES];
        
        if (ns_event) {
            // Let NSApp process the event first
            // This will trigger window delegate callbacks for window events
            [NSApp sendEvent:ns_event];
            
            // Translate keyboard/mouse events
            if (process_ns_event(ns_event, event)) {
                return true;
            }
            
            // Check if window events were added to queue by delegates
            [queue_lock_ lock];
            if (!event_queue_.is_empty()) {
                event = event_queue_[0];
                event_queue_.remove_at(0);
                [queue_lock_ unlock];
                return true;
            }
            [queue_lock_ unlock];
        }
    }
    
    return false;
}

bool CocoaEventSource::wait_events(uint32_t timeout_ms) SKR_NOEXCEPT
{
    @autoreleasepool {
        NSDate* untilDate = timeout_ms == 0 ? [NSDate distantFuture] : [NSDate dateWithTimeIntervalSinceNow:timeout_ms / 1000.0];
        
        NSEvent* ns_event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                untilDate:untilDate
                                                   inMode:NSDefaultRunLoopMode
                                                  dequeue:NO];
        
        return ns_event != nil;
    }
}

void CocoaEventSource::push_event(const SkrSystemEvent& event) SKR_NOEXCEPT
{
    [queue_lock_ lock];
    event_queue_.add(event);
    [queue_lock_ unlock];
    
    // Wake up the event loop if it's waiting
    [NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined
                                         location:NSMakePoint(0, 0)
                                    modifierFlags:0
                                        timestamp:0
                                     windowNumber:0
                                          context:nil
                                          subtype:0
                                            data1:0
                                            data2:0]
             atStart:NO];
}

bool CocoaEventSource::process_ns_event(NSEvent* ns_event, SkrSystemEvent& event) SKR_NOEXCEPT
{
    event.type = SKR_SYSTEM_EVENT_INVALID;
    // Note: timestamp is not part of the event structure
    
    switch (ns_event.type) {
        // Key events
        case NSEventTypeKeyDown:
        case NSEventTypeKeyUp:
            translate_key_event(ns_event, event);
            return true;
            
        // Mouse events
        case NSEventTypeLeftMouseDown:
        case NSEventTypeLeftMouseUp:
        case NSEventTypeRightMouseDown:
        case NSEventTypeRightMouseUp:
        case NSEventTypeOtherMouseDown:
        case NSEventTypeOtherMouseUp:
        case NSEventTypeMouseMoved:
        case NSEventTypeLeftMouseDragged:
        case NSEventTypeRightMouseDragged:
        case NSEventTypeOtherMouseDragged:
        case NSEventTypeMouseEntered:
        case NSEventTypeMouseExited:
        case NSEventTypeScrollWheel:
            translate_mouse_event(ns_event, event);
            return true;
            
        // Window events are handled by window delegates
        // They push events directly to our queue
        default:
            return false;
    }
    
    return false;
}

void CocoaEventSource::translate_key_event(NSEvent* ns_event, SkrSystemEvent& event) SKR_NOEXCEPT
{
    event.type = (ns_event.type == NSEventTypeKeyDown) ? SKR_SYSTEM_EVENT_KEY_DOWN : SKR_SYSTEM_EVENT_KEY_UP;
    event.key.keycode = TranslateCocoaKeyCode(ns_event.keyCode);
    event.key.modifiers = TranslateCocoaModifiers(ns_event.modifierFlags);
    event.key.repeat = ns_event.isARepeat;
    
    // Get the window
    NSWindow* nsWindow = ns_event.window;
    if (nsWindow) {
        auto* window_manager = app_->get_window_manager();
        if (window_manager) {
            event.key.window_native_handle = (uint64_t)(__bridge void*)nsWindow;
        }
    }
    
    // Forward to IME if active
    if (ime_ && ns_event.type == NSEventTypeKeyDown) {
        ime_->process_text_event(ns_event);
    }
}

void CocoaEventSource::translate_mouse_event(NSEvent* ns_event, SkrSystemEvent& event) SKR_NOEXCEPT
{
    // Get window and convert coordinates
    NSWindow* nsWindow = ns_event.window;
    skr::SystemWindow* window = nullptr;
    
    if (nsWindow) {
        auto* window_manager = app_->get_window_manager();
        if (window_manager) {
            window = window_manager->get_window_by_native_handle((__bridge void*)nsWindow);
        }
    }
    
    NSPoint location = ns_event.locationInWindow;
    if (window) {
        // Convert from bottom-left to top-left origin
        NSRect contentRect = [nsWindow contentRectForFrameRect:nsWindow.frame];
        location.y = contentRect.size.height - location.y;
    }
    
    switch (ns_event.type) {
        case NSEventTypeLeftMouseDown:
        case NSEventTypeRightMouseDown:
        case NSEventTypeOtherMouseDown:
            event.type = SKR_SYSTEM_EVENT_MOUSE_BUTTON_DOWN;
            event.mouse.button = TranslateCocoaMouseButton(ns_event.buttonNumber);
            event.mouse.x = (int32_t)location.x;
            event.mouse.y = (int32_t)location.y;
            event.mouse.modifiers = TranslateCocoaModifiers(ns_event.modifierFlags);
            event.mouse.window_native_handle = window ? (uint64_t)window->get_native_handle() : 0;
            break;
            
        case NSEventTypeLeftMouseUp:
        case NSEventTypeRightMouseUp:
        case NSEventTypeOtherMouseUp:
            event.type = SKR_SYSTEM_EVENT_MOUSE_BUTTON_UP;
            event.mouse.button = TranslateCocoaMouseButton(ns_event.buttonNumber);
            event.mouse.x = (int32_t)location.x;
            event.mouse.y = (int32_t)location.y;
            event.mouse.modifiers = TranslateCocoaModifiers(ns_event.modifierFlags);
            event.mouse.window_native_handle = window ? (uint64_t)window->get_native_handle() : 0;
            break;
            
        case NSEventTypeMouseMoved:
        case NSEventTypeLeftMouseDragged:
        case NSEventTypeRightMouseDragged:
        case NSEventTypeOtherMouseDragged:
            event.type = SKR_SYSTEM_EVENT_MOUSE_MOVE;
            event.mouse.x = (int32_t)location.x;
            event.mouse.y = (int32_t)location.y;
            event.mouse.modifiers = TranslateCocoaModifiers(ns_event.modifierFlags);
            event.mouse.window_native_handle = window ? (uint64_t)window->get_native_handle() : 0;
            break;
            
        case NSEventTypeScrollWheel:
            event.type = SKR_SYSTEM_EVENT_MOUSE_WHEEL;
            event.mouse.wheel_x = (float)ns_event.scrollingDeltaX;
            event.mouse.wheel_y = (float)ns_event.scrollingDeltaY;
            event.mouse.modifiers = TranslateCocoaModifiers(ns_event.modifierFlags);
            event.mouse.window_native_handle = window ? (uint64_t)window->get_native_handle() : 0;
            break;
            
        case NSEventTypeMouseEntered:
            event.type = SKR_SYSTEM_EVENT_MOUSE_ENTER;
            event.mouse.window_native_handle = window ? (uint64_t)window->get_native_handle() : 0;
            break;
            
        case NSEventTypeMouseExited:
            event.type = SKR_SYSTEM_EVENT_MOUSE_LEAVE;
            event.mouse.window_native_handle = window ? (uint64_t)window->get_native_handle() : 0;
            break;
    }
}