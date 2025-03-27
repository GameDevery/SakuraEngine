#include <V8Playground/debug.hpp>
#include <SkrCore/log.hpp>

namespace skr
{
void Debug::info(StringView message)
{
    SKR_LOG_FMT_INFO(u8"{}", message);
}
void Debug::warn(StringView message)
{
    SKR_LOG_FMT_WARN(u8"{}", message);
}
void Debug::error(StringView message)
{
    SKR_LOG_FMT_ERROR(u8"{}", message);
}
void Debug::exit(int32_t exit_code)
{
    ::exit(exit_code);
}
}