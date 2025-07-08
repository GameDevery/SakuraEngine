# VSCode Debug Configuration Generator

## 概述

SB 提供了 VSCode 调试配置生成器，可以自动为所有可执行目标生成 `launch.json` 和 `tasks.json` 文件。生成器支持多种调试器，并能保留用户自定义的配置。

## 使用方法

### 基本使用

```bash
# 生成默认调试配置（使用 cppdbg）
dotnet run --project tools/SB/SB.csproj -- vscode

# 指定调试器类型
dotnet run --project tools/SB/SB.csproj -- vscode --debugger lldb-dap

# 指定工作区根目录
dotnet run --project tools/SB/SB.csproj -- vscode --workspace /path/to/workspace

# 覆盖所有配置（不保留用户配置）
dotnet run --project tools/SB/SB.csproj -- vscode --preserve-user false
```

### 命令行选项

- `--debugger`: 调试器类型，支持:
  - `cppdbg`: Microsoft C++ 调试器（默认）
  - `lldb-dap`: LLDB DAP 适配器
  - `codelldb`: CodeLLDB 扩展
  - `cppvsdbg`: Visual Studio 调试器（仅 Windows）

- `--workspace`: 工作区根目录（默认为当前目录）

- `--preserve-user`: 是否保留用户创建的配置（默认为 true）

## 生成的文件

### launch.json

生成器会为每个可执行目标创建调试配置，包括：

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "[SB Generated] MyApp (Debug)",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/.build/clang/debug/bin/MyApp",
            "args": [],
            "cwd": "${workspaceFolder}",
            "environment": {
                "SKR_BUILD_TYPE": "Debug",
                "SKR_TARGET_NAME": "MyApp",
                "LD_LIBRARY_PATH": "${workspaceFolder}/.build/clang/debug/bin:${LD_LIBRARY_PATH}"
            },
            "preLaunchTask": "Build MyApp (Debug)",
            "MIMode": "gdb",
            "miDebuggerPath": "/usr/bin/gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ]
        }
    ]
}
```

### tasks.json

同时生成构建任务：

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build MyApp (Debug)",
            "type": "shell",
            "command": "dotnet",
            "args": [
                "run",
                "--project",
                "${workspaceFolder}/tools/SB/SB.csproj",
                "--",
                "build",
                "--target",
                "MyApp",
                "--mode",
                "debug"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": "$msCompile"
        }
    ]
}
```

## 功能特性

### 1. 多调试器支持

- **cppdbg**: 跨平台支持，自动检测 GDB/LLDB
- **lldb-dap**: 新的 LLDB DAP 协议支持
- **CodeLLDB**: 功能丰富的 LLDB 扩展
- **cppvsdbg**: Windows 原生调试器

### 2. 智能环境配置

- 自动设置动态库路径（PATH/LD_LIBRARY_PATH/DYLD_LIBRARY_PATH）
- 添加构建类型和目标名称环境变量
- 支持目标特定的环境变量配置

### 3. 保留用户配置

默认情况下，生成器会：
- 保留用户手动创建的调试配置
- 只更新 `[SB Generated]` 标记的配置
- 合并而非覆盖文件内容

### 4. 平台特定优化

- Windows: 设置控制台类型，配置调试器路径
- macOS: 配置源代码映射，LLDB 特定设置
- Linux: GDB pretty-printing，调试符号设置

## 配置目标

在 `build.cs` 中可以为目标添加调试配置：

```csharp
public static class MyApp
{
    static MyApp()
    {
        Engine.Module("MyApp", "MY_APP_API")
            .TargetType(TargetType.Executable)
            // 添加运行参数
            .RunArgs("--config", "debug.ini")
            // 添加环境变量
            .RunEnv("MY_APP_DEBUG", "1")
            .RunEnv("LOG_LEVEL", "debug");
    }
}
```

## 注意事项

1. **调试器安装**
   - cppdbg: 需要安装 C/C++ 扩展
   - lldb-dap: 需要安装 LLDB DAP 扩展
   - CodeLLDB: 需要安装 CodeLLDB 扩展

2. **路径处理**
   - 所有路径使用 VSCode 变量（如 `${workspaceFolder}`）
   - 支持跨平台路径分隔符

3. **性能考虑**
   - 生成器只处理可执行目标
   - 使用并发处理提高生成速度

## 故障排除

### 调试器找不到

检查调试器是否在 PATH 中，或手动编辑 `miDebuggerPath`。

### 动态库加载失败

确认生成的环境变量包含所有依赖库路径。

### 配置被覆盖

使用 `--preserve-user true` 保留用户配置。

## 扩展开发

VSCodeDebugEmitter 可以通过继承扩展：

```csharp
public class CustomVSCodeEmitter : VSCodeDebugEmitter
{
    protected override DebugConfiguration CreateDebugConfiguration(Target target, string buildType)
    {
        var config = base.CreateDebugConfiguration(target, buildType);
        
        // 添加自定义配置
        config.CustomProperties["myProperty"] = "value";
        
        return config;
    }
}
```