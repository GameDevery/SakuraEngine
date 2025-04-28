test_target("CGPUTests")
    add_rules("utils.dxc", {
        spv_outdir = "/../resources/shaders/cgpu-rspool-test",
        dxil_outdir = "/../resources/shaders/cgpu-rspool-test"})
    set_group("05.vid_tests/cgpu")
    public_dependency("SkrRT")
    skr_unity_build()
    add_files(
        "Common.cpp",
        -- Device Initialize
        "DeviceInitialize/**.cpp",
        -- Device Extensions
        "Exts/**.cpp",
        -- QueueOps
        "QueueOperations/QueueOperations.cpp",
        -- RSPool
        "RootSignaturePool/RootSignaturePool.cpp",
        "RootSignaturePool/shaders/**.hlsl",
        -- ResourceCreation
        "ResourceCreation/ResourceCreation.cpp",
        -- SwapChainCreation
        "SwapChainCreation/SwapChainCreation.cpp"
    , {unity_group = "CPUTests"})