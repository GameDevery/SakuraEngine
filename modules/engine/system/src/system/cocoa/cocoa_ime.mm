#include "cocoa_ime.h"
#include "cocoa_event_source.h"
#include "cocoa_window.h"
#include "SkrCore/log.h"
#include "SkrCore/memory/memory.h"

@implementation CocoaIMEView

- (instancetype)initWithFrame:(NSRect)frameRect
{
    self = [super initWithFrame:frameRect];
    if (self) {
        marked_text_ = skr::String();
        selected_range_ = NSMakeRange(0, 0);
        marked_range_ = NSMakeRange(NSNotFound, 0);
        current_window_ = nullptr;
        callbacks_ = {};
        input_area_ = {};
    }
    return self;
}

- (void)setCallbacks:(const skr::IMEEventCallbacks&)callbacks
{
    callbacks_ = callbacks;
}

- (void)setCurrentWindow:(skr::SystemWindow*)window
{
    current_window_ = window;
}

- (void)setInputArea:(const skr::IMETextInputArea&)area
{
    input_area_ = area;
}

- (void)clearComposition
{
    marked_text_.clear();
    marked_range_ = NSMakeRange(NSNotFound, 0);
    selected_range_ = NSMakeRange(0, 0);
    [[self inputContext] discardMarkedText];
}

- (void)commitComposition
{
    if (!marked_text_.is_empty()) {
        if (callbacks_.on_text_input) {
            callbacks_.on_text_input(marked_text_);
        }
        [self clearComposition];
    }
}

- (skr::String)getMarkedText
{
    return marked_text_;
}

- (NSRange)getMarkedRange
{
    return marked_range_;
}

#pragma mark - NSTextInputClient Protocol

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange
{
    NSString* text = nil;
    
    if ([string isKindOfClass:[NSString class]]) {
        text = string;
    } else if ([string isKindOfClass:[NSAttributedString class]]) {
        text = [string string];
    }
    
    if (text && text.length > 0) {
        // Clear any marked text first
        [self clearComposition];
        
        // Send the committed text
        if (callbacks_.on_text_input) {
            callbacks_.on_text_input(skr::String((const char8_t*)[text UTF8String]));
        }
    }
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange
{
    NSString* text = nil;
    
    if ([string isKindOfClass:[NSString class]]) {
        text = string;
    } else if ([string isKindOfClass:[NSAttributedString class]]) {
        text = [string string];
    }
    
    if (text) {
        marked_text_ = skr::String((const char8_t*)[text UTF8String]);
        marked_range_ = NSMakeRange(0, text.length);
        selected_range_ = selectedRange;
        
        // Notify about composition update
        if (callbacks_.on_composition_update) {
            skr::IMECompositionInfo info = {};
            info.text = marked_text_;
            info.cursor_pos = (int32_t)selectedRange.location;
            info.selection_start = (int32_t)selectedRange.location;
            info.selection_length = (int32_t)selectedRange.length;
            
            callbacks_.on_composition_update(info);
        }
    } else {
        [self clearComposition];
    }
}

- (void)unmarkText
{
    [self commitComposition];
}

- (NSRange)selectedRange
{
    return selected_range_;
}

- (NSRange)markedRange
{
    return marked_range_;
}

- (BOOL)hasMarkedText
{
    return marked_range_.location != NSNotFound;
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actualRange
{
    // For now, return nil to indicate we don't support this
    return nil;
}

- (NSArray<NSAttributedStringKey>*)validAttributesForMarkedText
{
    return @[];
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange
{
    // Return the input area rectangle for IME candidate window positioning
    if (current_window_) {
        CocoaWindow* cocoa_window = static_cast<CocoaWindow*>(current_window_);
        NSWindow* nsWindow = cocoa_window->get_nswindow();
        
        // Convert input area to screen coordinates
        NSRect rect = NSMakeRect(
            input_area_.position.x,
            input_area_.position.y,
            input_area_.size.x,
            input_area_.size.y
        );
        
        // Convert from view coordinates to window coordinates
        rect = [self convertRect:rect toView:nil];
        
        // Convert from window coordinates to screen coordinates
        rect = [nsWindow convertRectToScreen:rect];
        
        return rect;
    }
    
    return NSMakeRect(0, 0, 0, 0);
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point
{
    // Not implemented - return 0
    return 0;
}

- (void)doCommandBySelector:(SEL)selector
{
    // Handle special commands like cancel
    if (selector == @selector(cancelOperation:)) {
        [self clearComposition];
    }
}

@end

CocoaIME::CocoaIME() SKR_NOEXCEPT
    : ime_view_(nil)
    , current_window_(nullptr)
    , event_source_(nullptr)
    , is_active_(false)
{
    @autoreleasepool {
        // Create the IME view
        ime_view_ = [[CocoaIMEView alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
        
        SKR_LOG_INFO(u8"[CocoaIME] Initialized");
    }
}

CocoaIME::~CocoaIME() SKR_NOEXCEPT
{
    @autoreleasepool {
        stop_text_input();
        
        if (ime_view_) {
            [ime_view_ release];
        }
    }
}

void CocoaIME::start_text_input(skr::SystemWindow* window) SKR_NOEXCEPT
{
    start_text_input_ex(window, skr::EIMEInputType::Text);
}

void CocoaIME::start_text_input_ex(skr::SystemWindow* window, 
                                 skr::EIMEInputType input_type,
                                 skr::EIMECapitalization capitalization,
                                 bool autocorrect,
                                 bool multiline) SKR_NOEXCEPT
{
    @autoreleasepool {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        if (!window) {
            SKR_LOG_ERROR(u8"[CocoaIME] Cannot start text input without window");
            return;
        }
        
        current_window_ = window;
        is_active_ = true;
        
        // Set the window for the IME view
        [ime_view_ setCurrentWindow:window];
        
        // Add IME view to window if needed
        CocoaWindow* cocoa_window = static_cast<CocoaWindow*>(window);
        NSWindow* nsWindow = cocoa_window->get_nswindow();
        NSView* contentView = [nsWindow contentView];
        
        if ([ime_view_ superview] != contentView) {
            [contentView addSubview:ime_view_];
        }
        
        // Make the IME view the first responder to receive text input
        [nsWindow makeFirstResponder:ime_view_];
        
        // Clear any existing composition
        [ime_view_ clearComposition];
        
        SKR_LOG_INFO(u8"[CocoaIME] Started text input for window");
    }
}

void CocoaIME::stop_text_input() SKR_NOEXCEPT
{
    @autoreleasepool {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        if (!is_active_) {
            return;
        }
        
        // Commit any pending composition
        [ime_view_ commitComposition];
        
        // Remove from superview
        [ime_view_ removeFromSuperview];
        
        // Reset state
        current_window_ = nullptr;
        is_active_ = false;
        
        SKR_LOG_INFO(u8"[CocoaIME] Stopped text input");
    }
}

bool CocoaIME::is_text_input_active() const SKR_NOEXCEPT
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    return is_active_;
}

skr::SystemWindow* CocoaIME::get_text_input_window() const SKR_NOEXCEPT
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_window_;
}

void CocoaIME::set_text_input_area(const skr::IMETextInputArea& area) SKR_NOEXCEPT
{
    @autoreleasepool {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        input_area_ = area;
        [ime_view_ setInputArea:area];
        
        // Notify the input context about the change
        [[ime_view_ inputContext] invalidateCharacterCoordinates];
    }
}

skr::IMETextInputArea CocoaIME::get_text_input_area() const SKR_NOEXCEPT
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    return input_area_;
}

void CocoaIME::set_event_callbacks(const skr::IMEEventCallbacks& callbacks) SKR_NOEXCEPT
{
    @autoreleasepool {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        callbacks_ = callbacks;
        [ime_view_ setCallbacks:callbacks];
    }
}

skr::IMECompositionInfo CocoaIME::get_composition_info() const SKR_NOEXCEPT
{
    @autoreleasepool {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        skr::IMECompositionInfo info = {};
        
        if ([ime_view_ hasMarkedText]) {
            info.text = [ime_view_ getMarkedText];
            
            NSRange marked_range = [ime_view_ getMarkedRange];
            NSRange selected_range = [ime_view_ selectedRange];
            
            info.cursor_pos = (int32_t)selected_range.location;
            info.selection_start = (int32_t)selected_range.location;
            info.selection_length = (int32_t)selected_range.length;
        }
        
        return info;
    }
}

skr::IMECandidateInfo CocoaIME::get_candidates_info() const SKR_NOEXCEPT
{
    // macOS handles candidate windows natively
    // We don't have direct access to candidate information
    std::lock_guard<std::mutex> lock(state_mutex_);
    return candidate_info_;
}

void CocoaIME::clear_composition() SKR_NOEXCEPT
{
    @autoreleasepool {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        if (is_active_) {
            [ime_view_ clearComposition];
        }
    }
}

void CocoaIME::commit_composition() SKR_NOEXCEPT
{
    @autoreleasepool {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        if (is_active_) {
            [ime_view_ commitComposition];
        }
    }
}

bool CocoaIME::is_screen_keyboard_supported() const SKR_NOEXCEPT
{
    // macOS doesn't have a screen keyboard in the traditional sense
    // The on-screen keyboard is a system accessibility feature
    return false;
}

void CocoaIME::process_text_event(NSEvent* event) SKR_NOEXCEPT
{
    @autoreleasepool {
        std::lock_guard<std::mutex> lock(state_mutex_);
        
        if (!is_active_ || !ime_view_) {
            return;
        }
        
        // Let the IME view handle the event
        [[ime_view_ inputContext] handleEvent:event];
    }
}

bool CocoaIME::has_clipboard_text() const SKR_NOEXCEPT
{
    @autoreleasepool {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        return [pasteboard canReadItemWithDataConformingToTypes:@[NSPasteboardTypeString]] == YES;
    }
}

skr::String CocoaIME::get_clipboard_text() const SKR_NOEXCEPT
{
    @autoreleasepool {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        NSString* text = [pasteboard stringForType:NSPasteboardTypeString];
        
        if (text) {
            return skr::String((const char8_t*)[text UTF8String]);
        }
        
        return skr::String();
    }
}

void CocoaIME::set_clipboard_text(const skr::String& text) SKR_NOEXCEPT
{
    @autoreleasepool {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        
        NSString* nsText = [NSString stringWithUTF8String:text.c_str_raw()];
        [pasteboard setString:nsText forType:NSPasteboardTypeString];
    }
}