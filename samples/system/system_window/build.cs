using SB;
using SB.Core;

[TargetScript]
public static class SystemWindowSample
{
    static SystemWindowSample()
    {
        // 创建可执行程序目标
        Engine.Program("SystemWindowExample")
            // 依赖系统模块
            .Depend(Visibility.Private, "SkrSystem")
            // 依赖核心模块（日志、内存等）
            .Depend(Visibility.Private, "SkrCore")
            // 添加源文件
            .AddCppFiles("main.cpp");
    }
}