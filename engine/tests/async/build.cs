using SB;
using SB.Core;

[TargetScript]
public static class AsyncTests
{
    static AsyncTests()
    {
        Test.UnitTest("ThreadsTest")
            .Depend(Visibility.Public, "SkrCore")
            .AddCppFiles("threads/threads.cpp");
            
        Test.UnitTest("ServiceThreadTest")
            .Depend(Visibility.Public, "SkrCore")
            .AddCppFiles("threads/service_thread.cpp");
            
        Test.UnitTest("JobTest")
            .Depend(Visibility.Public, "SkrCore")
            .AddCppFiles("threads/job.cpp");

        /* seems this little toy is buggy
        Test.UnitTest("Task2Test")
            .Depend(Visibility.Public, "SkrCore")
            .AddCppFiles("task2/**.cpp");
        */

        Test.UnitTest("MarlTest")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrTask")
            .AddCppFiles("marl-test/**.cpp");
    }
}