#pragma once
#include "Attr.hpp"
#include "Expr.hpp"
#include "Decl.hpp"
#include "Constant.hpp"

#include <set>
#include <map>
#include <unordered_map>
#include <format>

namespace skr::SSL {

// Forward declarations for template system
struct TemplateCallableDecl;
struct SpecializedFunctionDecl;
struct SpecializedMethodDecl;
struct VarConceptDecl;
struct ScalarVarConcept;
struct VectorVarConcept;
struct NumericVarConcept;
struct AnyTypeVarConcept;

#define VEC_TYPES(N) const TypeDecl* N##2Type = nullptr; const TypeDecl* N##3Type = nullptr; const TypeDecl* N##4Type = nullptr;
#define MATRIX_TYPES(N) const TypeDecl* N##2x2Type = nullptr; const TypeDecl* N##3x3Type = nullptr; const TypeDecl* N##4x4Type = nullptr; 

struct AST
{
public:
    AST();
    ~AST();

    AccessExpr* Access(Expr* base, Expr* index);
    BinaryExpr* Binary(BinaryOp op, Expr* left, Expr* right);
    BitwiseCastExpr* BitwiseCast(const TypeDecl* type, Expr* expr);
    BreakStmt* Break();
    CompoundStmt* Block(const std::vector<Stmt*>& statements);
    CallExpr* CallFunction(DeclRefExpr* callee, std::span<Expr*> args);
    CaseStmt* Case(Expr* cond, CompoundStmt* body);
    MethodCallExpr* CallMethod(MemberExpr* callee, std::span<Expr*> args);
    ConditionalExpr* Conditional(Expr* cond, Expr* _then, Expr* _else);
    ConstantExpr* Constant(const IntValue& v);
    ConstantExpr* Constant(const FloatValue& v);
    ConstructExpr* Construct(const TypeDecl* type, std::span<Expr*> args);
    ContinueStmt* Continue();
    CommentStmt* Comment(const String& text);
    DefaultStmt* Default(CompoundStmt* body);
    FieldExpr* Field(Expr* base, const FieldDecl* field);
    ForStmt* For(Stmt* init, Expr* cond, Stmt* inc, CompoundStmt* body);
    IfStmt* If(Expr* cond, CompoundStmt* then_body, CompoundStmt* else_body = nullptr);
    InitListExpr* InitList(std::span<Expr*> exprs);
    ImplicitCastExpr* ImplicitCast(const TypeDecl* type, Expr* expr);
    MethodExpr* Method(DeclRefExpr* base, const MethodDecl* method);
    DeclRefExpr* Ref(const Decl* decl);
    ReturnStmt* Return(Expr* expr);
    StaticCastExpr* StaticCast(const TypeDecl* type, Expr* expr);
    SwizzleExpr* Swizzle(Expr* expr, const TypeDecl* type, uint64_t comps, const uint64_t* seq);
    SwitchStmt* Switch(Expr* cond, std::span<CaseStmt*> cases);
    ThisExpr* This(const TypeDecl* type);
    UnaryExpr* Unary(UnaryOp op, Expr* expr);
    DeclStmt* Variable(EVariableQualifier qualifier, const TypeDecl* type, const Name& name, Expr* initializer = nullptr);
    DeclGroupStmt* DeclGroup(std::span<DeclStmt* const> children);
    WhileStmt* While(Expr* cond, CompoundStmt* body);

    TypeDecl* DeclareStructure(const Name& name, std::span<FieldDecl*> members);
    const TypeDecl* DeclareBuiltinType(const Name& name, uint32_t size, uint32_t alignment = 4, std::vector<FieldDecl*> fields = {});
    const ScalarTypeDecl* DeclareScalarType(const Name& name, uint32_t size, uint32_t alignment = 4);
    GlobalVarDecl* DeclareGlobalConstant(const TypeDecl* type, const Name& name, ConstantExpr* initializer = nullptr);
    GlobalVarDecl* DeclareGlobalResource(const TypeDecl* type, const Name& name);
    FieldDecl* DeclareField(const Name& name, const TypeDecl* type);
    FunctionDecl* DeclareFunction(const Name& name, const TypeDecl* return_type, std::span<const ParamVarDecl* const> params, CompoundStmt* body);
    MethodDecl* DeclareMethod(TypeDecl* owner, const Name& name, const TypeDecl* return_type, std::span<const ParamVarDecl* const> params, CompoundStmt* body);    
    ConstructorDecl* DeclareConstructor(TypeDecl* owner, const Name& name, std::span<const ParamVarDecl* const> params, CompoundStmt* body);
    ParamVarDecl* DeclareParam(EVariableQualifier qualifier, const TypeDecl* type, const Name& name);
    VarConceptDecl* DeclareVarConcept(const Name& name, std::function<bool(EVariableQualifier, const TypeDecl*)> validator);    
    TemplateCallableDecl* DeclareTemplateFunction(const Name& name, const TypeDecl* return_type, std::span<const VarConceptDecl* const> param_concepts);
    TemplateCallableDecl* DeclareTemplateFunction(const Name& name, TemplateCallableDecl::ReturnTypeSpecializer ret_spec, std::span<const VarConceptDecl* const> param_concepts);
    TemplateCallableDecl* DeclareTemplateMethod(TypeDecl* owner, const Name& name, const TypeDecl* return_type, std::span<const VarConceptDecl* const> param_concepts);
    TemplateCallableDecl* DeclareTemplateMethod(TypeDecl* owner, const Name& name, TemplateCallableDecl::ReturnTypeSpecializer ret_spec, std::span<const VarConceptDecl* const> param_concepts);
    
    const TemplateCallableDecl* FindIntrinsic(const char* name) const;
    SpecializedFunctionDecl* SpecializeTemplateFunction(const TemplateCallableDecl* template_decl, std::span<const TypeDecl* const> arg_types, std::span<const EVariableQualifier> arg_qualifiers);
    SpecializedMethodDecl* SpecializeTemplateMethod(const TemplateCallableDecl* template_decl, std::span<const TypeDecl* const> arg_types, std::span<const EVariableQualifier> arg_qualifiers);

    const VectorTypeDecl* VectorType(const TypeDecl* element, uint32_t count);
    const MatrixTypeDecl* MatrixType(const TypeDecl* element, uint32_t n);
    const ArrayTypeDecl* ArrayType(const TypeDecl* element, uint32_t count, ArrayFlags flags);
    const AccelTypeDecl* Accel();
    const RayQueryTypeDecl* RayQuery(RayQueryFlags flags);
    ByteBufferTypeDecl* ByteBuffer(BufferFlags flags);
    StructuredBufferTypeDecl* StructuredBuffer(const TypeDecl* element, BufferFlags flags);
    Texture2DTypeDecl* Texture2D(const TypeDecl* element, TextureFlags flags);
    Texture3DTypeDecl* Texture3D(const TypeDecl* element, TextureFlags flags);

    template <typename ATTR, typename... Args>
    inline ATTR* DeclareAttr(Args&&... args) {
        auto attr = new ATTR(std::forward<Args>(args)...);
        _attrs.emplace_back(attr);
        return attr;
    }

    inline BinaryExpr* Add(Expr* left, Expr* right) { return Binary(BinaryOp::ADD, left, right); }
    inline BinaryExpr* Sub(Expr* left, Expr* right) { return Binary(BinaryOp::SUB, left, right); }
    inline BinaryExpr* Mul(Expr* left, Expr* right) { return Binary(BinaryOp::MUL, left, right); }
    inline BinaryExpr* Div(Expr* left, Expr* right) { return Binary(BinaryOp::DIV, left, right); }
    inline BinaryExpr* Mod(Expr* left, Expr* right) { return Binary(BinaryOp::MOD, left, right); }
    inline BinaryExpr* BitAnd(Expr* left, Expr* right) { return Binary(BinaryOp::BIT_AND, left, right); }
    inline BinaryExpr* BitOr(Expr* left, Expr* right) { return Binary(BinaryOp::BIT_OR, left, right); }
    inline BinaryExpr* BitXor(Expr* left, Expr* right) { return Binary(BinaryOp::BIT_XOR, left, right); }
    inline BinaryExpr* Shl(Expr* left, Expr* right) { return Binary(BinaryOp::SHL, left, right); }
    inline BinaryExpr* Shr(Expr* left, Expr* right) { return Binary(BinaryOp::SHR, left, right); }
    inline BinaryExpr* And(Expr* left, Expr* right) { return Binary(BinaryOp::AND, left, right); }
    inline BinaryExpr* Or(Expr* left, Expr* right) { return Binary(BinaryOp::OR, left, right); }
    inline BinaryExpr* Less(Expr* left, Expr* right) { return Binary(BinaryOp::LESS, left, right); }
    inline BinaryExpr* Greater(Expr* left, Expr* right) { return Binary(BinaryOp::GREATER, left, right); }
    inline BinaryExpr* LessEqual(Expr* left, Expr* right) { return Binary(BinaryOp::LESS_EQUAL, left, right); }
    inline BinaryExpr* GreaterEqual(Expr* left, Expr* right) { return Binary(BinaryOp::GREATER_EQUAL, left, right); }
    inline BinaryExpr* Equal(Expr* left, Expr* right) { return Binary(BinaryOp::EQUAL, left, right); }
    inline BinaryExpr* NotEqual(Expr* left, Expr* right) { return Binary(BinaryOp::NOT_EQUAL, left, right); }
    inline BinaryExpr* Assign(Expr* left, Expr* right) { return Binary(BinaryOp::ASSIGN, left, right); }
    inline BinaryExpr* AddAssign(Expr* left, Expr* right) { return Binary(BinaryOp::ADD_ASSIGN, left, right); }
    inline BinaryExpr* SubAssign(Expr* left, Expr* right) { return Binary(BinaryOp::SUB_ASSIGN, left, right); }
    inline BinaryExpr* MulAssign(Expr* left, Expr* right) { return Binary(BinaryOp::MUL_ASSIGN, left, right); }
    inline BinaryExpr* DivAssign(Expr* left, Expr* right) { return Binary(BinaryOp::DIV_ASSIGN, left, right); }
    inline BinaryExpr* ModAssign(Expr* left, Expr* right) { return Binary(BinaryOp::MOD_ASSIGN, left, right); }

    const TypeDecl* GetType(const Name& name) const;

    std::span<Decl* const> decls() const { return _decls; }
    std::span<Stmt* const> stmts() const { return _stmts; }
    std::span<TypeDecl* const> types() const { return _types; }
    std::span<GlobalVarDecl* const> global_vars() const { return _globals; }
    std::span<FunctionDecl* const> funcs() const { return _funcs; }

    String dump() const;

private:
    template <typename... Args>
    [[noreturn]] void ReportFatalError(std::wformat_string<Args...> fmt, Args&&... args) const;
    [[noreturn]] void ReportFatalError(const String& message) const;
    void ReservedWordsCheck(const Name& name) const;

    const VectorTypeDecl* DeclareVectorType(const TypeDecl* element, uint32_t count, uint32_t alignment);
    const MatrixTypeDecl* DeclareMatrixType(const TypeDecl* element, uint32_t n, uint32_t alignment);

    std::vector<Decl*> _decls;
    std::vector<Stmt*> _stmts;
    AccelTypeDecl* _accel = nullptr;
    std::map<RayQueryFlags, RayQueryTypeDecl*> _ray_queries;
    std::map<std::pair<const TypeDecl*, uint32_t>, VectorTypeDecl*> _vecs;
    std::map<std::pair<const TypeDecl*, std::array<uint32_t, 2>>, MatrixTypeDecl*> _matrices;
    std::map<std::pair<const TypeDecl*, uint32_t>, ArrayTypeDecl*> _arrs;
    std::map<std::pair<const TypeDecl*, BufferFlags>, BufferTypeDecl*> _buffers;
    std::map<std::pair<const TypeDecl*, TextureFlags>, Texture2DTypeDecl*> _texture2ds;
    std::map<std::pair<const TypeDecl*, TextureFlags>, Texture3DTypeDecl*> _texture3ds;
    std::vector<TypeDecl*> _types;
    std::vector<GlobalVarDecl*> _globals;
    std::vector<FunctionDecl*> _funcs;
    std::vector<MethodDecl*> _methods;
    std::vector<ConstructorDecl*> _ctors;
    std::vector<Attr*> _attrs;
    
    // Template and specialized declarations
    std::map<std::string, TemplateCallableDecl*> _intrinstics;

public:
    void DeclareIntrinstics();

    const TypeDecl* VoidType = nullptr;
    const TypeDecl* BoolType = nullptr;
    VEC_TYPES(Bool);
    MATRIX_TYPES(Bool);

    const TypeDecl* HalfType = nullptr;
    VEC_TYPES(Half);

    const TypeDecl* FloatType = nullptr;
    VEC_TYPES(Float);
    MATRIX_TYPES(Float);

    const TypeDecl* UIntType = nullptr;
    VEC_TYPES(UInt);
    MATRIX_TYPES(UInt);
    
    const TypeDecl* IntType = nullptr;
    VEC_TYPES(Int);
    MATRIX_TYPES(Int);
    
    const TypeDecl* DoubleType = nullptr;
    const TypeDecl* U64Type = nullptr;
    const TypeDecl* I64Type = nullptr;
};

#undef VEC_TYPES
#undef MATRIX_TYPES

} // namespace skr::SSL