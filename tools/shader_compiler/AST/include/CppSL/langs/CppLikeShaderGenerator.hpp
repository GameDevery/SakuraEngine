#pragma once
#include "CppSL/AST.hpp"
#include "CppSL/SourceBuilder.hpp"
#include <unordered_map>

namespace skr::CppSL
{

enum struct FunctionStyle
{
    Normal,
    SignatureOnly,
    OutterImplmentation
};

struct CppLikeShaderGenerator
{
public:
    String generate_code(SourceBuilderNew& sb, const AST& ast);

    virtual String GetTypeName(const TypeDecl* type) = 0;
    virtual String GetFunctionName(const FunctionDecl* func);
    virtual void RecordBuiltinHeader(SourceBuilderNew& sb, const AST& ast) = 0;
    virtual void VisitConstructExpr(SourceBuilderNew& sb, const ConstructExpr* expr) = 0;
    virtual void VisitDeclRef(SourceBuilderNew& sb, const DeclRefExpr* expr);
    virtual void VisitAccessExpr(SourceBuilderNew& sb, const AccessExpr* expr);
    virtual void VisitBinaryExpr(SourceBuilderNew& sb, const BinaryExpr* expr);
    virtual void VisitGlobalResource(SourceBuilderNew& sb, const skr::CppSL::VarDecl* var) = 0;
    virtual void VisitVariable(SourceBuilderNew& sb, const skr::CppSL::VarDecl* var) = 0;
    virtual void VisitParameter(SourceBuilderNew& sb, const skr::CppSL::FunctionDecl* funcDecl, const skr::CppSL::ParamVarDecl* param) = 0;
    virtual void VisitField(SourceBuilderNew& sb, const skr::CppSL::TypeDecl* type, const skr::CppSL::FieldDecl* field) = 0;
    virtual void VisitConstructor(SourceBuilderNew& sb, const ConstructorDecl* ctor, FunctionStyle style);
    virtual void GenerateFunctionAttributes(SourceBuilderNew& sb, const FunctionDecl* func);
    virtual void GenerateFunctionSignaturePostfix(SourceBuilderNew& sb, const FunctionDecl* func);
    virtual void GenerateKernelWrapper(SourceBuilderNew& sb, const skr::CppSL::FunctionDecl* funcDecl);
    virtual bool SupportConstructor() const = 0;
    
    virtual void BeforeGenerateGlobalVariables(SourceBuilderNew& sb, const AST& ast);
    virtual void BeforeGenerateFunctionImplementations(SourceBuilderNew& sb, const AST& ast);

protected:
    template <typename T>
    inline static T* FindAttr(std::span<Attr* const> attrs)
    {
        for (auto attr : attrs)
        {
            if (auto found = dynamic_cast<T*>(attr))
                return found;
        }
        return nullptr;
    }

protected:
    String GetQualifiedTypeName(const TypeDecl* type);
    void visitExpr(SourceBuilderNew& sb, const skr::CppSL::Stmt* stmt);
    void visit(SourceBuilderNew& sb, const skr::CppSL::TypeDecl* typeDecl);
    void visit(SourceBuilderNew& sb, const skr::CppSL::FunctionDecl* funcDecl, FunctionStyle style);
    void visit(SourceBuilderNew& sb, const skr::CppSL::VarDecl* varDecl);
    void visit_decl(SourceBuilderNew& sb, const skr::CppSL::Decl* decl);

private:
    void generate_array_helpers(SourceBuilderNew& sb, const AST& ast);
    void generate_namespace_declarations(SourceBuilderNew& sb, const AST& ast);
    void generate_namespace_recursive(SourceBuilderNew& sb, const NamespaceDecl* ns, int indent_level);
    void build_type_namespace_map(const AST& ast);
    std::unordered_map<const TypeDecl*, String> type_namespace_map_;
};
} // namespace skr::CppSL