import("net.http")
import("core.base.json")
import("core.base.object")
import("skr.utils")

------------------------concepts------------------------
-- source: {
--   name: [string] source name
--   url: [string] base url
--   download_dir: [string] download dir
--   manifest_path: [string] manifest file path
--   files: [table] file_name->sha256, load from manifest_path
--   [error]: error message if load manifest failed
-- }
--
-- download_mgr: {
--   sources: [source],
--   files: [table], key->file_name, value->{sha:[sha256], sources_info: list of{source, sha}}
-- }

local download = download or object {}

function download:new()
    return download { sources = {}, files = {} }
end

------------------------sources------------------------
function download:add_source(name, url)
    local download_dir = utils.download_dir()
    local normalized_name = name:gsub(" ", "_")
    table.insert(self.sources, {
        name = name,
        url = url,
        download_dir = download_dir,
        manifest_path = path.join(download_dir, "manifests", normalized_name..".json"),
        files = {},
        error = nil,
        is_success = function (self)
            return self.error == nil
        end,
        is_failed = function (self)
            return self.error ~= nil
        end
    })
end
function download:add_source_default()
    self:add_source("skr_github", "https://github.com/SakuraEngine/Sakura.Resources/releases/download/SDKs/")
end

------------------------manifest------------------------
function download:fetch_manifests()
    -- download
    for _, source in ipairs(self.sources) do
        local url = path.join(source.base_url, "manifest.json")

        try {
            function ()
                print("[fetch manifest]: %s to %s", url, source.manifest_path)
                http.download(url, source.manifest_path, { continue = false })
                print("${green}success${clear}")
            end,
        catch {
            function (error)
                print("${red}failed${clear}, reason: %s", error)
            end
        }}
    end

    -- load files
    self:load_manifests()
end

function download:load_manifests()
    -- load files
    for _, source in ipairs(self.sources) do
        if os.exists(source.manifest_path) then
            source.files = json.loadfile(source.manifest_path)
        else
            -- if call from fetch_manifests, error may be set
            if not source.error then
                print("manifest file not found, when load [%s]", source.name)
                source.error = "manifest file not found"
            end
        end
    end

    -- solve global file list
    for _, source in ipairs(self.sources) do
        if source:is_success() then
            for name, sha in pairs(source.files) do
                -- add file first
                if not self.files[name] then
                    self.files[name] = { sha = nil, sources = {}, _shas = {} }
                end

                -- add source
                local info = self.files[name]
                table.insert(info.sources_info, {
                    source = source,
                    sha = sha,
                })
            end
        end
    end

    -- check sha conflict
    for name, file_info in pairs(self.files) do
        local reference_sha = nil
        for _, source_info in ipairs(file_info.sources_info) do
            if reference_sha then
                if reference_sha ~= source_info.sha then
                    local err_msg = format("sha conflict when solve file \"%s\":", file_name)
                    for _, err_source_info in ipairs(file_info.sources_info) do
                        err_msg = err_msg .. format("\n  %s: %s", err_source_info.source.name, err_source_info.sha)
                    end
                    raise(err_msg)
                end
            else
                reference_sha = source_info.sha
            end
        end
    end
end

------------------------download------------------------
function download:_download_from_source(file_name, source)
    -- get sha
    local sha = source.files[file_name]
    if not sha then
        raise("file not found in source: %s", file_name)
    end

    -- get download params
    local download_dir = utils.download_dir()
    local download_path = path.join(download_dir, file_name)
    local download_url = path.join(source.url, file_name)

    -- download file
    print("download: %s to %s", download_url, download_path)
    http.download(download_url, download_path, { continue = false })

    -- check sha
    local actual_sha = hash.sha256(download_path)
    if actual_sha ~= sha then
        raise("sha miss match, failed to download %s from github!\nexpect: %s\nactual: %s",
            file_name,
            sha,
            actual_sha)
    end
end
function download:download_file(file_name, opt)
    -- load opt
    opt = opt or {}
    local force = opt.force or false

    -- get file info
    local file_info = self.files[file_name]
    if not file_info then
        raise("file info not found: %s", file_name)
    end
    local sha = file_info.sha
    local download_reason = "not found"

    -- find exists and check sha
    if not force then
        local found_file = utils.find_download_file(file_name)
        if found_file then
            local exists_sha = hash.sha256(found_file)
            if exists_sha == sha then
                download_reason = "sha conflict"
                return found_file
            end
        end
    end

    -- try download from sources
    local success = false
    print("[%s] download %s", download_reason, file_name)
    for _, source_info in ipairs(file_info.sources_info) do
        try {
            function ()
                print("trying: %s", source_info.source.name)
                self:_download_from_source(file_name, source_info.source)
                success = true
            end,
        catch {
            function (error)
                print("${red}failed${clear}, reason: %s", error)
            end
        }}
    end

    -- raise
    if not success then
        raise("failed to download %s", file_name)
    end
end

------------------------[temp] install------------------------
function download:install_tool(tool_name)
    -- find package file
    local package_path = utils.find_download_package_tool(tool_name)
    if not package_path then
        raise("failed to find tool \"%s\" when install", tool_name)
    end
    
    -- extract package
    archive.extract(package_path, utils.install_dir_tool())
end
function download:tool(tool_name)
    self:download_file(utils.package_name_tool(tool_name))
    self:install_tool(tool_name)
end
function download:sdk(sdk_name, opt)
    self:download_file(utils.package_name_sdk(sdk_name, opt))
end

return download