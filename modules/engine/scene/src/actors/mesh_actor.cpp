#include "SkrScene/actor.h"

namespace skr
{

MeshActor::~MeshActor() SKR_NOEXCEPT
{
    for (auto& child : children)
    {
        child->DetachFromParent();
    }
    if (_parent)
    {
        DetachFromParent();
    }
}

} // namespace skr