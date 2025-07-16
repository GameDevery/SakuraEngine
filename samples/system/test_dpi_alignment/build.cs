using SB;
using SB.Core;

[TargetScript]
public static class SystemTests
{
    static SystemTests()
    {
        // DPI Alignment Test
        Engine.Program("DPIAlignmentTest", "DPI_ALIGNMENT_TEST")
            .IncludeDirs(Visibility.Private, ".")
            .AddCppFiles("test_dpi_alignment.cpp")
            .Depend(Visibility.Private, "SkrSystem")
            .Depend(Visibility.Private, "SkrCore");
    }
}