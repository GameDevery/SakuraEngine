#include "AnimView/renderer.hpp"

skr::AnimViewRenderer* skr::AnimViewRenderer::Create()
{
    return SkrNew<skr::AnimViewRenderer>();
}