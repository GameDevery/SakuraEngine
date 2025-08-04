#include "AnimDebugRuntime/util.h"
namespace animd
{
skr_float4x4_t ozz2skr_float4x4(ozz::math::Float4x4 ozz_mat)
{
    float model_data[16];
    ozz::math::StorePtrU(ozz_mat.cols[0], model_data);
    ozz::math::StorePtrU(ozz_mat.cols[1], model_data + 4);
    ozz::math::StorePtrU(ozz_mat.cols[2], model_data + 8);
    ozz::math::StorePtrU(ozz_mat.cols[3], model_data + 12);
    // skr_float4x4 is row-major, so we need to rearrange the data
    skr_float4x4_t skr_mat{
        model_data[0], model_data[4], model_data[8], model_data[12],
        model_data[1], model_data[5], model_data[9], model_data[13],
        model_data[2], model_data[6], model_data[10], model_data[14],
        model_data[3], model_data[7], model_data[11], model_data[15]
    };

    return skr_mat;
}
} // namespace animd