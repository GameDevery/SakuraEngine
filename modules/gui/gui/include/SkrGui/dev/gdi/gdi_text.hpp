#pragma once
#include "SkrGui/fwd_config.hpp"

namespace skr::gui
{
struct SKR_GUI_API IGDIText {
    static bool      Initialize(IGDIRenderer* renderer);
    static bool      Finalize();
    static IGDIText* Get();
};
} // namespace skr::gui