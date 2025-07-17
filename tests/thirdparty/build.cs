using SB;
using SB.Core;
using System.Runtime.CompilerServices;

[TargetScript]
public static class ThirdPartyTests
{
    static ThirdPartyTests()
    {
        Engine.UnitTest("ThirdPartyOzzTest")
            .Depend(Visibility.Public, "SkrAnim")
            .AddCppFiles("ozz/*.cpp"); 
    }
}