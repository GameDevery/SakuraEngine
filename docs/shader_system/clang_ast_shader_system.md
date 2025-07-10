# SakuraEngine Clang AST 着色器系统设计文档

## 概述

SakuraEngine 采用了创新的着色器开发方式，使用 **C++ 作为着色器语言**，通过 Clang LibTooling 解析 C++ AST，再转换为中间表示（ShaderAST），最终生成 HLSL/MSL 等目标着色器代码。

## 系统架构

```
C++ 源码 → Clang AST → Shader AST → HLSL/MSL/GLSL
```

### 核心组件

- **CppSLLLVM**：基于 Clang LibTooling 的 C++ AST 解析器（位于 `tools/shader_compiler/LLVM`）
- **CppSLAst**：中间层着色器 AST 表示（位于 `tools/shader_compiler/AST`）
- **代码生成器**：将 Shader AST 转换为各种目标语言

## Clang AST 解析实现

### ShaderCompiler 主框架

```cpp
// tools/shader_compiler/LLVM/src/shader_compiler.hpp
struct ShaderCompiler {
    static ShaderCompiler* Create(int argc, const char **argv);
    virtual int Run() = 0;
    virtual const AST& GetAST() const = 0;
};

// 实现类使用 Clang Tooling
struct ShaderCompilerImpl : public ShaderCompiler {
    CommonOptionsParser OptionsParser;
    ClangTool tool;
    skr::CppSL::AST AST;
    
    int Run() override {
        // 创建 AST Consumer 并运行
        tool.run(newFrontendActionFactory<ASTFrontendAction>().get());
        return 0;
    }
};
```

### AST Consumer 设计

AST Consumer 负责遍历 Clang AST 并构建 ShaderAST：

```cpp
class ASTConsumer : public clang::ASTConsumer, 
                    public clang::RecursiveASTVisitor<ASTConsumer> {
public:
    // 访问器方法
    bool VisitFunctionDecl(const clang::FunctionDecl* x);
    bool VisitRecordDecl(const clang::RecordDecl* x);
    bool VisitEnumDecl(const clang::EnumDecl* x);
    bool VisitVarDecl(const clang::VarDecl* x);
    
private:
    // 类型映射表
    std::map<const clang::TagDecl*, skr::CppSL::TypeDecl*> _tag_types;
    std::map<const clang::BuiltinType::Kind, skr::CppSL::TypeDecl*> _builtin_types;
    std::map<const clang::FunctionDecl*, skr::CppSL::FunctionDecl*> _funcs;
    
    // 转换方法
    TypeDecl* TranslateType(const clang::QualType& type);
    Stmt* TranslateStmt(const clang::Stmt* stmt);
    Expr* TranslateExpr(const clang::Expr* expr);
};
```

## C++ 到 ShaderAST 的转换

### 类型系统映射

#### 基础类型映射

```cpp
void ASTConsumer::SetupBuiltinTypes() {
    // 标量类型
    addType(Context.FloatTy, AST.FloatType);
    addType(Context.IntTy, AST.IntType);
    addType(Context.BoolTy, AST.BoolType);
    addType(Context.VoidTy, AST.VoidType);
    
    // 无符号类型
    addType(Context.UnsignedIntTy, AST.UintType);
    addType(Context.UnsignedShortTy, AST.UshortType);
}
```

#### 向量类型识别

通过模板特化识别向量类型：

```cpp
if (TSD && What == "vec") {
    const auto ET = Arguments.get(0).getAsType();  // 元素类型
    const uint64_t N = Arguments.get(1).getAsIntegral(); // 维度
    
    if (getType(ET) == AST.FloatType) {
        const TypeDecl* Types[] = { 
            AST.Float2Type, AST.Float3Type, AST.Float4Type 
        };
        addType(ThisQualType, Types[N - 2]);
    }
    else if (getType(ET) == AST.IntType) {
        const TypeDecl* Types[] = { 
            AST.Int2Type, AST.Int3Type, AST.Int4Type 
        };
        addType(ThisQualType, Types[N - 2]);
    }
}
```

### 属性系统

使用 C++ 的 `[[clang::annotate]]` 属性标记着色器特性：

#### 着色器入口点

```cpp
// 计算着色器
[[clang::annotate("skr-shader", "kernel", 16, 16)]]
void compute_main();

// 顶点着色器
[[clang::annotate("skr-shader", "vertex")]]
PSOutput vs_main(VSInput input);

// 像素着色器
[[clang::annotate("skr-shader", "fragment")]]
float4 ps_main(PSInput input);
```

#### 内置变量

```cpp
// 线程 ID
[[clang::annotate("skr-shader", "builtin", "ThreadID")]]
uint2 tid;

// 顶点 ID
[[clang::annotate("skr-shader", "builtin", "VertexID")]]
uint vid;

// 位置输出
[[clang::annotate("skr-shader", "builtin", "Position")]]
float4 position;
```

#### 资源绑定

```cpp
// Buffer 绑定
[[clang::annotate("skr-shader", "binding", 0, 0)]]
Buffer<float4>& buffer;

// 纹理绑定
[[clang::annotate("skr-shader", "binding", 1, 0)]]
Texture2D<float4>& texture;

// 采样器绑定
[[clang::annotate("skr-shader", "binding", 2, 0)]]
Sampler& sampler;
```

### 语句和表达式转换

#### 二元运算符处理

```cpp
Stmt* ASTConsumer::TranslateStmt(const clang::Stmt* x) {
    if (auto binOp = dyn_cast<BinaryOperator>(x)) {
        auto op = TranslateBinaryOp(binOp->getOpcode());
        auto left = TranslateStmt(binOp->getLHS());
        auto right = TranslateStmt(binOp->getRHS());
        return AST.Binary(op, left, right);
    }
}

BinaryOp TranslateBinaryOp(clang::BinaryOperatorKind op) {
    switch (op) {
        case BO_Add: return BinaryOp::ADD;
        case BO_Sub: return BinaryOp::SUB;
        case BO_Mul: return BinaryOp::MUL;
        case BO_Div: return BinaryOp::DIV;
        // ... 其他运算符
    }
}
```

#### 函数调用处理

```cpp
if (auto call = dyn_cast<CallExpr>(x)) {
    auto callee = TranslateStmt(call->getCallee());
    std::vector<Expr*> args;
    for (auto arg : call->arguments()) {
        args.push_back(TranslateStmt(arg));
    }
    return AST.CallFunction(callee, args);
}
```

#### 控制流语句

```cpp
// If 语句
if (auto ifStmt = dyn_cast<IfStmt>(x)) {
    auto cond = TranslateExpr(ifStmt->getCond());
    auto then = TranslateStmt(ifStmt->getThen());
    auto else_ = ifStmt->getElse() ? TranslateStmt(ifStmt->getElse()) : nullptr;
    return AST.If(cond, then, else_);
}

// For 循环
if (auto forStmt = dyn_cast<ForStmt>(x)) {
    auto init = TranslateStmt(forStmt->getInit());
    auto cond = TranslateExpr(forStmt->getCond());
    auto inc = TranslateStmt(forStmt->getInc());
    auto body = TranslateStmt(forStmt->getBody());
    return AST.For(init, cond, inc, body);
}
```

### 特殊着色器构造处理

#### 向量构造

```cpp
// 识别向量构造函数
if (isVectorConstruct(call)) {
    auto type = getVectorType(call);
    std::vector<Expr*> components;
    for (auto arg : call->arguments()) {
        components.push_back(TranslateExpr(arg));
    }
    return AST.Construct(type, components);
}
```

#### 纹理采样

```cpp
// 纹理采样调用
if (isSampleCall(call)) {
    auto texture = TranslateExpr(call->getArg(0));
    auto sampler = TranslateExpr(call->getArg(1));
    auto coords = TranslateExpr(call->getArg(2));
    return AST.SampleTexture(texture, sampler, coords);
}
```

## 使用示例

### 计算着色器示例

```cpp
#include "std/std.hpp"
using namespace skr::shader;

// 声明外部资源
extern Buffer<float4>& output_buffer;
extern Texture2D<float4>& input_texture;

// 辅助函数
float4 process_pixel(uint2 coord, uint2 size) {
    float2 uv = float2(coord) / float2(size);
    return float4(uv.x, uv.y, 0.0f, 1.0f);
}

// 计算着色器入口点
[[clang::annotate("skr-shader", "kernel", 8, 8)]]
void compute_main([[clang::annotate("skr-shader", "builtin", "ThreadID")]] uint2 tid) {
    const uint2 size = uint2(1920, 1080);
    
    if (tid.x < size.x && tid.y < size.y) {
        uint index = tid.x + tid.y * size.x;
        output_buffer.store(index, process_pixel(tid, size));
    }
}
```

### 顶点/像素着色器示例

```cpp
// 输入输出结构
struct VSInput {
    [[clang::annotate("skr-shader", "semantic", "POSITION")]]
    float3 position;
    
    [[clang::annotate("skr-shader", "semantic", "TEXCOORD0")]]
    float2 uv;
};

struct PSInput {
    [[clang::annotate("skr-shader", "stage_inout")]]
    [[clang::annotate("skr-shader", "builtin", "Position")]]
    float4 position;
    
    float2 uv;
};

// 顶点着色器
[[clang::annotate("skr-shader", "vertex")]]
PSInput vs_main(VSInput input) {
    PSInput output;
    output.position = float4(input.position, 1.0f);
    output.uv = input.uv;
    return output;
}

// 像素着色器
[[clang::annotate("skr-shader", "fragment")]]
float4 ps_main(PSInput input) {
    return float4(input.uv.x, input.uv.y, 0.0f, 1.0f);
}
```

## 技术优势

1. **类型安全**：利用 C++ 的强类型系统在编译期捕获错误
2. **代码复用**：可以使用 C++ 的函数、模板等特性复用代码
3. **工具链支持**：可以使用现有的 C++ IDE、调试器等工具
4. **渐进式迁移**：可以逐步将传统着色器代码迁移到 C++
5. **跨平台一致性**：同一份代码生成不同平台的着色器

## 限制和注意事项

1. **C++ 子集**：只支持 C++ 的一个子集，不支持动态内存分配、虚函数等
2. **GPU 限制**：需要遵守 GPU 编程的限制（如递归深度、动态分支等）
3. **平台差异**：某些特性在不同平台可能有细微差异
4. **调试支持**：虽然可以使用 C++ 工具，但 GPU 调试仍需专门工具

## 总结

SakuraEngine 的 Clang AST 着色器系统展示了如何利用现有的编译器基础设施来构建领域特定语言。通过将 C++ 作为着色器语言，不仅提供了类型安全和代码复用的优势，还大大简化了跨平台着色器开发的复杂性。这是一个优雅且实用的工程解决方案。