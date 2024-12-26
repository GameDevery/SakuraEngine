function saved_config_dir()
    return "build/.skr/project.conf"
end

function binary_dir()
    return vformat("$(buildir)/$(os)/$(arch)/$(mode)")
end