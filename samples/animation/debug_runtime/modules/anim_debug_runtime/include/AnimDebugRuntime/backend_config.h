#pragma once
#include "SkrBase/config.h"
#include "SkrRT/resource/config_resource.h"
#include "SkrContainers/function_ref.hpp"

#ifndef __meta__
    #include "AnimDebugRuntime/backend_config.generated.h" // IWYU pragma: export
#endif

sreflect_enum(
    guid = "0197bfcf-cac6-7597-81c1-8f2263b0b925"
    serde = @bin|@json
)
    ECGPUBackEnd
    SKR_ENUM(uint32_t)
{
    Vulkan,
    DX12,
    Metal
};
typedef enum ECGPUBackEnd ECGPUBackEnd;
