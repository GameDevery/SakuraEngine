#pragma once
#include <clang/AST/ASTConsumer.h>
#include <clang/Tooling/Tooling.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include "SSL/AST.hpp"

namespace skr::SSL {

struct CompileFrontendAction : public clang::ASTFrontendAction 
{
public:
    CompileFrontendAction(skr::SSL::AST& AST);
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef InFile) final;
    skr::SSL::AST& AST;
};

struct FunctionStack
{
public:
    const skr::SSL::TypeDecl* methodThisType() const;

    std::map<const clang::Expr*, skr::SSL::Expr*> _lambda_expr_redirects;
    std::map<const clang::VarDecl*, skr::SSL::ParamVarDecl*> _lambda_value_redirects;
    std::vector<skr::SSL::ParamVarDecl*> _captured_params;
    struct CapturedParamInfo
    {
        const clang::LambdaExpr* owner = nullptr;
        const clang::VarDecl* asVar = nullptr;
        const clang::FieldDecl* asCaptureThisField = nullptr;
        bool operator<(const CapturedParamInfo& other) const
        {
            return std::tie(owner, asVar, asCaptureThisField) < std::tie(other.owner, other.asVar, other.asCaptureThisField);
        }
    };
    std::map<skr::SSL::ParamVarDecl*, CapturedParamInfo> _captured_infos;
    std::map<CapturedParamInfo, skr::SSL::ParamVarDecl*> _captured_maps;
    std::set<const clang::LambdaExpr*> _local_lambdas;

private:
    friend class ASTConsumer;
    FunctionStack(const clang::FunctionDecl* func, const ASTConsumer* pASTConsumer)
        : func(func), pASTConsumer(pASTConsumer)
    {

    }

    const clang::FunctionDecl* func = nullptr;
    const ASTConsumer* pASTConsumer = nullptr;
    FunctionStack* prev = nullptr;
};

class ASTConsumer : public clang::ASTConsumer, public clang::RecursiveASTVisitor<ASTConsumer>
{
public:
    friend struct FunctionStack;
    explicit ASTConsumer(skr::SSL::AST& AST);
    virtual ~ASTConsumer() override;

    void HandleTranslationUnit(clang::ASTContext &Context) override;

public:
    // ASTVisitor APIs
    bool shouldVisitTemplateInstantiations() const { return true; }
    bool VisitEnumDecl(const clang::EnumDecl* x);
    bool VisitRecordDecl(const clang::RecordDecl* x);
    bool VisitFunctionDecl(const clang::FunctionDecl* x);
    bool VisitFieldDecl(const clang::FieldDecl* x);
    bool VisitVarDecl(const clang::VarDecl* x);

protected:
    SSL::TypeDecl* TranslateType(clang::QualType type);
    SSL::TypeDecl* TranslateRecordDecl(const clang::RecordDecl* x);
    SSL::TypeDecl* TranslateEnumDecl(const clang::EnumDecl* x);
    SSL::ParamVarDecl* TranslateParam(std::vector<SSL::ParamVarDecl*>& params, skr::SSL::EVariableQualifier qualifier, const skr::SSL::TypeDecl* type, const skr::SSL::Name& name);
    void TranslateParams(std::vector<SSL::ParamVarDecl*>& params, const clang::FunctionDecl* x);
    SSL::FunctionDecl* TranslateFunction(const clang::FunctionDecl* x, llvm::StringRef override_name = {});
    const SSL::TypeDecl* TranslateLambda(const clang::LambdaExpr* x);
    void TranslateLambdaCapturesToParams(const clang::LambdaExpr* x);
    SSL::Stmt* TranslateCall(const clang::Decl* toCall, const clang::Stmt* callExpr);

    Stmt* TranslateStmt(const clang::Stmt *x);
    template <typename T>
    T* TranslateStmt(const clang::Stmt* x);
    
    bool addType(clang::QualType type, const skr::SSL::TypeDecl* decl);
    bool addType(clang::QualType type, skr::SSL::TypeDecl* decl);
    skr::SSL::TypeDecl* getType(clang::QualType type) const;
    bool addVar(const clang::VarDecl* var, skr::SSL::VarDecl* decl);
    skr::SSL::VarDecl* getVar(const clang::VarDecl* var) const;
    bool addFunc(const clang::FunctionDecl* func, skr::SSL::FunctionDecl* decl);
    skr::SSL::FunctionDecl* getFunc(const clang::FunctionDecl* func) const;
    
    clang::ASTContext* pASTContext = nullptr;
    std::map<const clang::TagDecl*, skr::SSL::TypeDecl*> _tag_types;
    std::map<const clang::BuiltinType::Kind, skr::SSL::TypeDecl*> _builtin_types;
    std::map<const clang::VarDecl*, skr::SSL::VarDecl*> _vars;
    std::map<const clang::FunctionDecl*, skr::SSL::FunctionDecl*> _funcs;
    std::map<const clang::EnumConstantDecl*, skr::SSL::GlobalVarDecl*> _enum_constants;

    std::map<const clang::LambdaExpr*, const skr::SSL::TypeDecl*> _lambda_types;
    std::map<const clang::CXXMethodDecl*, const clang::LambdaExpr*> _lambda_methods;
    std::map<const skr::SSL::TypeDecl*, const clang::LambdaExpr*> _lambda_wrappers;

    uint64_t next_lambda_id = 0;
    uint64_t next_template_spec_id = 0;
    AST& AST;
    
protected:
    FunctionStack* root_stack = nullptr;
    FunctionStack* current_stack = nullptr;
    std::map<const clang::FunctionDecl*, FunctionStack*> _stacks;

    FunctionStack* zzNewStack(const clang::FunctionDecl* func);
    void appendStack(const clang::FunctionDecl* func)
    {
        auto prev = current_stack;
        auto _new = zzNewStack(func);
        _new->prev = prev;
        current_stack = _new;
        if (!root_stack) root_stack = _new;
    }
    void popStack()
    {
        if (current_stack)
        {
            current_stack = current_stack->prev;
        }
    }

protected:
    void DumpWithLocation(const clang::Stmt *stmt) const;
    void DumpWithLocation(const clang::Decl *decl) const;
    void ReportFatalError(const std::string& message) const;
    template <typename... Args>
    void ReportFatalError(std::format_string<Args...> _fmt, Args&&... args) const;
    template <typename... Args>
    void ReportFatalError(const clang::Stmt* expr, std::format_string<Args...> _fmt, Args&&... args) const;
    template <typename... Args>
    void ReportFatalError(const clang::Decl* decl, std::format_string<Args...> _fmt, Args&&... args) const;
    
    std::map<std::string, skr::SSL::BinaryOp> _bin_ops;
};
    
} // namespace skr::SSL