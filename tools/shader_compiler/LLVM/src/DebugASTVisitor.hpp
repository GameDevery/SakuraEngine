#pragma once
#include <clang/AST/RecursiveASTVisitor.h>

namespace skr::CppSL {

struct DebugASTVisitor : public clang::RecursiveASTVisitor<DebugASTVisitor>
{
    bool shouldVisitTemplateInstantiations() const { return true; }
    bool VisitStmt(clang::Stmt* x);
    bool VisitRecordDecl(clang::RecordDecl* x);
    bool VisitFunctionDecl(clang::FunctionDecl* x);
    bool VisitParmVarDecl(clang::ParmVarDecl* x);
    bool VisitMemberExpr(clang::MemberExpr* x);
};

} // namespace skr::CppSL