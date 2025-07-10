#pragma once
#include "SkrSystem/IME.h"
#include "SkrContainers/string.hpp"
#include "SkrContainers/vector.hpp"
#include <mutex>

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#else
// Forward declarations for C++
typedef struct objc_object NSView;
typedef struct objc_object CocoaIMEView;
typedef struct objc_object NSEvent;
#endif

class CocoaEventSource;

#ifdef __OBJC__
// NSView that implements NSTextInputClient protocol
@interface CocoaIMEView : NSView<NSTextInputClient>
{
    skr::IMEEventCallbacks callbacks_;
    skr::String marked_text_;
    NSRange selected_range_;
    NSRange marked_range_;
    skr::SystemWindow* current_window_;
    skr::IMETextInputArea input_area_;
}

- (void)setCallbacks:(const skr::IMEEventCallbacks&)callbacks;
- (void)setCurrentWindow:(skr::SystemWindow*)window;
- (void)setInputArea:(const skr::IMETextInputArea&)area;
- (void)clearComposition;
- (void)commitComposition;
- (skr::String)getMarkedText;
- (NSRange)getMarkedRange;

@end
#endif

class CocoaIME : public skr::IME {
public:
    CocoaIME() SKR_NOEXCEPT;
    ~CocoaIME() SKR_NOEXCEPT override;

    // Session management
    void start_text_input(skr::SystemWindow* window) SKR_NOEXCEPT override;
    void start_text_input_ex(skr::SystemWindow* window, 
                           skr::EIMEInputType input_type,
                           skr::EIMECapitalization capitalization = skr::EIMECapitalization::Sentences,
                           bool autocorrect = true,
                           bool multiline = false) SKR_NOEXCEPT override;
    void stop_text_input() SKR_NOEXCEPT override;
    bool is_text_input_active() const SKR_NOEXCEPT override;
    skr::SystemWindow* get_text_input_window() const SKR_NOEXCEPT override;

    // Input area
    void set_text_input_area(const skr::IMETextInputArea& area) SKR_NOEXCEPT override;
    skr::IMETextInputArea get_text_input_area() const SKR_NOEXCEPT override;

    // Event callbacks
    void set_event_callbacks(const skr::IMEEventCallbacks& callbacks) SKR_NOEXCEPT override;

    // State query
    skr::IMECompositionInfo get_composition_info() const SKR_NOEXCEPT override;
    skr::IMECandidateInfo get_candidates_info() const SKR_NOEXCEPT override;
    void clear_composition() SKR_NOEXCEPT override;
    void commit_composition() SKR_NOEXCEPT override;
    bool is_screen_keyboard_supported() const SKR_NOEXCEPT override;

    // Cocoa specific
    void set_event_source(CocoaEventSource* source) SKR_NOEXCEPT { event_source_ = source; }
    void process_text_event(NSEvent* event) SKR_NOEXCEPT;

private:
    CocoaIMEView* ime_view_;
    skr::SystemWindow* current_window_;
    skr::IMETextInputArea input_area_;
    skr::IMEEventCallbacks callbacks_;
    CocoaEventSource* event_source_;
    
    mutable std::mutex state_mutex_;
    bool is_active_;
    
    // State tracking
    skr::IMECompositionInfo composition_info_;
    skr::IMECandidateInfo candidate_info_;
};