#pragma once

#include "SkrBase/math.h"
#include "SkrBase/config.h"
#include "SkrAnim/ozz/base/maths/simd_math.h"

namespace animd
{

ANIM_DEBUG_RUNTIME_API skr_float4x4_t ozz2skr_float4x4(ozz::math::Float4x4 ozz_mat);

} // namespace animd