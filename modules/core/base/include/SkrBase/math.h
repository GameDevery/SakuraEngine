#pragma once

#ifdef __cplusplus
    #include "./math/gen/gen_math.hpp"
    #include "./math/gen/gen_math_c_decl.hpp"

    #include "./math/manual/quat.hpp"
    #include "./math/manual/transform.hpp"
    #include "./math/manual/rotator.hpp"
    #include "./math/manual/matrix.hpp"
#else
    #include "./math/gen//gen_math_c_decl.h"
#endif