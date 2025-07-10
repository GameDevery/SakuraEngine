# SakuraEngine ShaderAST 设计与实现文档

## 概述

ShaderAST 是 SakuraEngine 自研的着色器中间表示（IR）系统，作为从 C++ AST 到各种着色器语言（HLSL、MSL、GLSL）的桥梁。它剔除了 C++ 的复杂特性，保留了着色器编程所需的核心功能，同时提供了 GPU 友好的类型系统和资源抽象。

## 设计理念

### 核心目标

1. **简化性**：剔除 C++ 的复杂特性（模板、继承、虚函数等）
2. **GPU 友好**：内置 GPU 内存布局规则和资源模型
3. **跨平台**：统一的中间表示支持多种目标语言
4. **类型安全**：保持强类型系统，便于验证和优化
5. **可扩展**：易于添加新的语言特性和目标平台

### 架构位置

```
┌─────────────┐     ┌──────────────┐     ┌──────────────┐
│  C++ 源码   │ ──> │  Clang AST   │ ──> │  ShaderAST   │
└─────────────┘     └──────────────┘     └──────────────┘
                                                  │
                    ┌──────────────────────────────┼──────────────────────────────┐
                    ▼                              ▼                              ▼
            ┌──────────────┐            ┌──────────────┐              ┌──────────────┐
            │     HLSL     │            │     MSL      │              │     GLSL     │
            └──────────────┘            └──────────────┘              └──────────────┘
```

## 核心数据结构

### AST 主类

```cpp
// tools/shader_compiler/AST/include/ast/ast.hpp
struct AST {
    // === 表达式构建器 ===
    BinaryExpr* Binary(BinaryOp op, Expr* left, Expr* right);
    UnaryExpr* Unary(UnaryOp op, Expr* arg);
    CallExpr* CallFunction(DeclRefExpr* callee, std::span<Expr*> args);
    ConstructExpr* Construct(const TypeDecl* type, std::span<Expr*> args);
    CastExpr* Cast(const TypeDecl* type, Expr* arg);
    MemberExpr* Member(Expr* object, FieldDecl* field);
    ArrayAccessExpr* ArrayAccess(Expr* array, Expr* index);
    
    // === 语句构建器 ===
    CompoundStmt* Block(const std::vector<Stmt*>& statements);
    IfStmt* If(Expr* cond, CompoundStmt* then_body, CompoundStmt* else_body = nullptr);
    ForStmt* For(Stmt* init, Expr* cond, Stmt* inc, CompoundStmt* body);
    WhileStmt* While(Expr* cond, CompoundStmt* body);
    ReturnStmt* Return(Expr* value = nullptr);
    
    // === 声明构建器 ===
    TypeDecl* DeclareStructure(const Name& name, std::span<FieldDecl*> members);
    FunctionDecl* DeclareFunction(const Name& name, const TypeDecl* return_type, 
                                  std::span<ParamDecl*> params, CompoundStmt* body);
    VarDecl* DeclareVariable(const Name& name, const TypeDecl* type, Expr* init = nullptr);
    
    // === 类型系统 ===
    const VectorTypeDecl* VectorType(const TypeDecl* element, uint32_t count);
    const MatrixTypeDecl* MatrixType(const TypeDecl* element, uint32_t rows, uint32_t cols);
    const ArrayTypeDecl* ArrayType(const TypeDecl* element, uint32_t count);
    const PointerTypeDecl* PointerType(const TypeDecl* pointee);
    
    // === 内置类型 ===
    const TypeDecl* VoidType;
    const TypeDecl* BoolType;
    const TypeDecl* FloatType, *DoubleType, *HalfType;
    const TypeDecl* IntType, *UintType;
    const TypeDecl* ShortType, *UshortType;
    
    // 向量类型
    VEC_TYPES(Float);   // Float2Type, Float3Type, Float4Type
    VEC_TYPES(Int);     // Int2Type, Int3Type, Int4Type
    VEC_TYPES(Uint);    // Uint2Type, Uint3Type, Uint4Type
    VEC_TYPES(Bool);    // Bool2Type, Bool3Type, Bool4Type
    
    // 矩阵类型
    MATRIX_TYPES(Float); // Float2x2Type, Float3x3Type, Float4x4Type
};
```

### 类型系统层次结构

```cpp
// 基础声明类
class Decl {
public:
    virtual DeclKind kind() const = 0;
    const std::vector<Attr*>& attrs() const;
};

// 命名声明
class NamedDecl : public Decl {
    Name _name;
public:
    const Name& name() const;
};

// 类型声明层次
class TypeDecl : public NamedDecl {
public:
    virtual uint32_t size() const = 0;
    virtual uint32_t align() const = 0;
};

// 值类型（可以存储在变量中）
class ValueTypeDecl : public TypeDecl {
public:
    virtual bool is_scalar() const { return false; }
    virtual bool is_vector() const { return false; }
    virtual bool is_matrix() const { return false; }
    virtual bool is_structure() const { return false; }
};

// 资源类型（GPU 资源）
class ResourceTypeDecl : public TypeDecl {
public:
    virtual ResourceKind resource_kind() const = 0;
};
```

### 具体类型实现

#### 标量类型

```cpp
class ScalarTypeDecl : public ValueTypeDecl {
    ScalarKind _kind;
public:
    bool is_scalar() const override { return true; }
    ScalarKind scalar_kind() const { return _kind; }
    
    // GPU 内存布局
    uint32_t size() const override {
        switch (_kind) {
            case ScalarKind::Bool: return 4;    // GPU bool 是 4 字节
            case ScalarKind::Float: return 4;
            case ScalarKind::Double: return 8;
            case ScalarKind::Int: return 4;
            case ScalarKind::Uint: return 4;
            // ...
        }
    }
};
```

#### 向量类型

```cpp
class VectorTypeDecl : public ValueTypeDecl {
    const TypeDecl* _element;
    uint32_t _count;
public:
    bool is_vector() const override { return true; }
    const TypeDecl& element() const { return *_element; }
    uint32_t count() const { return _count; }
    
    // GPU 对齐规则
    uint32_t align() const override {
        // float3 需要 16 字节对齐
        if (_element->size() == 4 && _count == 3) return 16;
        return _element->size() * _count;
    }
};
```

#### 矩阵类型

```cpp
class MatrixTypeDecl : public ValueTypeDecl {
    const TypeDecl* _element;
    uint32_t _rows, _cols;
public:
    bool is_matrix() const override { return true; }
    
    // 列主序存储（GPU 标准）
    uint32_t size() const override {
        return _element->size() * _rows * _cols;
    }
    
    uint32_t align() const override {
        // 矩阵按列对齐
        return VectorTypeDecl(_element, _rows).align();
    }
};
```

#### 结构体类型

```cpp
class StructureTypeDecl : public ValueTypeDecl {
    std::vector<FieldDecl*> _fields;
    mutable uint32_t _size = 0;
    mutable uint32_t _align = 0;
public:
    bool is_structure() const override { return true; }
    
    // 计算 GPU 内存布局
    void calculate_layout() const {
        uint32_t offset = 0;
        for (auto field : _fields) {
            auto field_align = field->type().align();
            _align = std::max(_align, field_align);
            
            // 对齐到字段要求
            offset = (offset + field_align - 1) & ~(field_align - 1);
            field->set_offset(offset);
            offset += field->type().size();
        }
        
        // 结构体大小需要对齐
        _size = (offset + _align - 1) & ~(_align - 1);
    }
};
```

### 资源类型

#### Buffer 类型

```cpp
class BufferTypeDecl : public ResourceTypeDecl {
    const TypeDecl* _element;
    BufferFlags _flags;
public:
    ResourceKind resource_kind() const override { 
        return ResourceKind::Buffer; 
    }
    
    bool is_read_only() const { 
        return !(_flags & BufferFlags::ReadWrite); 
    }
    
    bool is_structured() const {
        return _element->is_structure();
    }
};

// 具体 Buffer 类型
class StructuredBufferTypeDecl : public BufferTypeDecl {};
class ByteAddressBufferTypeDecl : public BufferTypeDecl {};
class ConstantBufferTypeDecl : public BufferTypeDecl {};
```

#### 纹理类型

```cpp
class TextureTypeDecl : public ResourceTypeDecl {
    const TypeDecl* _element;
    TextureDimension _dimension;
    TextureFlags _flags;
public:
    ResourceKind resource_kind() const override { 
        return ResourceKind::Texture; 
    }
    
    bool is_array() const { 
        return _flags & TextureFlags::Array; 
    }
    
    bool is_ms() const { 
        return _flags & TextureFlags::Multisample; 
    }
    
    uint32_t dimension_count() const {
        switch (_dimension) {
            case TextureDimension::Tex1D: return 1;
            case TextureDimension::Tex2D: return 2;
            case TextureDimension::Tex3D: return 3;
            case TextureDimension::TexCube: return 2;
        }
    }
};
```

## 属性系统

### 属性基类

```cpp
class Attr {
public:
    virtual AttrKind kind() const = 0;
};

// 属性种类
enum class AttrKind {
    Stage,          // 着色器阶段
    Semantic,       // 语义绑定
    ResourceBind,   // 资源绑定
    KernelSize,     // 计算核大小
    Builtin,        // 内置变量
    // ...
};
```

### 具体属性实现

```cpp
// 着色器阶段属性
class StageAttr : public Attr {
    ShaderStage _stage;
public:
    AttrKind kind() const override { return AttrKind::Stage; }
    ShaderStage stage() const { return _stage; }
};

// 语义属性
class SemanticAttr : public Attr {
    SemanticType _semantic;
public:
    AttrKind kind() const override { return AttrKind::Semantic; }
    SemanticType semantic() const { return _semantic; }
};

// 资源绑定属性
class ResourceBindAttr : public Attr {
    uint32_t _group;    // Vulkan set / HLSL space
    uint32_t _binding;  // Vulkan binding / HLSL register
public:
    AttrKind kind() const override { return AttrKind::ResourceBind; }
    uint32_t group() const { return _group; }
    uint32_t binding() const { return _binding; }
};

// 计算核大小属性
class KernelSizeAttr : public Attr {
    uint32_t _x, _y, _z;
public:
    AttrKind kind() const override { return AttrKind::KernelSize; }
    uint32_t x() const { return _x; }
    uint32_t y() const { return _y; }
    uint32_t z() const { return _z; }
};
```

## 代码生成

### 代码生成器接口

```cpp
class ICodeGenerator {
public:
    virtual ~ICodeGenerator() = default;
    virtual String generate(const AST& ast) = 0;
    
protected:
    virtual void visitDecl(const Decl* decl) = 0;
    virtual void visitStmt(const Stmt* stmt) = 0;
    virtual void visitExpr(const Expr* expr) = 0;
    virtual void visitType(const TypeDecl* type) = 0;
};
```

### HLSL 生成器

```cpp
class HLSLGenerator : public ICodeGenerator {
    SourceBuilderNew sb;
    
public:
    String generate(const AST& ast) override {
        // 生成头部
        generate_header();
        
        // 访问所有声明
        for (auto decl : ast.declarations()) {
            visitDecl(decl);
        }
        
        return sb.to_string();
    }
    
protected:
    void visitExpr(const Expr* expr) override {
        switch (expr->kind()) {
            case ExprKind::Binary: {
                auto binary = static_cast<const BinaryExpr*>(expr);
                
                // 特殊处理矩阵乘法
                if (is_matrix_mul(binary)) {
                    sb.append(L"mul(");
                    visitExpr(binary->left());
                    sb.append(L", ");
                    visitExpr(binary->right());
                    sb.append(L")");
                } else {
                    visitExpr(binary->left());
                    sb.append(L" ");
                    sb.append(op_to_string(binary->op()));
                    sb.append(L" ");
                    visitExpr(binary->right());
                }
                break;
            }
            // ... 其他表达式类型
        }
    }
    
    void generate_resource_binding(const VarDecl* var) {
        if (auto bind = find_attr<ResourceBindAttr>(var)) {
            auto reg_char = get_register_char(var->type());
            sb.append_format(L" : register({}{}, space{})",
                           reg_char, bind->binding(), bind->group());
        }
    }
};
```

### MSL 生成器

```cpp
class MSLGenerator : public ICodeGenerator {
protected:
    void generate_function_signature(const FunctionDecl* func) {
        // Metal 特定的函数签名
        if (is_kernel_function(func)) {
            sb.append(L"kernel ");
        } else if (is_vertex_function(func)) {
            sb.append(L"vertex ");
        } else if (is_fragment_function(func)) {
            sb.append(L"fragment ");
        }
        
        // 返回类型和函数名
        visitType(func->return_type());
        sb.append(L" ");
        sb.append(func->name());
        
        // 参数列表（包含 Metal 属性）
        generate_parameters_with_attributes(func);
    }
    
    String get_metal_attribute(const Attr* attr) {
        static const std::unordered_map<String, String> AttrMap = {
            { L"VertexID", L"[[vertex_id]]" },
            { L"InstanceID", L"[[instance_id]]" },
            { L"Position", L"[[position]]" },
            { L"ThreadID", L"[[thread_position_in_grid]]" },
            { L"GroupID", L"[[threadgroup_position_in_grid]]" },
        };
        // ...
    }
};
```

## 使用示例

### 构建简单的计算着色器 AST

```cpp
// 创建 AST
AST ast;

// 声明外部 Buffer
auto buffer_type = ast.StructuredBufferType(ast.Float4Type, BufferFlags::ReadWrite);
auto buffer_var = ast.DeclareVariable(L"output_buffer", buffer_type);
buffer_var->add_attr(new ResourceBindAttr(0, 0));

// 创建计算函数
auto tid_param = ast.DeclareParameter(L"tid", ast.Uint2Type);
tid_param->add_attr(new BuiltinAttr(L"ThreadID"));

auto func_body = ast.Block({
    // uint index = tid.x + tid.y * 1920;
    auto index_var = ast.DeclareVariable(L"index", ast.UintType,
        ast.Binary(BinaryOp::ADD,
            ast.Member(ast.Ref(tid_param), L"x"),
            ast.Binary(BinaryOp::MUL,
                ast.Member(ast.Ref(tid_param), L"y"),
                ast.Literal(1920u))));
    
    // output_buffer[index] = float4(1, 0, 0, 1);
    ast.ExprStmt(
        ast.ArrayAssign(
            ast.Ref(buffer_var),
            ast.Ref(index_var),
            ast.Construct(ast.Float4Type, {
                ast.Literal(1.0f), ast.Literal(0.0f),
                ast.Literal(0.0f), ast.Literal(1.0f)
            })));
});

auto compute_func = ast.DeclareFunction(L"compute_main", ast.VoidType, 
                                       {tid_param}, func_body);
compute_func->add_attr(new StageAttr(ShaderStage::Compute));
compute_func->add_attr(new KernelSizeAttr(8, 8, 1));
```

### 生成 HLSL 代码

```cpp
HLSLGenerator hlsl_gen;
String hlsl_code = hlsl_gen.generate(ast);

// 输出：
// RWStructuredBuffer<float4> output_buffer : register(u0, space0);
// 
// [numthreads(8, 8, 1)]
// void compute_main(uint2 tid : SV_DispatchThreadID) {
//     uint index = tid.x + tid.y * 1920;
//     output_buffer[index] = float4(1, 0, 0, 1);
// }
```

## GPU 内存布局规则

### 标准布局规则 (std140/std430)

```cpp
// GPU 对齐计算
template <typename T, int N>
inline static constexpr int gpu_vec_align() {
    // 标量和 vec2 按自身大小对齐
    if constexpr (N <= 2) return sizeof(T) * N;
    // vec3 和 vec4 按 16 字节对齐
    else return 16;
}

// 结构体成员对齐
struct GPULayout {
    static uint32_t align_offset(uint32_t offset, uint32_t alignment) {
        return (offset + alignment - 1) & ~(alignment - 1);
    }
    
    static uint32_t struct_size(uint32_t last_offset, uint32_t last_size, uint32_t alignment) {
        uint32_t size = last_offset + last_size;
        return align_offset(size, alignment);
    }
};
```

## 优化和验证

### AST 优化

```cpp
class ASTOptimizer {
public:
    void optimize(AST& ast) {
        // 常量折叠
        constant_folding(ast);
        
        // 死代码消除
        dead_code_elimination(ast);
        
        // 公共子表达式消除
        common_subexpression_elimination(ast);
    }
    
private:
    void constant_folding(AST& ast) {
        // 折叠常量表达式
        // 2 + 3 -> 5
        // float3(1, 2, 3).x -> 1
    }
};
```

### AST 验证

```cpp
class ASTValidator {
public:
    bool validate(const AST& ast, std::vector<Error>& errors) {
        // 类型检查
        check_types(ast, errors);
        
        // 资源绑定冲突检查
        check_resource_bindings(ast, errors);
        
        // 语义检查
        check_semantics(ast, errors);
        
        return errors.empty();
    }
};
```

## 扩展性设计

### 添加新的类型

```cpp
// 添加半精度浮点支持
class HalfTypeDecl : public ScalarTypeDecl {
public:
    uint32_t size() const override { return 2; }
    uint32_t align() const override { return 2; }
};

// 添加到 AST
ast.HalfType = new HalfTypeDecl();
ast.Half2Type = ast.VectorType(ast.HalfType, 2);
ast.Half3Type = ast.VectorType(ast.HalfType, 3);
ast.Half4Type = ast.VectorType(ast.HalfType, 4);
```

### 添加新的目标语言

```cpp
// GLSL 生成器
class GLSLGenerator : public ICodeGenerator {
    // 实现 GLSL 特定的代码生成逻辑
};

// SPIR-V 生成器
class SPIRVGenerator : public ICodeGenerator {
    // 生成 SPIR-V 字节码
};
```

## 最佳实践

### 设计原则

1. **保持简单**：只添加着色器真正需要的特性
2. **类型安全**：在 AST 层面保证类型正确性
3. **平台无关**：避免在 AST 中引入平台特定概念
4. **可验证性**：设计时考虑验证和优化需求

### 性能考虑

1. **内存池**：使用内存池分配 AST 节点
2. **引用语义**：使用指针避免深拷贝
3. **延迟计算**：布局信息等按需计算
4. **缓存友好**：相关节点在内存中连续存储

## 总结

ShaderAST 是 SakuraEngine 着色器系统的核心，它提供了：

1. **清晰的抽象**：简化的 AST 结构易于理解和操作
2. **强大的类型系统**：GPU 友好的类型表示和布局
3. **灵活的属性机制**：支持各种着色器元数据
4. **高效的代码生成**：针对不同目标优化的生成器
5. **良好的扩展性**：易于添加新特性和目标平台

通过这个设计，SakuraEngine 实现了用 C++ 编写着色器的愿景，同时保持了高性能和跨平台的特性。