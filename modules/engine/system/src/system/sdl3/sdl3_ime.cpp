#include "sdl3_ime.h"
#include "sdl3_system_app.h"
#include "sdl3_window.h"
#include "SkrCore/log.h"
#include "SkrCore/memory/memory.h"


namespace skr {

SDL3IME::SDL3IME(SDL3SystemApp* app) SKR_NOEXCEPT
    : text_input_props_(SDL_CreateProperties())
{
    if (!text_input_props_)
    {
        SKR_LOG_ERROR(u8"Failed to create SDL properties for text input");
    }
}

SDL3IME::~SDL3IME() SKR_NOEXCEPT
{
    stop_text_input();
    
    if (text_input_props_)
    {
        SDL_DestroyProperties(text_input_props_);
    }
}

void SDL3IME::start_text_input(SystemWindow* window)
{
    start_text_input_ex(window, EIMEInputType::Text);
}

void SDL3IME::start_text_input_ex(SystemWindow* window, 
                                  EIMEInputType input_type,
                                  EIMECapitalization capitalization,
                                  bool autocorrect,
                                  bool multiline)
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    // Validate input
    if (!window)
    {
        SKR_LOG_ERROR(u8"Cannot start text input with null window");
        return;
    }
    
    // Stop previous input session
    if (active_window_)
    {
        if (auto* sdl_window = get_sdl_window(active_window_))
        {
            SDL_StopTextInput(sdl_window);
        }
    }
    
    active_window_ = static_cast<SDL3Window*>(window);
    
    // Reset state
    reset_composition_state();
    
    // Configure SDL text input properties
    SDL_Window* sdl_window = get_sdl_window(active_window_);
    if (!sdl_window)
    {
        SKR_LOG_ERROR(u8"Failed to get SDL window handle");
        active_window_ = nullptr;
        return;
    }
    
    // Set properties for SDL3
    SDL_SetNumberProperty(text_input_props_, SDL_PROP_TEXTINPUT_TYPE_NUMBER, 
                         static_cast<int>(convert_input_type(input_type)));
    SDL_SetNumberProperty(text_input_props_, SDL_PROP_TEXTINPUT_CAPITALIZATION_NUMBER,
                         static_cast<int>(convert_capitalization(capitalization)));
    SDL_SetBooleanProperty(text_input_props_, SDL_PROP_TEXTINPUT_AUTOCORRECT_BOOLEAN, autocorrect);
    SDL_SetBooleanProperty(text_input_props_, SDL_PROP_TEXTINPUT_MULTILINE_BOOLEAN, multiline);
    
    // Start text input with properties
    SDL_StartTextInputWithProperties(sdl_window, text_input_props_);
    
    // Update text input area
    update_sdl_text_input_area();
    
    
    // Notify callbacks
    if (callbacks_.on_composition_start)
    {
        callbacks_.on_composition_start();
    }
}

void SDL3IME::stop_text_input()
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (!active_window_)
        return;
        
    if (auto* sdl_window = get_sdl_window(active_window_))
    {
        SDL_StopTextInput(sdl_window);
    }
    
    // Commit any pending composition
    if (composition_state_.active && !composition_state_.text.is_empty())
    {
        commit_current_text();
    }
    
    // Notify callbacks
    if (callbacks_.on_composition_end)
    {
        callbacks_.on_composition_end(false);
    }
    
    active_window_ = nullptr;
    reset_composition_state();
}

bool SDL3IME::is_text_input_active() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (!active_window_)
        return false;
        
    if (auto* sdl_window = get_sdl_window(active_window_))
    {
        return SDL_TextInputActive(sdl_window);
    }
    return false;
}

SystemWindow* SDL3IME::get_text_input_window() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    return active_window_;
}

void SDL3IME::set_text_input_area(const IMETextInputArea& area)
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    input_area_ = area;
    update_sdl_text_input_area();
    
    // Notify callbacks
    if (callbacks_.on_input_area_change)
    {
        callbacks_.on_input_area_change(area);
    }
}

IMETextInputArea SDL3IME::get_text_input_area() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    return input_area_;
}



void SDL3IME::set_event_callbacks(const IMEEventCallbacks& callbacks)
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    callbacks_ = callbacks;
}

bool SDL3IME::is_screen_keyboard_supported() const
{
    return SDL_HasScreenKeyboardSupport();
}

IMECompositionInfo SDL3IME::get_composition_info() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    IMECompositionInfo info;
    info.text = composition_state_.text;
    info.cursor_pos = composition_state_.cursor_pos;
    info.selection_start = composition_state_.selection_start;
    info.selection_length = composition_state_.selection_length;
    return info;
}

IMECandidateInfo SDL3IME::get_candidates_info() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    IMECandidateInfo info;
    info.candidates = candidate_state_.candidates;
    info.selected_index = candidate_state_.selected_index;
    info.page_start = 0;
    info.page_size = kDefaultPageSize;
    info.total_candidates = candidate_state_.total_candidates;
    info.preferred_layout = candidate_state_.layout;
    return info;
}

void SDL3IME::clear_composition()
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    reset_composition_state();
    
    // Platform specific clear
    if (!active_window_)
        return;
        
    if (auto* sdl_window = get_sdl_window(active_window_))
    {
        // SDL3 doesn't have SDL_ClearComposition
        // We can restart text input to clear composition
        SDL_StopTextInput(sdl_window);
        SDL_StartTextInputWithProperties(sdl_window, text_input_props_);
    }
}

void SDL3IME::commit_composition()
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (composition_state_.active && !composition_state_.text.is_empty())
    {
        commit_current_text();
        reset_composition_state();
    }
}


void SDL3IME::process_event(const SDL_Event& event)
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    switch (event.type)
    {
        case SDL_EVENT_TEXT_EDITING:
        {
            const SDL_TextEditingEvent& edit_event = event.edit;
            
            // Update composition state
            update_composition_state(edit_event);
            
            
            // Notify callback
            if (callbacks_.on_composition_update)
            {
                callbacks_.on_composition_update(get_composition_info());
            }
            break;
        }
        
        case SDL_EVENT_TEXT_EDITING_CANDIDATES:
        {
            const SDL_TextEditingCandidatesEvent& candidates_event = event.edit_candidates;
            
            // Update candidate state
            update_candidate_state(candidates_event);
            
            // Notify callback
            if (callbacks_.on_candidates_update)
            {
                callbacks_.on_candidates_update(get_candidates_info());
            }
            break;
        }
        
        case SDL_EVENT_TEXT_INPUT:
        {
            const SDL_TextInputEvent& text_event = event.text;
            
            // Text has been committed
            composition_state_.active = false;
            
            // Notify callback
            if (callbacks_.on_text_input)
            {
                callbacks_.on_text_input(skr::String((const char8_t*)text_event.text));
            }
            
            // Clear composition state after commit
            reset_composition_state();
            
            // Notify composition end
            if (callbacks_.on_composition_end)
            {
                callbacks_.on_composition_end(true);
            }
            break;
        }
    }
}

// Helper methods implementation

void SDL3IME::update_sdl_text_input_area()
{
    if (!active_window_)
        return;
        
    if (auto* sdl_window = get_sdl_window(active_window_))
    {
        SDL_Rect rect = {
            .x = input_area_.position.x,
            .y = input_area_.position.y,
            .w = static_cast<int>(input_area_.size.x),
            .h = static_cast<int>(input_area_.size.y)
        };
        
        SDL_SetTextInputArea(sdl_window, &rect, input_area_.cursor_height);
    }
}

SDL_TextInputType SDL3IME::convert_input_type(EIMEInputType type) const
{
    switch (type)
    {
        case EIMEInputType::Text:
        case EIMEInputType::Search:
        case EIMEInputType::Multiline:
            return SDL_TEXTINPUT_TYPE_TEXT;
            
        case EIMEInputType::Email:
            return SDL_TEXTINPUT_TYPE_TEXT_EMAIL;
            
        case EIMEInputType::Username:
            return SDL_TEXTINPUT_TYPE_TEXT_USERNAME;
            
        case EIMEInputType::Password:
            return SDL_TEXTINPUT_TYPE_TEXT_PASSWORD_HIDDEN;
            
        case EIMEInputType::URL:
            return SDL_TEXTINPUT_TYPE_TEXT; // SDL3 doesn't have URL type
            
        case EIMEInputType::Telephone:
            return SDL_TEXTINPUT_TYPE_NUMBER; // SDL3 doesn't have phone number type
            
        case EIMEInputType::Number:
        case EIMEInputType::Date:
        case EIMEInputType::Time:
            return SDL_TEXTINPUT_TYPE_NUMBER;
            
        case EIMEInputType::NumberPassword:
            return SDL_TEXTINPUT_TYPE_NUMBER_PASSWORD_HIDDEN;
            
        default:
            return SDL_TEXTINPUT_TYPE_TEXT;
    }
}

SDL_Capitalization SDL3IME::convert_capitalization(EIMECapitalization cap) const
{
    switch (cap)
    {
        case EIMECapitalization::None:
            return SDL_CAPITALIZE_NONE;
        case EIMECapitalization::Sentences:
            return SDL_CAPITALIZE_SENTENCES;
        case EIMECapitalization::Words:
            return SDL_CAPITALIZE_WORDS;
        case EIMECapitalization::Characters:
            return SDL_CAPITALIZE_LETTERS;
        default:
            return SDL_CAPITALIZE_NONE;
    }
}


void SDL3IME::reset_composition_state()
{
    composition_state_.text.clear();
    composition_state_.cursor_pos = 0;
    composition_state_.selection_start = kInvalidIndex;
    composition_state_.selection_length = kInvalidIndex;
    composition_state_.active = false;
    
    candidate_state_.candidates.clear();
    candidate_state_.selected_index = kInvalidIndex;
    candidate_state_.visible = false;
}

void SDL3IME::commit_current_text()
{
    if (!composition_state_.text.is_empty() && callbacks_.on_text_input)
    {
        callbacks_.on_text_input(composition_state_.text);
    }
}


SDL_Window* SDL3IME::get_sdl_window(SystemWindow* window) const
{
    if (!window)
        return nullptr;
        
    auto* sdl3_window = static_cast<SDL3Window*>(window);
    return sdl3_window->get_sdl_window();
}

void SDL3IME::update_composition_state(const SDL_TextEditingEvent& event)
{
    composition_state_.active = true;
    composition_state_.text = skr::String((const char8_t*)event.text);
    composition_state_.cursor_pos = event.start;
    composition_state_.selection_start = event.start;
    composition_state_.selection_length = event.length;
}

void SDL3IME::update_candidate_state(const SDL_TextEditingCandidatesEvent& event)
{
    candidate_state_.candidates.clear();
    candidate_state_.candidates.reserve(event.num_candidates);
    
    for (int32_t i = 0; i < event.num_candidates; ++i)
    {
        candidate_state_.candidates.emplace_back(
            (const char8_t*)event.candidates[i]
        );
    }
    
    candidate_state_.total_candidates = event.num_candidates;
    candidate_state_.selected_index = event.selected_candidate;
    candidate_state_.layout = event.horizontal ? 
        EIMECandidateLayout::Horizontal : EIMECandidateLayout::Vertical;
    candidate_state_.visible = (event.num_candidates > 0);
}

// Factory methods
IME* IME::Create(struct SystemApp* app)
{
    return SkrNew<SDL3IME>(static_cast<SDL3SystemApp*>(app));
}

void IME::Destroy(IME* ime)
{
    SkrDelete(ime);
}

} // namespace skr