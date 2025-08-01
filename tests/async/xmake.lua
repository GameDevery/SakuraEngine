test_target("ThreadsTest")
    set_group("05.tests/threads")
    public_dependency("SkrRT")
    add_files("threads/threads.cpp")

test_target("ServiceThreadTest")
    set_group("05.tests/threads")
    public_dependency("SkrRT")
    add_files("threads/service_thread.cpp")

test_target("JobTest")
    set_group("05.tests/task")
    public_dependency("SkrRT")
    add_files("threads/job.cpp")

test_target("Task2Test")
    set_group("05.tests/task")
    public_dependency("SkrRT")
    add_files("task2/**.cpp")

test_target("MarlTest")
    set_group("05.tests/marl")
    public_dependency("SkrRT")
    skr_unity_build()
    add_files("marl-test/**.cpp")
    add_rules("PickSharedPCH")