-- TODO. config 探查机制，用于探查 engine.lua/project.lua 并装在
-- TODO. target disable 机制，并且能同时 disable 依赖它的 target，提供 tag 和 under path 两种方式
-- TODO. API skr_ 前缀，用于区分 skr engine 提供的 API

module_root = path.absolute("xmake/modules")
import("skr", {rootdir = module_root})

-- init download
local download = skr.download.new()
download:add_source_default()
download:fetch_manifests()

-- font
download:file("SourceSansPro-Regular.ttf")

-- python
if (os.host() == "windows") then
    download:tool("python-embed")
    local python = skr.utils.find_python()
    os.runv(python, {"-m", "pip", "install", "autopep8"})
else
    local python = skr.utils.find_python()
    os.runv(python, {"-m", "pip", "install", "mako"})
    os.runv(python, {"-m", "pip", "install", "autopep8"})
end

-- sdk and tools
if (os.host() == "windows") then
    -- gfx sdk
    download:sdk("WinPixEventRuntime")
    download:sdk("amdags")
    download:sdk("nvapi")
    download:sdk("nsight")
    download:sdk("dstorage-1.2.2")
    download:sdk("SDL2")
    -- dcc sdk
    download:sdk("sketchup-sdk-v2023.1.315")
    -- tools & compilers
    download:tool("dxc")
    download:tool("wasm-clang")
    download:tool("meta-v1.0.1-llvm_17.0.6")
    download:tool("tracy-gui-0.10.1a")
    -- network
    download:sdk("gns")
    download:sdk("gns", {debug = true})
end

if (os.host() == "macosx") then
    if (os.arch() == "x86_64") then
        --
        download:tool("dxc")
        download:tool("meta-v1.0.1-llvm_17.0.6")
        download:tool("tracy-gui-0.10.1a")
        -- dcc sdk
        download:sdk("sketchup-sdk-v2023.1.315")
        -- network
        download:sdk("gns")
        download:sdk("gns_d")
    else
        download:tool("dxc")
        download:tool("meta-v1.0.1-llvm_17.0.6")
        -- dcc sdk
        download:sdk("sketchup-sdk-v2023.1.315")
    end
end