set_languages("c11", "cxx17")
add_rules("mode.debug", "mode.release", "mode.releasedbg", "mode.asan")

target("tinygltf")
    set_kind("static")
    add_files("tinygltf/**.cc")
    add_headerfiles("tinygltf/*.h", {prefixdir = "tinygltf"})
    add_headerfiles("tinygltf/extern/*.h", {prefixdir = "tinygltf/extern"})
    add_headerfiles("tinygltf/extern/*.hpp", {prefixdir = "tinygltf/extern"})