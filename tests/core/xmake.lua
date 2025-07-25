test_target("JsonTest")
    set_group("05.tests/core")
    public_dependency("SkrCore")
    skr_unity_build()
    add_files("json/main.cpp")

test_target("SerdeTest")
    set_group("05.tests/core")
    public_dependency("SkrCore")
    skr_unity_build()
    add_files("serde/main.cpp")

test_target("NatvisTest")
    set_group("05.tests/core")
    public_dependency("SkrRT")
    skr_unity_build()
    add_files("natvis/*.cpp")

test_target("DelegateTest")
    set_group("05.tests/core")
    public_dependency("SkrCore")
    skr_unity_build()
    add_files("delegate/*.cpp")