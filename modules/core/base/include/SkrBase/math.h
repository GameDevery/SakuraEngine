#pragma once

#ifdef __cplusplus
    #include "./math/gen/gen_math.hpp"

    #include "./math/manual/quat.hpp"
    #include "./math/manual/transform.hpp"
    #include "./math/manual/rotator.hpp"
    #include "./math/manual/matrix.hpp"
    #include "./math/manual/vector.hpp"
    #include "./math/manual/matrix_utils.hpp"
    #include "./math/manual/color_utils.hpp"
    #include "./math/manual/camera_utils.hpp"
    #include "./math/manual/coordinate_system.hpp"

    #include "./math/gen/gen_math_c_decl.hpp"
    #include "./math/gen/gen_math_traits.hpp"

#else
    #include "./math/gen//gen_math_c_decl.h"
#endif