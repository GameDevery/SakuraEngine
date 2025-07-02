#pragma once

#define stage_inout clang::annotate("skr-shader", "stage_inout")
#define group(x) clang::annotate("skr-shader", "group", (x))
#define binding(x) clang::annotate("skr-shader", "binding", (x))
#define position(x) clang::annotate("skr-shader", "position", (x))

#define compute_shader(x) clang::annotate("skr-shader", "stage", "compute", (x))
#define vertex_shader(x) clang::annotate("skr-shader", "stage", "vertex", (x))
#define fragment_shader(x) clang::annotate("skr-shader", "stage", "fragment", (x))