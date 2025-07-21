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

struct HLSLGenerator
{
public:
    String generate_code(SourceBuilderNew& sb, const AST& ast);

private:
    void generate_array_helpers(SourceBuilderNew& sb, const AST& ast);
    void generate_namespace_declarations(SourceBuilderNew& sb, const AST& ast);
    void generate_namespace_recursive(SourceBuilderNew& sb, const NamespaceDecl* ns, int indent_level);
    void build_type_namespace_map(const AST& ast);
    String GetQualifiedTypeName(const TypeDecl* type);

    void visitExpr(SourceBuilderNew& sb, const skr::CppSL::Stmt* stmt);
    void visit(SourceBuilderNew& sb, const skr::CppSL::TypeDecl* typeDecl);
    void visit(SourceBuilderNew& sb, const skr::CppSL::FunctionDecl* funcDecl, FunctionStyle style);
    void visit(SourceBuilderNew& sb, const skr::CppSL::VarDecl* varDecl);
    void visit_decl(SourceBuilderNew& sb, const skr::CppSL::Decl* decl);
    
    std::unordered_map<const TypeDecl*, String> type_namespace_map_;
};
} // namespace skr::CppSL