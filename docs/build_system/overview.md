# SakuraEngine 构建系统 (SB) 概述

## 简介

SakuraEngine 使用自研的构建系统 **SB** (Sakura Build)，这是一个基于 C# 开发的高性能构建系统，专门为大型游戏引擎项目设计。

## 核心特性

### 目标导向架构
- **Target**: 构建的基本单元，代表一个可编译的模块或应用
- **TargetScript**: 使用特性标记的 C# 类，用于定义构建目标
- **TargetCategory**: 目标分类系统，支持模块、工具、示例等多种类型

### 任务发射器 (TaskEmitter)
构建系统的核心执行单元，负责特定类型的构建任务：

- **CppCompileEmitter**: C++ 编译任务
- **CppLinkEmitter**: 链接任务
- **CodegenMetaEmitter**: 代码生成元数据
- **CodegenRenderEmitter**: 代码生成渲染
- **VSEmitter**: Visual Studio 工程生成
- **VSCodeDebugEmitter**: VSCode 调试配置生成
- **ISPCEmitter**: ISPC 编译任务
- **UnityBuildEmitter**: Unity Build 优化

### 依赖管理
- **EntityFramework**: 使用 EF Core 管理构建依赖关系
- **DependDatabase**: 持久化依赖数据库，支持增量构建
- **DependencyModel**: 
  - `PerTarget`: 目标内部依赖
  - `ExternalTarget`: 跨目标依赖

### 工具链支持
- **IToolchain**: 工具链抽象接口
- **VisualStudioDoctor**: Windows 平台 MSVC 工具链
- **XCodeDoctor**: macOS 平台工具链
- **ArgumentDriver**: 编译器参数管理

## 构建流程

1. **Bootstrap**: 初始化构建环境，检测工具链
2. **LoadTargets**: 扫描并加载所有构建目标
3. **RunDoctors**: 执行工具链检查和配置
4. **AddTaskEmitters**: 注册任务发射器
5. **RunBuild**: 执行构建图

## 目录结构

``` txt
.sb/                    # 临时文件目录
  VisualStudio/        # VS 工程文件
  tools/               # 下载的工具
  downloads/           # 下载缓存

.build/                # 构建输出目录
  [ToolchainName]/     # 按工具链分类
    bin/               # 可执行文件
    lib/               # 库文件
    obj/               # 对象文件

.pkgs/                 # 包管理目录
  .build/              # 包构建缓存
```

## 配置系统

- 支持全局配置和目标特定配置
- 通过 `Target.Arguments` 传递编译参数
- 支持条件编译和平台特定设置

## 创建新模块

``` c#
  // 在相应目录创建 build.cs 文件
  [TargetScript(TargetCategory.Engine)]
  public static class MyModule
  {
      static MyModule()
      {
          Engine.Module("MyModule", "MY_MODULE_API")
              .TargetType(TargetType.Dynamic)
              .IncludeDirs(Visibility.Public, "include")
              .AddCppFiles("src/**.cpp")
              .Depend(Visibility.Private, "SkrCore");
      }
  }
```

## 配置编译选项

```c#
  Target.CppVersion("20")              // C++20
      .Exception(false)                // 禁用异常
      .RTTI(false)                     // 禁用 RTTI
      .WarningLevel(WarningLevel.All)  // 警告级别
      .Optimization(OptimizationLevel.Fastest); // 优化级别
```

## 重要注意事项

### ArgumentDriver 的作用

ArgumentDriver 是 SB 的核心设计之一，它：

  1. 抽象编译器差异：不同编译器的参数格式不同，ArgumentDriver 提供统一接口
  2. 支持查询模式：即使不实际编译，也能获取编译参数
  3. 用于工程生成：VS/XCode 工程生成器通过它获取正确的编译配置

### 依赖系统

  - 使用 Depend 方法添加模块依赖
  - 使用 Visibility 控制依赖传播：
    - Public: 传播给依赖者
    - Private: 仅当前目标使用
    - Interface: 仅传播，自己不使用

### 性能考虑

  1. 增量构建：充分利用缓存系统，避免重复构建
  2. 并行构建：SB 自动并行化独立任务
  3. Unity Build：通过 UnityBuildEmitter 支持统一构建

### 扩展性

  新的 TaskEmitter 可以通过以下方式添加：

```c#
  Engine.AddTaskEmitter("TaskName", new CustomEmitter())
      .AddDependency("OtherTask", DependencyModel.PerTarget);
```

## 创建自定义 TaskEmitter

### TaskEmitter 基础

TaskEmitter 是 SB 构建系统的核心扩展机制，用于实现各种构建任务。每个 TaskEmitter 负责特定类型的任务，如编译、链接、代码生成等。

#### 基本结构

```c#
public class CustomTaskEmitter : ITaskEmitter
{
    // 任务名称
    public string Name => "CustomTask";
    
    // 依赖关系
    public Dictionary<string, DependencyModel> Dependencies { get; } = new();
    
    // 生命周期方法
    public virtual bool EnableEmitter(TargetCollection targets) => true;
    public abstract void EmitTargetTask(TaskList tasks, Target target);
    public virtual void PerTargetTask(Target target) { }
    public virtual void PostGenerate() { }
}
```

#### 生命周期详解

1. **EnableEmitter**: 在任务发射器注册时调用，用于检查是否应该启用该发射器
   - 参数：所有目标的集合
   - 返回：是否启用此发射器
   - 用途：检查环境、工具链可用性等

2. **EmitTargetTask**: 为每个目标生成任务
   - 参数：任务列表和当前目标
   - 用途：添加编译、链接等具体任务

3. **PerTargetTask**: 每个目标的预处理
   - 参数：当前目标
   - 用途：收集信息、准备数据

4. **PostGenerate**: 所有任务生成后调用
   - 用途：生成汇总文件、清理临时数据

### 实践示例：VSCode 调试配置生成器

以下是一个完整的 TaskEmitter 实现示例，展示了最佳实践：

```c#
public class VSCodeDebugEmitter : ITaskEmitter
{
    // 1. 基本属性
    public string Name => "VSCodeDebug";
    public Dictionary<string, DependencyModel> Dependencies { get; } = new();
    
    // 2. 数据收集
    private readonly List<Target> executableTargets = new();
    
    // 3. 启用检查
    public bool EnableEmitter(TargetCollection targets)
    {
        // 检查是否有可执行目标
        return targets.Any(t => t.GetTargetType() == TargetType.Executable);
    }
    
    // 4. 目标处理
    public void PerTargetTask(Target target)
    {
        // 收集可执行目标
        if (target.GetTargetType() == TargetType.Executable)
        {
            executableTargets.Add(target);
        }
    }
    
    // 5. 任务生成（可选）
    public void EmitTargetTask(TaskList tasks, Target target)
    {
        // VSCode 配置生成不需要编译任务
        // 其他 TaskEmitter 可能需要在这里添加任务
    }
    
    // 6. 后处理生成文件
    public void PostGenerate()
    {
        if (executableTargets.Count == 0) return;
        
        // 生成 launch.json
        GenerateLaunchJson();
        
        // 生成 tasks.json
        GenerateTasksJson();
    }
    
    private void GenerateLaunchJson()
    {
        var configurations = new List<object>();
        
        foreach (var target in executableTargets)
        {
            // 使用 Target API 获取信息
            var binaryPath = target.GetBinaryPath();
            var targetName = target.FullName;
            
            // 获取平台特定信息
            var isWindows = RuntimeInformation.IsOSPlatform(OSPlatform.Windows);
            
            configurations.Add(new
            {
                name = $"[SB Generated] {targetName}",
                type = "cppdbg",
                request = "launch",
                program = binaryPath,
                cwd = "${workspaceFolder}",
                environment = GetEnvironmentVariables(target)
            });
        }
        
        // 序列化为 JSON
        var json = JsonSerializer.Serialize(new { configurations }, 
            new JsonSerializerOptions { WriteIndented = true });
        
        File.WriteAllText(".vscode/launch.json", json);
    }
}
```

### 关键 API 和最佳实践

#### 1. Target API 使用

```c#
// 获取目标类型
TargetType type = target.GetTargetType();

// 获取二进制路径
string binaryPath = target.GetBinaryPath();

// 获取输出目录
string outputDir = target.GetOutputDirectory();

// 获取依赖
var dependencies = target.GetDependencies();

// 获取编译参数
var compileArgs = target.Arguments.GetCompileArguments();
```

#### 2. 平台检测

```c#
// 使用 RuntimeInformation（推荐）
if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
{
    // Windows 特定逻辑
}

// 使用构建系统属性
var toolchain = BS.CurrentToolchain;
if (toolchain.GetType().Name.Contains("VisualStudio"))
{
    // MSVC 特定逻辑
}
```

#### 3. 文件路径处理

```c#
// 始终使用 Path.Combine
var outputPath = Path.Combine(BS.IntermediateDirectory, "generated", "file.json");

// 创建目录
Directory.CreateDirectory(Path.GetDirectoryName(outputPath));

// 相对路径处理
var relativePath = Path.GetRelativePath(BS.RootDirectory, absolutePath);
```

#### 4. JSON 序列化

```c#
// 使用 System.Text.Json
var options = new JsonSerializerOptions
{
    WriteIndented = true,
    PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
    DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
};

// 处理复杂对象
var json = JsonSerializer.Serialize(data, options);

// 保留现有配置
if (File.Exists(configPath))
{
    var existing = JsonSerializer.Deserialize<JsonDocument>(File.ReadAllText(configPath));
    // 合并配置...
}
```

#### 5. 错误处理

```c#
public void PostGenerate()
{
    try
    {
        GenerateFiles();
    }
    catch (Exception ex)
    {
        Console.WriteLine($"Warning: Failed to generate files: {ex.Message}");
        // TaskEmitter 通常不应该失败整个构建
    }
}
```

### 注册和使用

#### 在构建系统中注册

```c#
// 方式 1：直接注册
Engine.AddTaskEmitter("VSCodeDebug", new VSCodeDebugEmitter());

// 方式 2：带依赖注册
Engine.AddTaskEmitter("VSCodeDebug", new VSCodeDebugEmitter())
    .AddDependency("CppCompile", DependencyModel.PerTarget)
    .AddDependency("CppLink", DependencyModel.PerTarget);

// 方式 3：条件注册
if (BS.Arguments.Contains("--vscode"))
{
    Engine.AddTaskEmitter("VSCodeDebug", new VSCodeDebugEmitter());
}
```

#### 命令行集成

```c#
// 创建专用命令
public class VSCodeCommand : ICommand
{
    public string Name => "vscode";
    
    public void Execute(string[] args)
    {
        // 配置构建系统
        BS.ConfigureForVSCode();
        
        // 添加 TaskEmitter
        Engine.AddTaskEmitter("VSCodeDebug", new VSCodeDebugEmitter());
        
        // 运行构建（只生成，不编译）
        BS.RunGenerateOnly();
    }
}
```

### 调试技巧

1. **日志输出**
   ```c#
   Console.WriteLine($"[{Name}] Processing target: {target.FullName}");
   ```

2. **条件断点**
   ```c#
   if (target.FullName == "MyApp")
   {
       // 设置断点
       Debugger.Break();
   }
   ```

3. **验证生成的文件**
   ```c#
   var generatedFile = "path/to/file";
   if (File.Exists(generatedFile))
   {
       var content = File.ReadAllText(generatedFile);
       Console.WriteLine($"Generated file size: {content.Length}");
   }
   ```

### 常见陷阱和解决方案

1. **路径分隔符问题**
   - 错误：使用硬编码的 `/` 或 `\`
   - 正确：使用 `Path.Combine` 或 `Path.DirectorySeparatorChar`

2. **目标类型假设**
   - 错误：假设所有目标都有二进制输出
   - 正确：检查 `target.GetTargetType()` 和 `target.GetBinaryPath()`

3. **平台特定逻辑**
   - 错误：使用 `Environment.OSVersion`
   - 正确：使用 `RuntimeInformation.IsOSPlatform()`

4. **文件覆盖**
   - 错误：直接覆盖用户文件
   - 正确：保留用户配置，只更新生成的部分

5. **异常处理**
   - 错误：让异常传播导致构建失败
   - 正确：捕获异常并输出警告