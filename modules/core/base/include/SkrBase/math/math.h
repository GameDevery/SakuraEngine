#pragma once

#ifdef __cplusplus
#include "./gen/gen_math.hpp"
#include "./gen/gen_math_c_decl.hpp"

#include "./manual/quat.hpp"
#include "./manual/transform.hpp"
#include "./manual/rotator.hpp"
#include "./manual/matrix.hpp"
#else
#include "./gen//gen_math_c_decl.h"
#endif