#pragma once
#include "rtm/camera_utilsf.h"
#include "SkrBase/types.h"
#include "SkrBase/math.h"

#include "SkrContainersDef/vector.hpp"

namespace animd
{

struct BoneGeometry {
    struct InstanceData {
        skr_float4x4_t world[2];
    };
    static InstanceData instance_data;

    inline static uint32_t _vector_to_snorm8(const skr::float4& v)
    {
        return skr::pack_snorm8(skr::normalize(v));
    }
    // right hand inside geometry
    // 一个八面体，上下两个四棱锥组合而成，y up, x right, z forward
    const skr_float3_t g_Positions[24] = {
        { 1.0, 0.0, 0.0 }, // xyz face
        { 0.0, 4.0, 0.0 },
        { 0.0, 0.0, 1.0 },

        { 1.0, 0.0, 0.0 }, // xz-y face
        { 0.0, 0.0, 1.0 },
        { 0.0, -1.0, 0.0 },

        { 0.0, 0.0, 1.0 }, // zy-x face
        { 0.0, 4.0, 0.0 },
        { 0.0, 0.0, -1.0 },

        { 0.0, 0.0, 1.0 }, // z-x-y face
        { -1.0, 0.0, 0.0 },
        { 0.0, -1.0, 0.0 },

        { -1.0, 0.0, 0.0 }, // -xy-z face
        { 0.0, 4.0, 0.0 },
        { 0.0, 0.0, -1.0 },

        { -1.0, 0.0, 0.0 }, // -xz-y face
        { 0.0, 0.0, -1.0 },
        { 0.0, -1.0, 0.0 },

        { 0.0, 0.0, -1.0 }, // -zyx face
        { 0.0, 4.0, 0.0 },
        { 1.0, 0.0, 0.0 },

        { 0.0, 0.0, -1.0 }, // -zx-y face
        { 1.0, 0.0, 0.0 },
        { 0.0, -1.0, 0.0 },
    };

    const skr_float2_t g_TexCoords[24] = {
        { 0.0f, 0.0f }, // xyz face
        { 1.0f, 1.0f },
        { 0.0f, 1.0f },

        { 0.0f, 1.0f }, // xz-y face
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },

        { 0.0f, 1.0f }, // zy-x face
        { 1.0f, 1.0f },
        { 1.0f, 0.0f },

        { 0.0f, 1.0f }, // z-x-y face
        { 1.0f, 1.0f },
        { 1.0f, 0.0f },

        { 1.0f, 1.0f }, // -xy-z face
        { 1.0f, 1.0f },
        { 1.0f, 0.0f },

        { 1.0f, 1.0f }, // -xz-y face
        { 1.0f, 1.0f },
        { 1.0f, 0.0f },

        { 1.0f, 1.0f }, // -zyx face
        { 1.0f, 1.0f },
        { 1.0f, 0.0f },

        { 1.0f, 1.0f }, // -zx-y face
        { 1.0f, 1.0f },
        { 1.0f, 0.0f },
    };

    const uint32_t g_Normals[24] = {
        _vector_to_snorm8({ 1.0f, 0.25f, 1.0f, 0.0f }),
        _vector_to_snorm8({ 1.0f, 0.25f, 1.0f, 0.0f }),
        _vector_to_snorm8({ 1.0f, 0.25f, 1.0f, 0.0f }),

        _vector_to_snorm8({ 1.0f, -0.25f, 1.0f, 0.0f }),
        _vector_to_snorm8({ 1.0f, -0.25f, 1.0f, 0.0f }),
        _vector_to_snorm8({ 1.0f, -0.25f, 1.0f, 0.0f }),

        _vector_to_snorm8({ -1.0f, 0.25f, 1.0f, 0.0f }),
        _vector_to_snorm8({ -1.0f, 0.25f, 1.0f, 0.0f }),
        _vector_to_snorm8({ -1.0f, 0.25f, 1.0f, 0.0f }),

        _vector_to_snorm8({ -1.0f, -0.25f, 1.0f, 0.0f }),
        _vector_to_snorm8({ -1.0f, -0.25f, 1.0f, 0.0f }),
        _vector_to_snorm8({ -1.0f, -0.25f, 1.0f, 0.0f }),

        _vector_to_snorm8({ -1.0f, 0.25f, -1.0f, 0.0f }),
        _vector_to_snorm8({ -1.0f, 0.25f, -1.0f, 0.0f }),
        _vector_to_snorm8({ -1.0f, 0.25f, -1.0f, 0.0f }),

        _vector_to_snorm8({ -1.0f, -0.25f, -1.0f, 0.0f }),
        _vector_to_snorm8({ -1.0f, -0.25f, -1.0f, 0.0f }),
        _vector_to_snorm8({ -1.0f, -0.25f, -1.0f, 0.0f }),

        _vector_to_snorm8({ 1.0f, 0.25f, -1.0f, 0.0f }),
        _vector_to_snorm8({ 1.0f, 0.25f, -1.0f, 0.0f }),
        _vector_to_snorm8({ 1.0f, 0.25f, -1.0f, 0.0f }),

        _vector_to_snorm8({ 1.0f, -0.25f, -1.0f, 0.0f }),
        _vector_to_snorm8({ 1.0f, -0.25f, -1.0f, 0.0f }),
        _vector_to_snorm8({ 1.0f, -0.25f, -1.0f, 0.0f }),

    };

    const uint32_t g_Tangents[24] = {
        _vector_to_snorm8({ 0.0f, 1.0f, 0.0f, 0.0f }), // xyz face
        _vector_to_snorm8({ 0.0f, 1.0f, 0.0f, 0.0f }),
        _vector_to_snorm8({ 0.0f, 1.0f, 0.0f, 0.0f }),

        _vector_to_snorm8({ 1.0f, 0.0f, 0.0f, 0.0f }), // xz-y face
        _vector_to_snorm8({ 1.0f, 0.0f, 0.0f, 0.0f }),
        _vector_to_snorm8({ 1.0f, 0.0f, 0.0f, 0.0f }),

        _vector_to_snorm8({ -1.0f, 1.0f, -1.0f, 0.0f }), // zy-x face
        _vector_to_snorm8({ -1.0f, -1.0f, -1.0f, 0.0f }),
        _vector_to_snorm8({ -1.0f, -1.0f, -1.0f, 0.0f }),

        _vector_to_snorm8({ -1.5f, -1.f, -1.f, 2.f }), // z-x-y face
        _vector_to_snorm8({ -1.f, -1.f, -1.f, 2.f }),
        _vector_to_snorm8({ -1.f, -1.f, -1.f, 2.f }),

        _vector_to_snorm8({ -2.f, -2.f, -2.f, 3.f }), // -xy-z face
        _vector_to_snorm8({ -2.f, -2.f, -2.f, 3.f }),
        _vector_to_snorm8({ -2.f, -2.f, -2.f, 3.f }),

        _vector_to_snorm8({ -3.f, -3.f, -3.f, 4.f }), //- xz-y face
        _vector_to_snorm8({ -3.f, -3.f, -3.f, 4.f }),
        _vector_to_snorm8({ -3.f, -3.f, -3.f, 4.f })
    };

    // clang-format off
    static constexpr uint32_t g_Indices[] = {
        0, 1, 2,
        3,4 ,5,
        6,7 ,8,
        9,10,11,
        12,13,14,
        15,16,17,
        18,19,20,
        21,22,23
    };
};



} // namespace animd