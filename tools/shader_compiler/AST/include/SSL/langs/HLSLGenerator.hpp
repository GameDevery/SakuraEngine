#pragma once
#include "SSL/AST.hpp"
#include "SSL/SourceBuilder.hpp"

namespace skr::SSL
{
struct HLSLGenerator
{
public:
    String generate_code(SourceBuilderNew& sb, const AST& ast);

private:
    void visitExpr(SourceBuilderNew& sb, const skr::SSL::Stmt* stmt);
    void visit(SourceBuilderNew& sb, const skr::SSL::TypeDecl* typeDecl);
    void visit(SourceBuilderNew& sb, const skr::SSL::FunctionDecl* funcDecl);
    void visit(SourceBuilderNew& sb, const skr::SSL::VarDecl* varDecl);
};
} // namespace skr::SSL