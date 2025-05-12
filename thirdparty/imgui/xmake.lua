--! use version with dynamic fonts, maybe unstable
--! Branch: features/dynamic_fonts
--! Commit: cde94b1 - WIP - Fonts: fixed broken support for legacy backend due to a mismatch with initial pre-build baked id.
--! CommitSHA: cde94b14f3d26506d324fc2826bd92b463dd4248
target("ImGui")
    set_kind("static")
    add_includedirs("include", {public = true})
    add_includedirs("src", {private = true})
    add_files("src/**.cpp")

    add_defines("IMGUI_DEFINE_MATH_OPERATORS")