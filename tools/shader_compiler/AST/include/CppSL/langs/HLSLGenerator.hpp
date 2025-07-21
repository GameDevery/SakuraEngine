#pragma once
#include "CppSL/AST.hpp"
#include "CppSL/SourceBuilder.hpp"

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
    void visitExpr(SourceBuilderNew& sb, const skr::CppSL::Stmt* stmt);
    void visit(SourceBuilderNew& sb, const skr::CppSL::TypeDecl* typeDecl);
    void visit(SourceBuilderNew& sb, const skr::CppSL::FunctionDecl* funcDecl, FunctionStyle style);
    void visit(SourceBuilderNew& sb, const skr::CppSL::VarDecl* varDecl);
    void visit_decl(SourceBuilderNew& sb, const skr::CppSL::Decl* decl);
};
} // namespace skr::CppSL