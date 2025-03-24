#include <V8Playground/debug.hpp>
#include <SkrCore/log.hpp>

namespace v8_play
{
void Debug::info(skr::StringView message)
{
    SKR_LOG_FMT_INFO(u8"{}", message);
}
void Debug::warn(skr::StringView message)
{
    SKR_LOG_FMT_WARN(u8"{}", message);
}
void Debug::error(skr::StringView message)
{
    SKR_LOG_FMT_ERROR(u8"{}", message);
}
}