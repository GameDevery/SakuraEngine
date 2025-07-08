# TaskEmitter 开发指南

## 概述

TaskEmitter 是 SakuraEngine 构建系统的核心扩展机制。本指南详细介绍如何开发自定义 TaskEmitter，包括高级特性、性能优化和实际案例分析。

## 深入理解 TaskEmitter

### 执行流程

```
构建系统启动
    ↓
加载所有 Target
    ↓
注册 TaskEmitter
    ↓
调用 EnableEmitter() → 决定是否启用
    ↓
对每个 Target 调用 PerTargetTask() → 收集信息
    ↓
对每个 Target 调用 EmitTargetTask() → 生成任务
    ↓
执行所有任务
    ↓
调用 PostGenerate() → 后处理
```

### TaskEmitter 分类

#### 1. 编译型 TaskEmitter
负责生成编译和链接任务：

```c#
public class CompilerEmitter : ITaskEmitter
{
    public void EmitTargetTask(TaskList tasks, Target target)
    {
        foreach (var sourceFile in target.SourceFiles)
        {
            tasks.Add(new CompileTask
            {
                Source = sourceFile,
                Output = GetObjectPath(sourceFile),
                Arguments = GetCompileArgs(target)
            });
        }
    }
}
```

#### 2. 生成型 TaskEmitter
负责生成代码或配置文件：

```c#
public class GeneratorEmitter : ITaskEmitter
{
    private readonly Dictionary<Target, GeneratedFiles> generatedFiles = new();
    
    public void PerTargetTask(Target target)
    {
        // 分析目标，准备生成
        var files = AnalyzeTarget(target);
        generatedFiles[target] = files;
    }
    
    public void PostGenerate()
    {
        // 批量生成所有文件
        foreach (var (target, files) in generatedFiles)
        {
            GenerateFiles(target, files);
        }
    }
}
```

#### 3. 分析型 TaskEmitter
负责分析和报告：

```c#
public class AnalyzerEmitter : ITaskEmitter
{
    public void EmitTargetTask(TaskList tasks, Target target)
    {
        // 不生成构建任务，只分析
    }
    
    public void PostGenerate()
    {
        // 生成分析报告
        GenerateReport();
    }
}
```

## 高级特性

### 1. 依赖管理

#### 声明依赖关系

```c#
public class MyEmitter : ITaskEmitter
{
    public Dictionary<string, DependencyModel> Dependencies { get; } = new()
    {
        // 依赖编译任务
        ["CppCompile"] = DependencyModel.PerTarget,
        
        // 依赖所有目标的代码生成
        ["CodegenMeta"] = DependencyModel.AllTargets,
        
        // 外部目标依赖
        ["ResourcePack"] = DependencyModel.ExternalTarget
    };
}
```

#### 动态依赖

```c#
public bool EnableEmitter(TargetCollection targets)
{
    // 根据条件动态添加依赖
    if (NeedsSpecialProcessing())
    {
        Dependencies["SpecialTask"] = DependencyModel.PerTarget;
    }
    
    return true;
}
```

### 2. 任务并行化

```c#
public void PostGenerate()
{
    // 使用并行处理提高性能
    Parallel.ForEach(targetGroups, new ParallelOptions
    {
        MaxDegreeOfParallelism = Environment.ProcessorCount
    }, 
    group =>
    {
        ProcessTargetGroup(group);
    });
}
```

### 3. 增量构建支持

```c#
public class IncrementalEmitter : ITaskEmitter
{
    private readonly Dictionary<string, FileInfo> cache = new();
    
    public void EmitTargetTask(TaskList tasks, Target target)
    {
        var outputPath = GetOutputPath(target);
        var inputHash = ComputeInputHash(target);
        
        // 检查缓存
        if (cache.TryGetValue(outputPath, out var cachedInfo))
        {
            if (cachedInfo.Hash == inputHash)
            {
                // 跳过，输出已是最新
                return;
            }
        }
        
        // 添加任务
        tasks.Add(new GenerateTask
        {
            Target = target,
            Output = outputPath,
            OnComplete = () => cache[outputPath] = new FileInfo { Hash = inputHash }
        });
    }
}
```

### 4. 配置管理

```c#
public class ConfigurableEmitter : ITaskEmitter
{
    public class Config
    {
        public bool EnableOptimization { get; set; } = true;
        public string OutputFormat { get; set; } = "json";
        public List<string> ExcludePatterns { get; set; } = new();
    }
    
    private Config config;
    
    public bool EnableEmitter(TargetCollection targets)
    {
        // 从命令行或配置文件加载配置
        config = LoadConfiguration();
        return config != null;
    }
    
    private Config LoadConfiguration()
    {
        // 优先级：命令行 > 项目配置 > 默认值
        var config = new Config();
        
        // 从命令行
        if (BS.Arguments.TryGetValue("--emitter-opt", out var opt))
        {
            config.EnableOptimization = bool.Parse(opt);
        }
        
        // 从项目配置文件
        var configPath = Path.Combine(BS.RootDirectory, ".sb/emitter.json");
        if (File.Exists(configPath))
        {
            var json = File.ReadAllText(configPath);
            config = JsonSerializer.Deserialize<Config>(json);
        }
        
        return config;
    }
}
```

## 与构建系统集成

### 1. 访问构建系统状态

```c#
public class SystemAwareEmitter : ITaskEmitter
{
    public void EmitTargetTask(TaskList tasks, Target target)
    {
        // 访问构建模式
        var isDebug = BS.BuildMode == BuildMode.Debug;
        
        // 访问工具链信息
        var compiler = BS.CurrentToolchain.GetCompiler();
        
        // 访问输出目录
        var outputDir = BS.GetOutputDirectory(target);
        
        // 访问中间文件目录
        var intermediateDir = Path.Combine(BS.IntermediateDirectory, Name, target.Name);
    }
}
```

### 2. 与其他 TaskEmitter 协作

```c#
public class CooperativeEmitter : ITaskEmitter
{
    // 存储供其他 Emitter 使用的数据
    public static readonly Dictionary<Target, ProcessedData> SharedData = new();
    
    public void PerTargetTask(Target target)
    {
        // 处理并存储数据
        var data = ProcessTarget(target);
        SharedData[target] = data;
    }
}

public class ConsumerEmitter : ITaskEmitter
{
    public Dictionary<string, DependencyModel> Dependencies { get; } = new()
    {
        ["Cooperative"] = DependencyModel.PerTarget
    };
    
    public void EmitTargetTask(TaskList tasks, Target target)
    {
        // 使用其他 Emitter 的数据
        if (CooperativeEmitter.SharedData.TryGetValue(target, out var data))
        {
            UseProcessedData(data);
        }
    }
}
```

### 3. 自定义任务类型

```c#
public class CustomTask : ITask
{
    public string Name { get; set; }
    public Target Target { get; set; }
    public Action<ITaskContext> Execute { get; set; }
    
    public void Run(ITaskContext context)
    {
        context.LogInfo($"Executing {Name} for {Target.Name}");
        Execute?.Invoke(context);
    }
}

public class CustomTaskEmitter : ITaskEmitter
{
    public void EmitTargetTask(TaskList tasks, Target target)
    {
        tasks.Add(new CustomTask
        {
            Name = "CustomProcess",
            Target = target,
            Execute = ctx => ProcessTarget(target, ctx)
        });
    }
}
```

## 性能优化

### 1. 批处理

```c#
public class BatchEmitter : ITaskEmitter
{
    private const int BatchSize = 50;
    private readonly List<Target> allTargets = new();
    
    public void PerTargetTask(Target target)
    {
        allTargets.Add(target);
    }
    
    public void PostGenerate()
    {
        // 分批处理避免内存压力
        for (int i = 0; i < allTargets.Count; i += BatchSize)
        {
            var batch = allTargets.Skip(i).Take(BatchSize);
            ProcessBatch(batch);
        }
    }
}
```

### 2. 缓存优化

```c#
public class CachedEmitter : ITaskEmitter
{
    private readonly MemoryCache cache = new MemoryCache(new MemoryCacheOptions
    {
        SizeLimit = 1000
    });
    
    public void EmitTargetTask(TaskList tasks, Target target)
    {
        var cacheKey = GetCacheKey(target);
        
        if (!cache.TryGetValue(cacheKey, out ProcessedResult result))
        {
            result = ProcessTarget(target);
            
            var cacheEntry = cache.CreateEntry(cacheKey);
            cacheEntry.Value = result;
            cacheEntry.Size = 1;
            cacheEntry.SlidingExpiration = TimeSpan.FromMinutes(5);
        }
        
        UseResult(result);
    }
}
```

### 3. 异步处理

```c#
public class AsyncEmitter : ITaskEmitter
{
    public async void PostGenerate()
    {
        var tasks = targets.Select(async target =>
        {
            await ProcessTargetAsync(target);
        });
        
        await Task.WhenAll(tasks);
    }
    
    private async Task ProcessTargetAsync(Target target)
    {
        // 异步 I/O 操作
        var data = await ReadDataAsync(target);
        await WriteOutputAsync(target, data);
    }
}
```

## 错误处理和诊断

### 1. 验证和错误报告

```c#
public class ValidatingEmitter : ITaskEmitter
{
    private readonly List<string> errors = new();
    private readonly List<string> warnings = new();
    
    public bool EnableEmitter(TargetCollection targets)
    {
        // 验证环境
        if (!ValidateEnvironment())
        {
            Console.WriteLine("Error: Required tools not found");
            return false;
        }
        
        return true;
    }
    
    public void PerTargetTask(Target target)
    {
        // 验证目标配置
        if (!ValidateTarget(target, out var error))
        {
            errors.Add($"{target.Name}: {error}");
        }
    }
    
    public void PostGenerate()
    {
        // 报告所有错误
        if (errors.Count > 0)
        {
            Console.WriteLine($"\n{Name} Errors:");
            foreach (var error in errors)
            {
                Console.WriteLine($"  - {error}");
            }
            
            throw new BuildException($"{Name} failed with {errors.Count} errors");
        }
        
        // 报告警告
        if (warnings.Count > 0)
        {
            Console.WriteLine($"\n{Name} Warnings:");
            foreach (var warning in warnings)
            {
                Console.WriteLine($"  - {warning}");
            }
        }
    }
}
```

### 2. 调试支持

```c#
public class DebuggableEmitter : ITaskEmitter
{
    private readonly bool debugMode;
    
    public DebuggableEmitter()
    {
        debugMode = Environment.GetEnvironmentVariable("SB_DEBUG_EMITTER") == "1";
    }
    
    private void DebugLog(string message)
    {
        if (debugMode)
        {
            Console.WriteLine($"[DEBUG][{Name}] {message}");
        }
    }
    
    public void EmitTargetTask(TaskList tasks, Target target)
    {
        DebugLog($"Processing target: {target.Name}");
        DebugLog($"  Type: {target.GetTargetType()}");
        DebugLog($"  Dependencies: {string.Join(", ", target.GetDependencies())}");
        
        // 实际处理...
    }
}
```

## 实际案例分析

### 案例 1：资源打包 TaskEmitter

```c#
public class ResourcePackEmitter : ITaskEmitter
{
    public string Name => "ResourcePack";
    
    private class ResourceInfo
    {
        public string SourcePath { get; set; }
        public string PackPath { get; set; }
        public string Hash { get; set; }
    }
    
    private readonly Dictionary<Target, List<ResourceInfo>> targetResources = new();
    
    public void PerTargetTask(Target target)
    {
        // 收集资源文件
        var resources = new List<ResourceInfo>();
        
        foreach (var pattern in target.ResourcePatterns)
        {
            var files = Directory.GetFiles(target.Directory, pattern, SearchOption.AllDirectories);
            
            foreach (var file in files)
            {
                resources.Add(new ResourceInfo
                {
                    SourcePath = file,
                    PackPath = Path.GetRelativePath(target.Directory, file),
                    Hash = ComputeFileHash(file)
                });
            }
        }
        
        targetResources[target] = resources;
    }
    
    public void EmitTargetTask(TaskList tasks, Target target)
    {
        if (!targetResources.TryGetValue(target, out var resources))
            return;
        
        var packFile = Path.Combine(BS.GetOutputDirectory(target), $"{target.Name}.pak");
        
        tasks.Add(new Task
        {
            Name = $"Pack_{target.Name}",
            Execute = () => CreateResourcePack(packFile, resources)
        });
    }
    
    private void CreateResourcePack(string packFile, List<ResourceInfo> resources)
    {
        using var archive = ZipFile.Open(packFile, ZipArchiveMode.Create);
        
        foreach (var resource in resources)
        {
            archive.CreateEntryFromFile(resource.SourcePath, resource.PackPath);
        }
        
        // 写入清单
        var manifest = JsonSerializer.Serialize(resources);
        var manifestEntry = archive.CreateEntry("manifest.json");
        using var writer = new StreamWriter(manifestEntry.Open());
        writer.Write(manifest);
    }
}
```

### 案例 2：文档生成 TaskEmitter

```c#
public class DocGeneratorEmitter : ITaskEmitter
{
    public string Name => "DocGenerator";
    
    private readonly List<Target> documentedTargets = new();
    
    public void PerTargetTask(Target target)
    {
        // 检查是否有文档注释
        if (target.HasDocumentation)
        {
            documentedTargets.Add(target);
        }
    }
    
    public void PostGenerate()
    {
        if (documentedTargets.Count == 0)
            return;
        
        // 生成文档索引
        var indexPath = Path.Combine(BS.RootDirectory, "docs/generated/index.md");
        Directory.CreateDirectory(Path.GetDirectoryName(indexPath));
        
        using var writer = new StreamWriter(indexPath);
        writer.WriteLine("# API Documentation");
        writer.WriteLine();
        
        foreach (var target in documentedTargets.OrderBy(t => t.Name))
        {
            writer.WriteLine($"- [{target.Name}](./{target.Name}.md)");
            GenerateTargetDoc(target);
        }
    }
    
    private void GenerateTargetDoc(Target target)
    {
        var docPath = Path.Combine(BS.RootDirectory, $"docs/generated/{target.Name}.md");
        
        using var writer = new StreamWriter(docPath);
        writer.WriteLine($"# {target.Name}");
        writer.WriteLine();
        writer.WriteLine(target.Description);
        writer.WriteLine();
        
        // 生成 API 文档...
    }
}
```

## 测试 TaskEmitter

### 单元测试

```c#
[TestClass]
public class MyEmitterTests
{
    [TestMethod]
    public void TestEnableEmitter()
    {
        var emitter = new MyEmitter();
        var targets = new TargetCollection();
        targets.Add(CreateTestTarget());
        
        var result = emitter.EnableEmitter(targets);
        
        Assert.IsTrue(result);
    }
    
    [TestMethod]
    public void TestTaskGeneration()
    {
        var emitter = new MyEmitter();
        var tasks = new TaskList();
        var target = CreateTestTarget();
        
        emitter.EmitTargetTask(tasks, target);
        
        Assert.AreEqual(1, tasks.Count);
        Assert.AreEqual("ExpectedTaskName", tasks[0].Name);
    }
    
    private Target CreateTestTarget()
    {
        return new Target
        {
            Name = "TestTarget",
            Type = TargetType.Executable
        };
    }
}
```

### 集成测试

```c#
[TestMethod]
public void TestFullBuildWithEmitter()
{
    // 设置测试环境
    var testDir = Path.Combine(Path.GetTempPath(), "EmitterTest");
    Directory.CreateDirectory(testDir);
    
    try
    {
        // 创建测试项目
        CreateTestProject(testDir);
        
        // 运行构建
        var result = BuildSystem.Build(new BuildOptions
        {
            RootDirectory = testDir,
            Emitters = new[] { new MyEmitter() }
        });
        
        // 验证结果
        Assert.IsTrue(result.Success);
        Assert.IsTrue(File.Exists(Path.Combine(testDir, "expected_output.json")));
    }
    finally
    {
        // 清理
        Directory.Delete(testDir, true);
    }
}
```

## 最佳实践总结

1. **保持单一职责** - 每个 TaskEmitter 应该专注于一个特定功能
2. **支持增量构建** - 避免不必要的重复工作
3. **提供清晰的错误信息** - 帮助用户快速定位问题
4. **考虑性能** - 使用并行处理和批处理优化大型项目
5. **编写测试** - 确保 TaskEmitter 的可靠性
6. **文档化配置选项** - 让用户了解如何自定义行为
7. **遵循命名约定** - 使用描述性的名称和一致的命名模式
8. **处理边缘情况** - 考虑空目标、无效配置等情况
9. **提供调试支持** - 添加日志和诊断功能
10. **版本兼容性** - 考虑向后兼容性和迁移路径

通过遵循这些指南和最佳实践，你可以创建强大、可靠且易于维护的 TaskEmitter，为 SakuraEngine 构建系统添加新的功能。