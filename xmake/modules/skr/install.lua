import("net.http")
import("core.base.json")
import("lib.detect")
import("utils.archive")
import("skr.utils")

-- TODO. 下载的 manifest 放在对应的 source 文件夹下，下载完毕立刻存储既存的 manifest 列表
-- TODO. fetch manifest 完毕立刻构建 source 数据库，将 source_dir, source_url 等信息直接存储，并对文件构造 source 来源表
-- TODO. 明确的语义 (name, opt = {version, debug}), 用于规范化的下载和安装
-- TODO. detect 的 cache 不是非常靠谱，考虑清理 cache 或者采用更稳定的实现

------------------------concepts------------------------
-- sources structure:
--   name: [string] source name
--   base_url: [string] source url
--     fetch manifest use "${base_url}/manifest.json"
--     download file use "${base_url}/${file_name}"

-- manifest structure:
--   source: [source] see above
--   url: [string] source url
--   path: [string] manifest file path
--   files: [table] file_name->sha25 table
--   [error]: error message when fetch manifest failed

-- TODO. move find and dir api to util file

------------------------dirs------------------------
function download_dir()
    return "build/.skr/download"
end

function install_dir_tool()
    return vformat("build/.skr/tools/$(host)")
end

------------------------package name rule------------------------
function package_name_tool(tool_name)
    return vformat("%s-$(host)-%s.zip", tool_name, os.arch())
end
function package_name_sdk(sdk_name, opt)
    opt = opt or {}

    if opt.debug then
        return vformat("%s_d-$(os)-%s.zip", sdk_name, os.arch())
    else
        return vformat("%s-$(os)-%s.zip", sdk_name, os.arch())
    end
    
end

------------------------find------------------------
function find_download_file(file_name)
    local file = detect.find_file(path.join("/**/", file_name), { download_dir() })
    return file
end

function find_download_tool(tool_name)
    return find_download_file(package_name_tool(tool_name))
end

function find_download_sdk(sdk_name)
    local result
    
    if is_mode("asan") or is_mode("debug") or is_mode("releasedbg") then
        result = find_download_file(package_name_sdk(sdk_name, { debug = true }))
    end
    
    if not result then
        local sdk_package_name = vformat("%s-$(os)-%s.zip", sdk_name, os.arch())
        result = find_download_file(package_name_sdk(sdk_name, { debug = false }))
    end

    return result
end

------------------------source------------------------
-- get default source of
function default_source()
    return {
        name = "skr_github",
        base_url = "https://github.com/SakuraEngine/Sakura.Resources/releases/download/SDKs/"
    }
end

------------------------download------------------------
-- fetch manifests from sources
-- @sources: source urls of sdk repo
-- @return: list of manifests info, see top of this file
function fetch_manifests(sources)
    -- TODO. remove here, must passed by caller
    sources = sources or { default_source() }
    local download_dir = download_dir()

    local manifests = {}
    for _, source in ipairs(sources) do
        local url = path.join(source.base_url, "manifest.json")
        local path = path.join(download_dir, source.name:gsub(" ", "_") .. "_manifest.json")
        try {
            function()
                print("[fetch manifest]: %s to %s", url, path)
                http.download(url, path, { continue = false })
                table.insert(manifests, {
                    source = source,
                    url = url,
                    path = path,
                    files = json.loadfile(path),
                })
                print("${green}success${clear}")
            end,
            catch {
                function(error)
                    print("${red}failed, reason: %s${clear}", error)
                    table.insert(manifests, {
                        source = source,
                        url = url,
                        path = path,
                        error = error,
                    })
                end
            }
        }
    end

    return manifests
end

-- solve candidates sources of file
-- @file_name: file name wants to download
-- @manifests: list of manifests, download by fetch_manifests
-- @return: list of candidate manifests, see top of this file
function solve_sources(file_name, manifests)
    -- filter manifests that contains file_name
    local candidates = {}
    for _, manifest in ipairs(manifests) do
        if manifest.files then
            local sha = manifest.files[file_name]
            if sha then
                table.insert(candidates, {
                    manifest = manifest,
                    sha = sha,
                })
            end
        end
    end

    -- check sha conflict
    do
        local reference_sha = nil
        local conflict = false
        for _, candidate in ipairs(candidates) do
            reference_sha = reference_sha or candidate.sha
            conflict = conflict or candidate.sha ~= reference_sha
        end

        if conflict then
            local err_msg = format("sha conflict when solve file \"%s\":", file_name)
            for _, candidate in ipairs(candidates) do
                err_msg = err_msg .. format("\n  %s: %s", candidate.manifest.source.name, candidate.sha)
            end
            raise(err_msg)
        end
    end

    -- return candidates
    if #candidates > 0 then
        local result = {}
        for _, candidate in ipairs(candidates) do
            table.insert(result, candidate.manifest)
        end
        return result
    end
end

-- download file from manifest
-- @file_name: file name wants to download
-- @manifest: manifest of source, download by fetch_manifests
-- @return: downloaded file path
function download_file(file_name, manifest)
    -- get sha
    local sha = manifest.files[file_name]
    if not sha then
        raise("failed to find file \"%s\" in manifest \"%s\"", file_name, manifest.source.name)
    end

    -- get download params
    local download_dir = path.join(download_dir(), manifest.source.name:gsub(" ", "_"))
    local download_path = path.join(download_dir, file_name)
    local download_url = path.join(manifest.source.base_url, file_name)

    -- download file
    print("download: %s to %s", download_url, download_path)
    http.download(download_url, download_path, { continue = false })

    -- check sha
    local actual_sha = hash.sha256(download_path)
    if hash.sha256(download_path) ~= sha then
        raise("sha miss match, failed to download %s from github!\nexpect: %s\nactual: %s",
            file_name,
            sha,
            actual_sha)
    end

    return download_path
end

-- download file with manifest try
-- @file_name: file name wants to download
-- @manifests: list of manifest, download by fetch_manifests
-- @return: [1] downloaded file path, [2] per manifest download try result, object structure:
--   manifest: [manifest] input from @manifests, see top of this file
--   [error]: error message if download failed
--   [path]: downloaded file path if download success
function download_or_update_file(file_name, manifests)
    -- solve candidate manifests
    local candidate_manifests = solve_sources(file_name, manifests)
    if not candidate_manifests then
        raise("failed to find file \"%s\" in any manifest", file_name)
    end

    -- paths
    local download_dir = download_dir()
    
    -- search exist files
    for _, manifest in ipairs(candidate_manifests) do
        local file_path = path.join(download_dir, manifest.source.name:gsub(" ", "_"), file_name)
        if os.exists(file_path) then
            -- if found file, return it
            local sha = hash.sha256(file_path)
            if sha == manifest.files[file_name] then
                return file_path, nil
            end
        end
    end
    
    -- try download file from manifest
    print("download %s", file_name)
    local try_results = {}
    local success_path = false
    for _, manifests in ipairs(candidate_manifests) do
        local file_path = path.join(download_dir, manifest.source.name:gsub(" ", "_"), file_name)
        local file_url = path.join(manifests.source.base_url, file_name)
        
        -- try download
        try {
            function ()
                print("trying: %s", file_url)
                http.download(file_url, file_path, { continue = false })
                success_path = file_path
                table.insert(try_results, {
                    manifest = manifest,
                    path = file_path,
                })
                print("${green}done${clear}")
            end,
            catch {
                function (error)
                    table.insert(try_results, {
                        manifest = manifest,
                        error = error,
                    })
                    print("${red}failed, reason: %s${clear}", error)
                end
            }
        }

        -- if success, return
        if success_path then
            return success_path, try_results
        end
    end

    -- return error
    return nil, try_results
end

------------------------install------------------------
-- install tool from zip file
function install_tool(tool_name)
    -- find package file
    local package_file = find_download_tool(tool_name)
    if not package_file then
        raise("failed to find tool \"%s\" when install", tool_name)
    end

    -- extract package
    archive.extract(package_file, install_dir_tool())
end

------------------------simple api------------------------
function tool(tool_name, manifests)
    -- download or update tool
    local package_name = package_name_tool(tool_name)
    local package_file = download_or_update_file(package_name, manifests)
    if not package_file then
        raise("failed to download or update tool \"%s\"", tool_name)
    end

    -- install tool
    install_tool(tool_name)
end
function sdk(sdk_name, opt)
    -- download or update sdk
    local package_name = package_name_sdk(sdk_name, opt)
    local package_file = download_or_update_file(package_name, manifests)
    if not package_file then
        raise("failed to download or update sdk \"%s\"", sdk_name)
    end
end