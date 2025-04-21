target("OpenSSLSample")
    set_kind("binary")
    add_deps("openssl", {public = true})
    add_files("src/**.cpp")