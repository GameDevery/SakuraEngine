#pragma once

#define stage_inout clang::annotate("skr-shader", "stage_inout")
#define push_constant clang::annotate("skr-shader", "push_constant")
#define binding(binding, group) clang::annotate("skr-shader", "binding", (binding), (group))

#define compute_shader(x) clang::annotate("skr-shader", "stage", "compute", (x))
#define vertex_shader(x) clang::annotate("skr-shader", "stage", "vertex", (x))
#define fragment_shader(x) clang::annotate("skr-shader", "stage", "fragment", (x))

#define position builtin("Position")
#define clip_distance builtin("ClipDistance")
#define cull_distance builtin("CullDistance")
#define render_target(n) builtin("RenderTarget" #n)
#define depth builtin("Depth")
#define depth_ge builtin("DepthGreaterEqual")
#define depth_le builtin("DepthLessEqual")
#define stencil builtin("StencilRef")
#define vertex_id builtin("VertexID")
#define instance_id builtin("InstanceID")

#define gs_instance_id builtin("GSInstanceID")
#define tess_factor builtin("TessFactor")
#define inside_tess_factor builtin("InsideTessFactor")
#define domain_location builtin("DomainLocation")
#define control_point_id builtin("ControlPointID")

#define primitive_id builtin("PrimitiveID")
#define is_front_face builtin("IsFrontFace")
#define sample_id builtin("SampleIndex")
#define sample_mask builtin("SampleMask")
#define barycentric_coord builtin("Barycentrics")

#define thread_id builtin("ThreadID")
#define group_id builtin("GroupID")
#define thread_position_in_group builtin("ThreadPositionInGroup")
#define thread_index_in_group builtin("ThreadIndexInGroup")
#define view_id builtin("ViewID")