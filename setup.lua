-- TODO. 构建系统优化
--------------------------------GOOD--------------------------------
-- TODO. 与 install 绑定的 SDK 结合 analyzer 与 target 相绑定, 是否进行 install 能通过依赖进行解析
--      utils.install_libraries 与上述逻辑进行联动，在 install 时会同时拷出
--      针对一些没有明确链接的目标，需要增加 install api 来加强导出
--      提供 phony target 来进行代理，执行某个特定的导出目标
--      utils.install_resources 也需要在 install 时联动拷出
--      现在使用 rule 的 before_build 可能会存在一些问题，考虑是否使用 target 进行 install 操作
-- TODO. xmake skr_setup plugin，用于 SDK 的下载与部署等操作
-- TODO. 与 install 无关的 SDK (比如 python/meta) 使用 phony target 进行代理, 是否进行 install 能通过依赖进行解析
-- TODO. project 拆离， 根目录脚本拆分为 xmake.lua/build_ext.lua，build_ext.lua 用于加载引擎提供的额外构建环境
--      保留 setup.lua，提供一组代码来导入引擎/拉取，是个范例/脚手架
--      project.lua 需要强化，主要目的是需要一个本地的配置覆盖文件， 比如 disable/enable 一些 target， 控制 vsc_dbg 生成结果等， 这个结果并不进入版本控制
-- TODO. 通过 skr_push_tag()/skr_pop_target() 来通过 analyzer 进行 target 的 enable/disable
-- TODO. 需要 build/.skr 文件夹来存放 skr engine 生成的文件
-- TODO. API skr_ 前缀，用于区分 skr engine 提供的 API
-- TODO. skr 构建提供的 build_ext
--   1. skr_analyze 系列 API
--   2. skr_dbg 系列 API
--   3. skr_install 系列 API
--   4. skr_pch 系列 API
--   5. skr_module 系列 API
--   6. skr_component 系列 API, 与 skr_module 配合使用
--   7. 用于 engine/project config 覆盖的 API
-- TODO. vsc_dbg 引入 natvis 拼接功能
--------------------------------BAD--------------------------------
-- 1. 全局 table skr_env，用于提供与 engine/project 相关的环境, 信息使用 skr_load_project() 加载，不存在则会导致配置失败
--      skr.engine.version
--      skr.engine.path
--      skr.engine.sdk_path
--      skr.project.path
--      skr.project.sdk_path
--------------------------------决定案--------------------------------
-- PART.1 引擎与项目的组织
--   由于对 xmake 的构建系统进行了大量的扩展，需要通过 engine 提供这些扩展，因此需要在根目录下提供 build_ext.lua 来加载这些扩展。
--   由于 xmake 对配置域的限制，如无必要，尽量避免使用全局变量传递信息，因此否决 [BAD.1]
--   目前抽离 project 只是相当于将模块向外抽离，最终依旧合并为一个项目编译，暂时无法达成类似 UE 的二进制分发效果，这是由于 xmake 本身没有进行类似的设计


module_root = path.absolute("xmake/modules")
import("skr", {rootdir = module_root})

import("core.project.config")
config.load()

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