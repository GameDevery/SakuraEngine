static_component("CubismFramework", "SkrLive2D", { public = false })
    set_optimize("fastest")
    add_rules("c++.unity_build", {batchsize = default_unity_batch})
    add_deps("SkrBase")
    add_includedirs("CubismNativeCore/include", "CubismFramework", "CubismFramework/Framework", {public=false})
    add_files("CubismFramework/Renderer/**.cpp", "CubismFramework/Framework/**.cpp")
    
    -- add install
    skr_install_rule()
    skr_install("download", {
        name = "CubismNativeCore",
        install_func = "sdk",
    })
    
    -- link to cubism core
    if (is_os("windows")) then 
        if (is_mode("asan")) then
            set_runtimes("MD") -- csmiPlatformDependentLogPrint uses freopen
        end
        -- if (is_mode("release")) then
            add_links("Live2DCubismCore_MD", {public=true})
        -- else
        --     add_links("Live2DCubismCore_MDd", {public=true})
        -- end
    end
    if (is_os("macosx")) then 
        add_links("Live2DCubismCore", {public=true})
    end

private_pch("CubismFramework")
    add_files("CubismFramework/pch.hpp")

shared_module("SkrLive2D", "SKR_LIVE2D")
    add_rules("c++.unity_build", {batchsize = default_unity_batch})
    public_dependency("SkrImageCoder")
    public_dependency("SkrRenderer")
    add_includedirs("include", {public=true})
    add_includedirs("CubismNativeCore/include", "CubismFramework", "CubismFramework/Framework", {public=false})
    add_files("src/*.cpp")
    -- add live2d shaders
    add_rules("utils.dxc", {
        spv_outdir = "/../resources/shaders", 
        dxil_outdir = "/../resources/shaders"})
    add_files("shaders/*.hlsl")

private_pch("SkrLive2D")
    add_files("src/pch.hpp")