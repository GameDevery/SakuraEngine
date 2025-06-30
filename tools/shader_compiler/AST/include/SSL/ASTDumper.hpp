#pragma once
#include "AST.hpp"
#include "SSL/SourceBuilder.hpp"

namespace skr::SSL {

struct ASTDumper
{
public:
    void Visit(const skr::SSL::Decl* decl, SourceBuilderNew& sb);
    void Visit(const skr::SSL::Stmt* stmt, SourceBuilderNew& sb);

protected:
    void visit(const skr::SSL::Stmt* stmt, SourceBuilderNew& sb);
    void visit(const skr::SSL::TypeDecl* typeDecl, SourceBuilderNew& sb);
    void visit(const skr::SSL::FieldDecl* fieldDecl, SourceBuilderNew& sb);
    void visit(const skr::SSL::ParamVarDecl* paramDecl, SourceBuilderNew& sb);
    void visit(const skr::SSL::FunctionDecl* funcDecl, SourceBuilderNew& sb);
    void visit(const skr::SSL::VarDecl* varDecl, SourceBuilderNew& sb);
};

} // namespace skr::SSL