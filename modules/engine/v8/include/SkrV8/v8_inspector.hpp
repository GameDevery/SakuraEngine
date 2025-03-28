#pragma once
#include "SkrBase/config.h"
#include "v8-inspector.h"

namespace skr
{
struct SKR_V8_API V8InspectorClient : v8_inspector::V8InspectorClient {
};
} // namespace skr