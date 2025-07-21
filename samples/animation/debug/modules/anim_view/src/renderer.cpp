#include "AnimView/renderer.hpp"

skr::AnimViewRenderer* skr::AnimViewRenderer::Create()
{
    return SkrNew<skr::AnimViewRenderer>();
}

void skr::AnimViewRenderer::Destory(AnimViewRenderer* renderer)
{
    SkrDelete(renderer);
}

skr::AnimViewRenderer::~AnimViewRenderer()
{
}