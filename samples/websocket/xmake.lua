codegen_component("WebSocketSample", {api = "WEB_SOCKET_SAMPLE", rootdir = "include/WebSocketSample"})
    add_files("include/**.hpp")

executable_module("WebSocketSample", "WEB_SOCKET_SAMPLE")
    set_kind("binary")
    add_includedirs("include", {public = true})
    add_deps("libwebsockets")
    public_dependency("SkrCore")
    add_files("src/**.cpp")