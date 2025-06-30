#include "DebugASTVisitor.hpp"

namespace skr::SSL
{

bool DebugASTVisitor::VisitStmt(clang::Stmt* x)
{
    /*
    if (IF(x))
        b = AddExpr(IF->Then()); c = AddExpr(IF->Else()); Expression.IfThenElse(a, b, c);    
    */
    if (auto cxxDecl = clang::dyn_cast<clang::DeclStmt>(x))
    {
        //for (auto child : cxxDecl->children())
        //    child->dump();

        const auto declGroup = cxxDecl->getDeclGroup();
        for (auto decl : declGroup)
        {
            if (auto var = clang::dyn_cast<clang::VarDecl>(decl))
            {
                /*
                llvm::outs() << var->getName() << " " << var->getDeclName() << "\n";
                clang::QualType t = var->getType();
                if (auto R = t.getCanonicalType()->getAsRecordDecl())
                {
                    R->dump();
                }
                llvm::outs() << "VarDecl: " << var->getName() << "\n";
                */
                return true;
            }
        }
    }

    return true;
}

bool DebugASTVisitor::VisitRecordDecl(clang::RecordDecl* x)
{
    const bool isNormalClass = x->isCompleteDefinition()&& !x->isTemplated();
    const bool isTemplateSpec = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(x) && x->isCompleteDefinition();
    if (isNormalClass || isTemplateSpec)
    {
        // x->dump();
    }
    return true;
}

bool DebugASTVisitor::VisitFunctionDecl(clang::FunctionDecl* x)
{
    return true;
}

bool DebugASTVisitor::VisitParmVarDecl(clang::ParmVarDecl* x)
{
    return true;
}

bool DebugASTVisitor::VisitMemberExpr(clang::MemberExpr* x)
{
    return true;
}

}