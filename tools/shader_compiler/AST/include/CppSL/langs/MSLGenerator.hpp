#pragma once
#include "CppLikeShaderGenerator.hpp"

namespace skr::CppSL::MSL
{
struct MSLGenerator : public CppLikeShaderGenerator
{
protected:
    String GetTypeName(const TypeDecl* type) override;
    void VisitConstructExpr(SourceBuilderNew& sb, const ConstructExpr* expr) override;
    void VisitDeclRef(SourceBuilderNew& sb, const DeclRefExpr* expr) override;
    void RecordBuiltinHeader(SourceBuilderNew& sb, const AST& ast) override;
    void VisitGlobalResource(SourceBuilderNew& sb, const skr::CppSL::VarDecl* var) override;
    void VisitVariable(SourceBuilderNew& sb, const skr::CppSL::VarDecl* var) override;
    void VisitParameter(SourceBuilderNew& sb, const skr::CppSL::FunctionDecl* funcDecl, const skr::CppSL::ParamVarDecl* param) override;
    void VisitField(SourceBuilderNew& sb, const skr::CppSL::TypeDecl* type, const skr::CppSL::FieldDecl* field) override;
    void GenerateFunctionAttributes(SourceBuilderNew& sb, const FunctionDecl* func) override;
    void GenerateKernelWrapper(SourceBuilderNew& sb, const skr::CppSL::FunctionDecl* funcDecl) override;

private:
    void GenerateSRTs(SourceBuilderNew& sb, const AST& ast);
    std::unordered_map<const skr::CppSL::VarDecl*, uint32_t> set_of_vars;
};
}