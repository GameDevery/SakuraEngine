#pragma once
#include "SkrSystem/event.h"
#include "SkrContainers/vector.hpp"

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#else
// Forward declarations for C++
typedef struct objc_object NSEvent;
typedef struct objc_object NSLock;
#endif

namespace skr {
class SystemApp;
}
class CocoaIME;

class CocoaEventSource : public skr::ISystemEventSource {
public:
    explicit CocoaEventSource(skr::SystemApp* app) SKR_NOEXCEPT;
    ~CocoaEventSource() SKR_NOEXCEPT override;

    // ISystemEventSource implementation
    bool poll_event(SkrSystemEvent& event) SKR_NOEXCEPT override;
    bool wait_events(uint32_t timeout_ms = 0) SKR_NOEXCEPT override;

    // Cocoa specific
    void set_ime(CocoaIME* ime) SKR_NOEXCEPT { ime_ = ime; }
    
    // Called from Cocoa delegates to push events
    void push_event(const SkrSystemEvent& event) SKR_NOEXCEPT;

private:
    bool process_ns_event(NSEvent* ns_event, SkrSystemEvent& event) SKR_NOEXCEPT;
    void translate_key_event(NSEvent* ns_event, SkrSystemEvent& event) SKR_NOEXCEPT;
    void translate_mouse_event(NSEvent* ns_event, SkrSystemEvent& event) SKR_NOEXCEPT;
    void translate_window_event(NSEvent* ns_event, SkrSystemEvent& event) SKR_NOEXCEPT;
    
    skr::SystemApp* app_;
    CocoaIME* ime_;
    
    // Event queue for custom events from delegates
    skr::Vector<SkrSystemEvent> event_queue_;
    NSLock* queue_lock_;
};