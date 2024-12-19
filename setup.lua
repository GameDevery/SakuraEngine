-- TODO. 与 install 绑定的 SDK 结合 analyzer 与 target 相绑定, 是否进行 install 能通过依赖进行解析
--      utils.install_libraries 与上述逻辑进行联动，在 install 时会同时拷出
--      针对一些没有明确链接的目标，需要增加 install api 来加强导出
--      提供 phony target 来进行代理，执行某个特定的导出目标
--      utils.install_resources 也需要在 install 时联动拷出
--      现在使用 rule 的 before_build 可能会存在一些问题，考虑是否使用 target 进行 install 操作
-- TODO. xmake skr_setup plugin，用于 SDK 的下载与部署等操作
-- TODO. 与 install 无关的 SDK (比如 python/meta) 使用 phony target 进行代理, 是否进行 install 能通过依赖进行解析
-- TODO. 全局 table skr_env，用于提供与 engine/project 相关的环境
--      skr_env.engine_version
--      skr_env.engine_path
--      skr_env.engine_sdk_path
--      skr_env.project_path
--      skr_env.project_sdk_path
-- TODO. 通过 skr_push_tag()/skr_pop_target() 来通过 analyzer 进行 target 的 enable/disable
-- TODO. 需要 build/.skr 文件夹来存放 skr engine 生成的文件
-- TODO. project 拆离
--      保留 setup.lua，提供一组代码来导入引擎/拉取，是个范例/脚手架
--      project.lua 需要强化，主要目的是需要一个本地的配置覆盖文件， 比如 disable/enable 一些 target， 控制 vsc_dbg 生成结果等， 这个结果并不进入版本控制
-- TODO. API skr_ 前缀，用于区分 skr engine 提供的 API
-- TODO. vsc_dbg 自动对 pre_cmd/post_cmd 插入返回值检查，custom debug target 的出参同时也要改为 str list

module_root = path.absolute("xmake/modules")
import("find_sdk", {rootdir = module_root})

-- python
find_sdk.file_from_github("SourceSansPro-Regular.ttf")
if (os.host() == "windows") then
    find_sdk.tool_from_github("python-embed", "python-embed-windows-x64.zip")
    python = find_sdk.find_embed_python()
    os.runv(python.program, {"-m", "pip", "install", "autopep8"})
else
    pip = find_sdk.find_program("pip3") or find_sdk.find_program("pip") or {program = "pip"}
    os.runv(pip.program, {"install", "mako"})
    os.runv(pip.program, {"install", "autopep8"})
end

if (os.host() == "windows") then
    -- gfx sdk
    find_sdk.lib_from_github("WinPixEventRuntime", "WinPixEventRuntime-windows-x64.zip")
    find_sdk.lib_from_github("amdags", "amdags-windows-x64.zip")
    find_sdk.lib_from_github("nvapi", "nvapi-windows-x64.zip")
    find_sdk.lib_from_github("nsight", "nsight-windows-x64.zip")
    find_sdk.lib_from_github("dstorage-1.2.2", "dstorage-1.2.2-windows-x64.zip")
    find_sdk.lib_from_github("SDL2", "SDL2-windows-x64.zip")
    -- dcc sdk
    find_sdk.lib_from_github("sketchup-sdk-v2023.1.315", "sketchup-sdk-v2023.1.315-windows-x64.zip")
    -- tools & compilers
    find_sdk.tool_from_github("dxc", "dxc-windows-x64.zip")
    find_sdk.tool_from_github("wasm-clang", "wasm-clang-windows-x64.zip")
    find_sdk.tool_from_github("meta-v1.0.1-llvm_17.0.6", "meta-v1.0.1-llvm_17.0.6-windows-x64.zip")
    find_sdk.tool_from_github("tracy-gui-0.10.1a", "tracy-gui-0.10.1a-windows-x64.zip")
    -- network
    find_sdk.lib_from_github("gns", "gns-windows-x64.zip")
    find_sdk.lib_from_github("gns_d", "gns_d-windows-x64.zip")
end

if (os.host() == "macosx") then
    if (os.arch() == "x86_64") then
        --
        find_sdk.tool_from_github("dxc", "dxc-macosx-x86_64.zip")
        find_sdk.tool_from_github("meta-v1.0.1-llvm_17.0.6", "meta-v1.0.1-llvm_17.0.6-macosx-x86_64.zip")
        find_sdk.tool_from_github("tracy-gui-0.10.1a", "tracy-gui-0.10.1a-macosx-x86_64.zip")
        -- dcc sdk
        find_sdk.lib_from_github("sketchup-sdk-v2023.1.315", "sketchup-sdk-v2023.1.315-macosx-x86_64.zip")
        -- network
        find_sdk.lib_from_github("gns", "gns-macosx-x86_64.zip")
        find_sdk.lib_from_github("gns_d", "gns_d-macosx-x86_64.zip")
    else
        find_sdk.tool_from_github("dxc", "dxc-macosx-arm64.zip")
        find_sdk.tool_from_github("meta-v1.0.1-llvm_17.0.6", "meta-v1.0.1-llvm_17.0.6-macosx-arm64.zip")
        -- dcc sdk
        find_sdk.lib_from_github("sketchup-sdk-v2023.1.315", "sketchup-sdk-v2023.1.315-macosx-arm64.zip")
    end
end

local setups = os.files("**/setup.lua")
for _, setup in ipairs(setups) do
    local dir = path.directory(setup)
    local basename = path.basename(setup)
    import(path.join(dir, basename))
end

--[[
if (os.host() == "windows") then
    os.exec("xmake project -k vsxmake -m \"debug,release\" -a x64 -y")
end
]]--
