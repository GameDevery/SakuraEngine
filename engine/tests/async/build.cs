using SB;
using SB.Core;

[TargetScript]
public static class AsyncTests
{
    static AsyncTests()
    {
        Test.UnitTest("ThreadsTest")
            .Depend(Visibility.Public, "SkrRT")
            .AddCppFiles("threads/threads.cpp");
            
        Test.UnitTest("ServiceThreadTest")
            .Depend(Visibility.Public, "SkrRT")
            .AddCppFiles("threads/service_thread.cpp");
            
        Test.UnitTest("JobTest")
            .Depend(Visibility.Public, "SkrRT")
            .AddCppFiles("threads/job.cpp");

        /* seems this little toy is buggy
        Test.UnitTest("Task2Test")
            .Depend(Visibility.Public, "SkrRT")
            .AddCppFiles("task2/**.cpp");
        */

        Test.UnitTest("MarlTest")
            .EnableUnityBuild()
            .Depend(Visibility.Public, "SkrRT")
            .AddCppFiles("marl-test/**.cpp");
    }
}