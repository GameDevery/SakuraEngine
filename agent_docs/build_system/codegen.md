# SakuraEngine 代码生成系统

## 概述

SakuraEngine 的代码生成系统是一个基于 Clang LibTooling 和 TypeScript 的元编程框架，用于自动生成 RTTR 注册代码、序列化代码等样板代码。系统通过解析 C++ AST 和自定义属性，大大减少了手动编写重复代码的工作量。

## 系统架构

### 工作流程

```
┌─────────────────┐     ┌──────────────┐     ┌────────────────┐
│   C++ 源文件    │ ──> │  Meta 工具   │ ──> │   .meta 文件   │
│ (带 sreflect)   │     │ (Clang AST)  │     │  (JSON 格式)   │
└─────────────────┘     └──────────────┘     └────────────────┘
                                                      │
                                                      ▼
┌─────────────────┐     ┌──────────────┐     ┌────────────────┐
│ 生成的 C++ 代码 │ <── │  TypeScript  │ <── │  代码生成器    │
│ (.generated.h)  │     │   渲染器     │     │  (*.meta.ts)   │
└─────────────────┘     └──────────────┘     └────────────────┘
```

### 核心组件

1. **Meta 工具** - C++ AST 解析器，提取类型信息和属性
2. **代码生成器** - TypeScript 编写的生成逻辑
3. **SB 集成** - 通过 TaskEmitter 驱动整个流程
4. **元语言编译器** - 处理属性中的元语言表达式

## 基本使用

### 标记需要生成的类型

使用 `sreflect` 宏标记类型：

```cpp
// 标记结构体
sreflect_struct(
    guid = "12345678-1234-5678-1234-567812345678";
    rttr = @enable;                    // 启用 RTTR 生成
    serialize = @enable;               // 启用序列化生成
    rttr.flags = ["ScriptVisible"];    // 额外标记
)
struct PlayerData {
    // 标记字段
    sattr(serialize.no = false)
    float health;
    
    sattr(rttr.flags = ["ReadOnly"])
    uint32_t level;
    
    // 标记方法
    sattr(rttr.script_mixin = true)
    void LevelUp();
    
    // 不带标记的成员不会被处理
    int internal_state;
};

// 标记枚举
sreflect_enum(
    guid = "87654321-4321-8765-4321-876543210987";
    rttr = @enable;
)
enum class GameState : uint8_t {
    sattr(display = "主菜单")
    MainMenu,
    
    sattr(display = "游戏中")
    Playing,
    
    sattr(display = "暂停")
    Paused
};
```

## 总结

SakuraEngine 的代码生成系统通过结合 Clang 的强大解析能力和 TypeScript 的灵活性，提供了一个高效、可扩展的元编程解决方案。系统不仅减少了样板代码，还通过类型安全和增量构建等特性保证了开发效率和代码质量。