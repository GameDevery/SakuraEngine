#pragma once

#define stage_inout clang::annotate("skr-shader", "stage_inout")
#define push_constant clang::annotate("skr-shader", "push_constant")
#define binding(binding, group) clang::annotate("skr-shader", "binding", (binding), (group))
#define position(x) clang::annotate("skr-shader", "position", (x))

#define compute_shader(x) clang::annotate("skr-shader", "stage", "compute", (x))
#define vertex_shader(x) clang::annotate("skr-shader", "stage", "vertex", (x))
#define fragment_shader(x) clang::annotate("skr-shader", "stage", "fragment", (x))