#pragma once
#include "SkrSystem/IME.h"
#include <SDL3/SDL.h>
#include <mutex>

namespace skr {

class SDL3SystemApp;
class SDL3Window;

// SDL3 IME implementation
class SDL3IME : public IME
{
public:
    SDL3IME(SDL3SystemApp* app) SKR_NOEXCEPT;
    ~SDL3IME() SKR_NOEXCEPT override;

    // IME interface implementation
    void start_text_input(SystemWindow* window) override;
    void start_text_input_ex(SystemWindow* window, 
                            EIMEInputType input_type,
                            EIMECapitalization capitalization = EIMECapitalization::Sentences,
                            bool autocorrect = true,
                            bool multiline = false) override;
    void stop_text_input() override;
    bool is_text_input_active() const override;
    SystemWindow* get_text_input_window() const override;
    
    void set_text_input_area(const IMETextInputArea& area) override;
    IMETextInputArea get_text_input_area() const override;
    
    void set_event_callbacks(const IMEEventCallbacks& callbacks) override;
    
    bool is_screen_keyboard_supported() const override;
    
    IMECompositionInfo get_composition_info() const override;
    IMECandidateInfo get_candidates_info() const override;
    
    void clear_composition() override;
    void commit_composition() override;

    // SDL3 specific - process SDL events
    void process_event(const SDL_Event& event);

private:
    // Internal state management
    struct CompositionState
    {
        skr::String text;
        int32_t cursor_pos = 0;
        int32_t selection_start = -1;
        int32_t selection_length = -1;
        bool active = false;
    };

    struct CandidateState
    {
        skr::Vector<skr::String> candidates;
        int32_t selected_index = -1;
        int32_t total_candidates = 0;
        bool visible = false;
        EIMECandidateLayout layout = EIMECandidateLayout::Vertical;
    };
    
    // Constants
    static constexpr int32_t kDefaultPageSize = 5;
    static constexpr int32_t kInvalidIndex = -1;

    // Helper methods
    void update_sdl_text_input_area();
    SDL_TextInputType convert_input_type(EIMEInputType type) const;
    SDL_Capitalization convert_capitalization(EIMECapitalization cap) const;
    void reset_composition_state();
    void commit_current_text();
    SDL_Window* get_sdl_window(SystemWindow* window) const;
    void update_composition_state(const SDL_TextEditingEvent& event);
    void update_candidate_state(const SDL_TextEditingCandidatesEvent& event);
    
    // Platform specific helpers

private:
    SDL3Window* active_window_ = nullptr;
    IMEEventCallbacks callbacks_;
    
    // Current state
    mutable std::mutex state_mutex_;
    CompositionState composition_state_;
    CandidateState candidate_state_;
    IMETextInputArea input_area_ = {};
    
    // SDL Properties ID for advanced settings
    SDL_PropertiesID text_input_props_ = 0;
};

} // namespace skr