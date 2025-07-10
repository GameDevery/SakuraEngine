#include "cocoa_window_manager.h"
#include "SkrCore/memory/memory.h"

namespace skr {

ISystemWindowManager* CreateCocoaWindowManager()
{
    return SkrNew<CocoaWindowManager>();
}

} // namespace skr