#include "CppSL/langs/CppLikeShaderGenerator.hpp"
#include <functional>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace skr::CppSL
{

// Forward declaration for CppLikeShaderGenerator member function
String CppLikeShaderGenerator::GetQualifiedTypeName(const TypeDecl* type)
{
    // Check if this type has a namespace mapping
    auto NonQualified = GetTypeName(type);
    auto it = type_namespace_map_.find(type);
    if (it != type_namespace_map_.end() && !it->second.empty())
    {
        return it->second + L"::" + NonQualified;
    }
    return NonQualified;
}

String CppLikeShaderGenerator::GetQualifiedFunctionName(const FunctionDecl* func)
{
    // Check if this function has a namespace mapping
    auto NonQualified = GetFunctionName(func);  // Use virtual GetFunctionName instead of func->name()
    auto it = function_namespace_map_.find(func);
    if (it != function_namespace_map_.end() && !it->second.empty())
    {
        return it->second + L"::" + NonQualified;
    }
    return NonQualified;
}

void CppLikeShaderGenerator::visitStmt(SourceBuilderNew& sb, const skr::CppSL::Stmt* stmt)
{
    using namespace skr::CppSL;

    bool isStatement = false;
    if (auto parent = stmt->parent())
    {
        isStatement |= dynamic_cast<const CompoundStmt*>(parent) != nullptr;
    }
    isStatement &= !dynamic_cast<const IfStmt*>(stmt);
    isStatement &= !dynamic_cast<const ForStmt*>(stmt);
    isStatement &= !dynamic_cast<const CompoundStmt*>(stmt);

    GenerateStmtAttributes(sb, stmt);

    if (auto binary = dynamic_cast<const BinaryExpr*>(stmt))
    {
        sb.append(L"(");
        VisitBinaryExpr(sb, binary);
        sb.append(L")");
    }
    else if (auto bitwiseCast = dynamic_cast<const BitwiseCastExpr*>(stmt))
    {
        auto _type = bitwiseCast->type();
        sb.append(L"bit_cast<" + GetQualifiedTypeName(_type) + L">(");
        ;
        visitStmt(sb, bitwiseCast->expr());
        sb.append(L")");
    }
    else if (auto breakStmt = dynamic_cast<const BreakStmt*>(stmt))
    {
        sb.append(L"break");
    }
    else if (auto block = dynamic_cast<const CompoundStmt*>(stmt))
    {
        sb.endline(L'{');
        sb.indent([&]() {
            for (auto expr : block->children())
            {
                visitStmt(sb, expr);
            }
        });
        sb.append(L"}");
    }
    else if (auto condExpr = dynamic_cast<const ConditionalExpr*>(stmt))
    {
        visitStmt(sb, condExpr->cond());
        sb.append(L" ? ");
        visitStmt(sb, condExpr->then_expr());
        sb.append(L" : ");
        visitStmt(sb, condExpr->else_expr());
    }
    else if (auto callExpr = dynamic_cast<const CallExpr*>(stmt))
    {
        auto callee = callExpr->callee();
        if (auto callee_decl = dynamic_cast<const FunctionDecl*>(callee->decl()))
        {
            auto func_name = GetQualifiedFunctionName(callee_decl);
            
            // TODO: Implement REAL TEMPLATE CALL (CallWithTypeArgs)
            const bool ByteBufferReadTyped = callee_decl->name() == L"byte_buffer_read";
            const bool WaveReadLaneFirst = callee_decl->name() == L"WaveReadLaneFirst";
            if (ByteBufferReadTyped || WaveReadLaneFirst)
            {
                func_name = func_name + L"<" + GetQualifiedTypeName(callee_decl->return_type()) + L">";
            }
            
            sb.append(func_name);
            sb.append(L"(");
            for (size_t i = 0; i < callExpr->args().size(); i++)
            {
                auto arg = callExpr->args()[i];
                if (i > 0)
                    sb.append(L", ");
                visitStmt(sb, arg);
            }
            sb.append(L")");
        }
        else
        {
            sb.append(L"unknown function call!");
        }
    }
    else if (auto caseStmt = dynamic_cast<const CaseStmt*>(stmt))
    {
        if (caseStmt->cond())
        {
            sb.append(L"case ");
            visitStmt(sb, caseStmt->cond());
            sb.append(L":");
        }
        else
        {
            sb.append(L"default:");
        }
        visitStmt(sb, caseStmt->body());
        sb.append(L";");
    }
    else if (auto methodCall = dynamic_cast<const MethodCallExpr*>(stmt))
    {
        auto callee = methodCall->callee();
        auto method = dynamic_cast<const MethodDecl*>(callee->member_decl());
        auto type = method->owner_type();
        if (auto as_buffer = dynamic_cast<const BufferTypeDecl*>(type) && method->name() == L"Store")
        {
            visitStmt(sb, callee->owner());
            sb.append(L"[");
            visitStmt(sb, methodCall->args()[0]);
            sb.append(L"] = ");
            visitStmt(sb, methodCall->args()[1]);
        }
        else
        {
            visitStmt(sb, callee);

            sb.append(L"(");
            for (size_t i = 0; i < methodCall->args().size(); i++)
            {
                auto arg = methodCall->args()[i];
                if (i > 0)
                    sb.append(L", ");
                visitStmt(sb, arg);
            }
            sb.append(L")");
        }
    }
    else if (auto constant = dynamic_cast<const ConstantExpr*>(stmt))
    {
        if (auto i = std::get_if<IntValue>(&constant->value))
        {
            if (i->is_signed())
                sb.append(std::to_wstring(i->value<int64_t>().get()));
            else
                sb.append(std::to_wstring(i->value<uint64_t>().get()));
        }
        else if (auto f = std::get_if<FloatValue>(&constant->value))
        {
            // Use hexfloat for exact precision
            double value = f->ieee.value();
            
            // Format as hexfloat
            std::wostringstream hexstream;
            hexstream << std::hexfloat << value;
            sb.append(hexstream.str());
            
            // Add readable comment - avoid scientific notation for readability
            std::wostringstream decstream;
            decstream << std::fixed; // Use fixed-point notation
            
            // Choose appropriate precision based on value magnitude
            double abs_value = std::abs(value);
            if (abs_value == 0.0) {
                decstream << std::setprecision(1) << value;
            }
            else if (abs_value >= 1e6 || abs_value < 1e-6) {
                // For very large or very small numbers, use scientific notation
                decstream << std::scientific << std::setprecision(6) << value;
            }
            else if (abs_value >= 1.0) {
                // For numbers >= 1, show up to 6 decimal places
                decstream << std::setprecision(6) << value;
            }
            else {
                // For small numbers, show more precision
                decstream << std::setprecision(9) << value;
            }
            
            std::wstring decstr = decstream.str();
            
            // Remove trailing zeros after decimal point
            size_t dot_pos = decstr.find(L'.');
            if (dot_pos != std::wstring::npos) {
                size_t last_nonzero = decstr.find_last_not_of(L'0');
                if (last_nonzero != std::wstring::npos && last_nonzero > dot_pos) {
                    decstr.erase(last_nonzero + 1);
                }
                // Remove decimal point if no fractional part remains
                if (decstr.back() == L'.') {
                    decstr.pop_back();
                }
            }
            
            sb.append(L"/*");
            sb.append(decstr);
            sb.append(L"*/");
        }
        else
        {
            sb.append(L"UnknownConstant: ");
        }
    }
    else if (auto ctorExpr = dynamic_cast<const ConstructExpr*>(stmt))
    {
        VisitConstructExpr(sb, ctorExpr);
    }
    else if (auto continueStmt = dynamic_cast<const ContinueStmt*>(stmt))
    {
        sb.append(L"continue;");
    }
    else if (auto defaultStmt = dynamic_cast<const DefaultStmt*>(stmt))
    {
        sb.append(L"default:");
        visitStmt(sb, defaultStmt->children()[0]);
        sb.append(L";");
    }
    else if (auto member = dynamic_cast<const MemberExpr*>(stmt))
    {
        auto owner = member->owner();
        if (auto _member = dynamic_cast<const NamedDecl*>(member->member_decl()))
        {
            if (auto fromThis = dynamic_cast<const ThisExpr*>(owner))
                sb.append(L"/*this.*/" + _member->name());
            else
            {
                visitStmt(sb, owner);
                sb.append(L"." + _member->name());
            }
        }
        else
        {
            sb.append(L"UnknownMember");
        }
    }
    else if (auto forStmt = dynamic_cast<const ForStmt*>(stmt))
    {
        sb.append(L"for (");
        if (forStmt->init())
            visitStmt(sb, forStmt->init());
        sb.append(L"; ");

        if (forStmt->cond())
            visitStmt(sb, forStmt->cond());
        sb.append(L"; ");

        if (forStmt->inc())
            visitStmt(sb, forStmt->inc());
        sb.append(L") ");

        visitStmt(sb, forStmt->body());
        sb.endline();
    }
    else if (auto ifStmt = dynamic_cast<const IfStmt*>(stmt))
    {
        sb.append(L"if (");
        visitStmt(sb, ifStmt->cond());
        sb.append(L")");
        sb.endline();
        visitStmt(sb, ifStmt->then_body());

        if (ifStmt->else_body())
        {
            sb.endline();
            sb.append(L"else");
            sb.endline();
            visitStmt(sb, ifStmt->else_body());
        }
        sb.endline();
    }
    else if (auto initList = dynamic_cast<const InitListExpr*>(stmt))
    {
        sb.append(L"{ ");
        for (size_t i = 0; i < initList->children().size(); i++)
        {
            auto expr = initList->children()[i];
            if (i > 0)
                sb.append(L", ");
            visitStmt(sb, expr);
        }
        sb.append(L" }");
    }
    else if (auto implicitCast = dynamic_cast<const ImplicitCastExpr*>(stmt))
    {
        visitStmt(sb, implicitCast->expr());
    }
    else if (auto declRef = dynamic_cast<const DeclRefExpr*>(stmt))
    {
        VisitDeclRef(sb, declRef);
    }
    else if (auto returnStmt = dynamic_cast<const ReturnStmt*>(stmt))
    {
        sb.append(L"return");
        if (returnStmt->value())
        {
            sb.append(L" ");
            visitStmt(sb, returnStmt->value());
        }
    }
    else if (auto staticCast = dynamic_cast<const StaticCastExpr*>(stmt))
    {
        sb.append(L"((" + GetQualifiedTypeName(staticCast->type()) + L")");
        ;
        visitStmt(sb, staticCast->expr());
        sb.append(L")");
    }
    else if (auto switchStmt = dynamic_cast<const SwitchStmt*>(stmt))
    {
        sb.append(L"switch (");
        visitStmt(sb, switchStmt->cond());
        sb.append(L") {");
        sb.endline();
        sb.indent([&]() {
            for (auto case_stmt : switchStmt->cases())
            {
                visitStmt(sb, case_stmt);
                sb.endline();
            }
        });
        sb.append(L"}");
    }
    else if (auto unary = dynamic_cast<const UnaryExpr*>(stmt))
    {
        sb.append(L"(");

        {
            String op_name = L"";
            bool post = false;
            switch (unary->op())
            {
            case UnaryOp::PLUS:
                op_name = L"+";
                break;
            case UnaryOp::MINUS:
                op_name = L"-";
                break;
            case UnaryOp::NOT:
                op_name = L"!";
                break;
            case UnaryOp::BIT_NOT:
                op_name = L"~";
                break;
            case UnaryOp::PRE_INC:
                op_name = L"++";
                break;
            case UnaryOp::PRE_DEC:
                op_name = L"--";
                break;
            case UnaryOp::POST_INC:
                op_name = L"++";
                post = true;
                break;
            case UnaryOp::POST_DEC:
                op_name = L"--";
                post = true;
                break;
            default:
                assert(false && "Unsupported unary operation");
            }
            if (!post)
                sb.append(op_name);

            visitStmt(sb, unary->expr());

            if (post)
                sb.append(op_name);
        }

        sb.append(L")");
    }
    else if (auto declStmt = dynamic_cast<const DeclStmt*>(stmt))
    {
        if (auto decl = dynamic_cast<const VarDecl*>(declStmt->decl()))
        {
            visit(sb, decl);
        }
    }
    else if (auto declGroupStmt = dynamic_cast<const DeclGroupStmt*>(stmt))
    {
        for (auto decl : declGroupStmt->children())
        {
            visitStmt(sb, dynamic_cast<const DeclStmt*>(decl));
            sb.append(L";");
        }
    }
    else if (auto whileStmt = dynamic_cast<const WhileStmt*>(stmt))
    {
        sb.append(L"while (");
        visitStmt(sb, whileStmt->cond());
        sb.append(L") ");
        visitStmt(sb, whileStmt->body());
    }
    else if (auto access = dynamic_cast<const AccessExpr*>(stmt))
    {
        VisitAccessExpr(sb, access);
    }
    else if (auto swizzle = dynamic_cast<const SwizzleExpr*>(stmt))
    {
        visitStmt(sb, swizzle->expr());
        sb.append(L".");
        for (auto comp : swizzle->seq())
        {
            skr::CppSL::String comps[4] = { L"x", L"y", L"z", L"w" };
            sb.append(comps[comp]);
        }
    }
    else if (auto thisExpr = dynamic_cast<const ThisExpr*>(stmt))
    {
        sb.append(L"/*this->*/");
    }
    else if (auto commentStmt = dynamic_cast<const CommentStmt*>(stmt))
    {
        sb.append(L"// " + commentStmt->text());
    }
    else
    {
        sb.append(L"[UnknownExpr]");
    }

    if (isStatement)
        sb.endline(L';');
}

void CppLikeShaderGenerator::visit(SourceBuilderNew& sb, const skr::CppSL::TypeDecl* typeDecl)
{
    using namespace skr::CppSL;
    const bool DUMP_BUILTIN_TYPES = false;
    if (!typeDecl->is_builtin())
    {
        sb.append(L"struct " + GetQualifiedTypeName(typeDecl));
        sb.endline(L'{');
        sb.indent([&] {
            for (auto field : typeDecl->fields())
            {
                VisitField(sb, typeDecl, field);
            }
            for (auto method : typeDecl->methods())
            {
                visit(sb, method, FunctionStyle::SignatureOnly);
            }
            for (auto ctor : typeDecl->ctors())
            {
                VisitConstructor(sb, ctor, FunctionStyle::SignatureOnly);
            }
        });
        sb.append(L"}");
        sb.endline(L';');
        sb.endline();
        sb.append_line();
    }
    else if (DUMP_BUILTIN_TYPES)
    {
        sb.append(L"//builtin type: ");
        sb.append(GetTypeName(typeDecl));
        sb.append(L", size: " + std::to_wstring(typeDecl->size()));
        sb.append(L", align: " + std::to_wstring(typeDecl->alignment()));
        sb.endline();
    }
}

void CppLikeShaderGenerator::VisitDeclRef(SourceBuilderNew& sb, const DeclRefExpr* declRef)
{
    if (auto decl = dynamic_cast<const VarDecl*>(declRef->decl()))
        sb.append(decl->name());
}

void CppLikeShaderGenerator::VisitAccessExpr(SourceBuilderNew& sb, const AccessExpr* expr)
{
    auto to_access = dynamic_cast<const Expr*>(expr->children()[0]);
    auto index = dynamic_cast<const Expr*>(expr->children()[1]);
    visitStmt(sb, to_access);
    sb.append(L"[");
    visitStmt(sb, index);
    sb.append(L"]");
}

void CppLikeShaderGenerator::VisitBinaryExpr(SourceBuilderNew& sb, const BinaryExpr* binary)
{
    auto ltype = binary->left()->type();
    auto rtype = binary->right()->type();
    {
        visitStmt(sb, binary->left());
        auto op = binary->op();
        String op_name = L"";
        switch (op)
        {
        case BinaryOp::ADD:
            op_name = L" + ";
            break;
        case BinaryOp::SUB:
            op_name = L" - ";
            break;
        case BinaryOp::MUL:
            op_name = L" * ";
            break;
        case BinaryOp::DIV:
            op_name = L" / ";
            break;
        case BinaryOp::MOD:
            op_name = L" % ";
            break;

        case BinaryOp::BIT_AND:
            op_name = L" & ";
            break;
        case BinaryOp::BIT_OR:
            op_name = L" | ";
            break;
        case BinaryOp::BIT_XOR:
            op_name = L" ^ ";
            break;
        case BinaryOp::SHL:
            op_name = L" << ";
            break;
        case BinaryOp::SHR:
            op_name = L" >> ";
            break;
        case BinaryOp::AND:
            op_name = L" && ";
            break;
        case BinaryOp::OR:
            op_name = L" || ";
            break;

        case BinaryOp::LESS:
            op_name = L" < ";
            break;
        case BinaryOp::GREATER:
            op_name = L" > ";
            break;
        case BinaryOp::LESS_EQUAL:
            op_name = L" <= ";
            break;
        case BinaryOp::GREATER_EQUAL:
            op_name = L" >= ";
            break;
        case BinaryOp::EQUAL:
            op_name = L" == ";
            break;
        case BinaryOp::NOT_EQUAL:
            op_name = L" != ";
            break;

        case BinaryOp::ASSIGN:
            op_name = L" = ";
            break;
        case BinaryOp::ADD_ASSIGN:
            op_name = L" += ";
            break;
        case BinaryOp::SUB_ASSIGN:
            op_name = L" -= ";
            break;
        case BinaryOp::MUL_ASSIGN:
            op_name = L" *= ";
            break;
        case BinaryOp::DIV_ASSIGN:
            op_name = L" /= ";
            break;
        case BinaryOp::MOD_ASSIGN:
            op_name = L" %= ";
            break;
        case BinaryOp::BIT_OR_ASSIGN:
            op_name = L" |= ";
            break;
        case BinaryOp::BIT_XOR_ASSIGN:
            op_name = L" ^= ";
            break;
        case BinaryOp::SHL_ASSIGN:
            op_name = L" <<= ";
            break;
        default:
            assert(false && "Unsupported binary operation");
        }
        sb.append(op_name);
        visitStmt(sb, binary->right());
    }
}

void CppLikeShaderGenerator::VisitConstructor(SourceBuilderNew& sb, const ConstructorDecl* ctor, FunctionStyle style)
{
    visit(sb, ctor, style);
}

void CppLikeShaderGenerator::visit(SourceBuilderNew& sb, const skr::CppSL::FunctionDecl* funcDecl, FunctionStyle style)
{
    using namespace skr::CppSL;
    auto AsMethod = dynamic_cast<const MethodDecl*>(funcDecl);
    auto AsCtor = dynamic_cast<const ConstructorDecl*>(AsMethod);
    const auto SupportCtor = SupportConstructor();
    if (auto body = funcDecl->body())
    {
        const StageAttr* StageEntry = FindAttr<StageAttr>(funcDecl->attrs());
        std::vector<const ParamVarDecl*> params;
        params.reserve(funcDecl->parameters().size());
        for (auto param : funcDecl->parameters())
        {
            params.emplace_back(param);
        }

        if (StageEntry)
        {
            // extract bindings from signature
            for (size_t i = 0; i < funcDecl->parameters().size(); i++)
            {
                auto param = funcDecl->parameters()[i];
                if (auto asResource = dynamic_cast<const ResourceTypeDecl*>(&param->type()))
                {
                    VisitShaderResource(sb, param);
                    params.erase(std::find(params.begin(), params.end(), param));
                }
            }
        }
        GenerateFunctionAttributes(sb, funcDecl);

        // generate signature
        auto functionName = GetFunctionName(funcDecl);
        if (SupportCtor && AsCtor)
            functionName = GetTypeName(AsMethod->owner_type());
        if ((style == FunctionStyle::OutterImplmentation) && AsMethod)
            functionName = GetQualifiedTypeName(AsMethod->owner_type()) + L"::" + functionName;
        else if (style == FunctionStyle::OutterImplmentation)
            functionName = GetQualifiedFunctionName(funcDecl);

        if (SupportCtor && AsCtor)
            sb.append(functionName + L"(");
        else
            sb.append(GetQualifiedTypeName(funcDecl->return_type()) + L" " + functionName + L"(");
        for (size_t i = 0; i < params.size(); i++)
        {
            if (i > 0)
                sb.append(L", ");

            VisitParameter(sb, funcDecl, params[i]);
        }
        sb.append(L")");
        GenerateFunctionSignaturePostfix(sb, funcDecl);

        // generate body
        if (style == FunctionStyle::SignatureOnly)
        {
            sb.append(L";");
            sb.endline();
        }
        else
        {
            // Generate member initializer list for constructors that support it
            if (SupportCtor && AsCtor && !AsCtor->member_inits().empty())
            {
                sb.endline();
                sb.append(L"    : ");
                bool first = true;
                for (const auto& init : AsCtor->member_inits())
                {
                    if (!first)
                    {
                        sb.append(L", ");
                    }
                    first = false;
                    sb.append(init.field->name());
                    sb.append(L"(");
                    visitStmt(sb, init.init_expr);
                    sb.append(L")");
                }
            }
            
            sb.endline();
            
            // For languages that don't support constructors, emit initializers as assignments in body
            if (!SupportCtor && AsCtor && !AsCtor->member_inits().empty())
            {
                sb.append(L"{");
                sb.endline();
                sb.indent([&] {
                    // First emit the member initializers as assignments
                    for (const auto& init : AsCtor->member_inits())
                    {
                        sb.append(L"(/*this.*/");
                        sb.append(init.field->name());
                        sb.append(L" = ");
                        visitStmt(sb, init.init_expr);
                        sb.append(L");");
                        sb.endline();
                    }
                    
                    // Then emit the original body if it exists
                    if (auto compound = dynamic_cast<const CompoundStmt*>(funcDecl->body()))
                    {
                        for (auto stmt : compound->children())
                        {
                            visitStmt(sb, stmt);
                            sb.endline();
                        }
                    }
                });
                sb.append(L"}");
            }
            else
            {
                visitStmt(sb, funcDecl->body());
            }
            sb.endline();

            if (StageEntry)
                GenerateKernelWrapper(sb, funcDecl);
        }
    }
}

String CppLikeShaderGenerator::GetFunctionName(const FunctionDecl* func)
{
    return func->name();
}

void CppLikeShaderGenerator::GenerateStmtAttributes(SourceBuilderNew& sb, const skr::CppSL::Stmt* stmt)
{

}

void CppLikeShaderGenerator::GenerateFunctionAttributes(SourceBuilderNew& sb, const FunctionDecl* funcDecl)
{

}

void CppLikeShaderGenerator::GenerateFunctionSignaturePostfix(SourceBuilderNew& sb, const FunctionDecl* func)
{

}

void CppLikeShaderGenerator::GenerateKernelWrapper(SourceBuilderNew& sb, const skr::CppSL::FunctionDecl* funcDecl)
{
}

void CppLikeShaderGenerator::visit(SourceBuilderNew& sb, const skr::CppSL::VarDecl* varDecl)
{
    const auto isGlobal = dynamic_cast<const skr::CppSL::GlobalVarDecl*>(varDecl);
    if (varDecl->type().is_resource())
    {
        VisitShaderResource(sb, varDecl);
    }
    else
    {
        VisitVariable(sb, varDecl);
    }
}

void CppLikeShaderGenerator::visit_decl(SourceBuilderNew& sb, const skr::CppSL::Decl* decl)
{
    if (auto asType = dynamic_cast<const TypeDecl*>(decl))
    {
        visit(sb, asType);
        sb.endline();
    }
    else if (auto asFunc = dynamic_cast<const FunctionDecl*>(decl))
    {
        visit(sb, asFunc, FunctionStyle::OutterImplmentation);
    }
    else if (auto asGlobalVar = dynamic_cast<const GlobalVarDecl*>(decl))
    {
        visit(sb, asGlobalVar);
        sb.endline(L';');
    }
}

void CppLikeShaderGenerator::BeforeGenerateGlobalVariables(SourceBuilderNew& sb, const AST& ast)
{

}

void CppLikeShaderGenerator::BeforeGenerateFunctionImplementations(SourceBuilderNew& sb, const AST& ast)
{

}

String CppLikeShaderGenerator::generate_code(SourceBuilderNew& sb, const AST& ast)
{
    using namespace skr::CppSL;

    RecordBuiltinHeader(sb, ast);

    build_type_namespace_map(ast);
    generate_namespace_declarations(sb, ast);

    for (const auto& decl : ast.types())
        visit_decl(sb, decl);

    BeforeGenerateGlobalVariables(sb, ast);
    for (const auto& decl : ast.global_vars())
        visit_decl(sb, decl);

    BeforeGenerateFunctionImplementations(sb, ast);
    for (const auto& decl : ast.funcs())
        visit_decl(sb, decl);

    return sb.build(SourceBuilderNew::line_builder_code);
}

void CppLikeShaderGenerator::generate_namespace_declarations(SourceBuilderNew& sb, const AST& ast)
{
    // Generate forward declarations for global types (types without namespace)
    bool has_global_types = false;
    for (const auto& type : ast.types())
    {
        // Check if this type is not in any namespace
        bool is_global = true;
        for (const auto& ns : ast.namespaces())
        {
            // Check recursively in all namespaces
            std::function<bool(const NamespaceDecl*)> contains_type = [&](const NamespaceDecl* ns) -> bool {
                for (const auto& ns_type : ns->types())
                {
                    if (ns_type == type)
                        return true;
                }
                for (const auto& nested : ns->nested())
                {
                    if (contains_type(nested))
                        return true;
                }
                return false;
            };

            if (contains_type(ns))
            {
                is_global = false;
                break;
            }
        }

        // Generate forward declaration for global types
        if (is_global && !type->is_builtin())
        {
            sb.append(L"struct " + type->name() + L"; ");
            has_global_types = true;
        }
    }

    if (has_global_types)
        sb.endline();

    // Generate namespace forward declarations for HLSL
    // Only generate root namespaces (those without parents) to avoid duplicates
    bool has_output = false;
    for (const auto& ns : ast.namespaces())
    {
        if (ns->parent() == nullptr) // Only process root namespaces
        {
            generate_namespace_recursive(sb, ns, 0, ForwardDeclareType::Type);
            has_output = true;
        }
    }
    if (has_output)
        sb.endline();

    has_output = false;
    for (const auto& ns : ast.namespaces())
    {
        if (ns->parent() == nullptr) // Only process root namespaces
        {
            generate_namespace_recursive(sb, ns, 0, ForwardDeclareType::Function);
            has_output = true;
        }
    }

    if (has_output)
        sb.endline();
}

void CppLikeShaderGenerator::generate_namespace_recursive(SourceBuilderNew& sb, const NamespaceDecl* ns, int indent_level, ForwardDeclareType type)
{
    // Skip empty namespaces
    if (!ns->has_content())
        return;

    // Generate namespace opening (compact style)
    sb.append(L"namespace " + ns->name() + L" { ");

    // Generate forward declarations for types in this namespace
    bool has_declarations = false;
    if (type == ForwardDeclareType::Type)
    {
        for (const auto& type : ns->types())
        {
            if (!type->is_builtin())
            {
                sb.append(L"struct " + type->name() + L"; ");
                has_declarations = true;
            }
        }
    }
    else if (type == ForwardDeclareType::Function)
    {
        // Generate forward declarations for functions in this namespace
        for (const auto& func : ns->functions())
        {
            visit(sb, func, FunctionStyle::SignatureOnly);
            sb.append(L"; ");
            has_declarations = true;
        }
    }

    // Recursively generate nested namespaces
    for (const auto& nested : ns->nested())
    {
        generate_namespace_recursive(sb, nested, indent_level + 1, type);
    }

    // Generate namespace closing
    sb.append(L"}");
    if (indent_level == 0) // Only add newline for root namespaces
        sb.endline();
    else
        sb.append(L" ");
}

void CppLikeShaderGenerator::build_type_namespace_map(const AST& ast)
{
    // Clear the maps first
    type_namespace_map_.clear();
    function_namespace_map_.clear();

    // Helper function to build namespace path
    std::function<String(const NamespaceDecl*)> build_namespace_path = [&](const NamespaceDecl* ns) -> String {
        if (!ns || !ns->parent())
        {
            return ns ? ns->name() : L"";
        }
        String parent_path = build_namespace_path(ns->parent());
        return parent_path.empty() ? ns->name() : parent_path + L"::" + ns->name();
    };

    // Recursively map all types in namespaces
    std::function<void(const NamespaceDecl*)> map_namespace_types = [&](const NamespaceDecl* ns) {
        String namespace_path = build_namespace_path(ns);

        // Map all types in this namespace
        for (const auto& type : ns->types())
        {
            if (!type->is_builtin())
            {
                type_namespace_map_[type] = namespace_path;
            }
        }
        
        // Map all functions in this namespace
        for (const auto& func : ns->functions())
        {
            function_namespace_map_[func] = namespace_path;
        }

        // Recursively process nested namespaces
        for (const auto& nested : ns->nested())
        {
            map_namespace_types(nested);
        }
    };

    // Process all root namespaces
    for (const auto& ns : ast.namespaces())
    {
        if (ns->parent() == nullptr) // Only process root namespaces
        {
            map_namespace_types(ns);
        }
    }
}

} // namespace skr::CppSL