#include "CppSL/langs/HLSLGenerator.hpp"

namespace skr::CppSL
{
inline wchar_t GetResourceRegisterCharacter(const ResourceTypeDecl* type)
{
    if (auto asCBV = dynamic_cast<const ConstantBufferTypeDecl*>(type))
        return L'b';
    else if (auto asSRV = dynamic_cast<const BufferTypeDecl*>(type))
        return has_flag(asSRV->flags(), BufferFlags::ReadWrite) ? L'u' : L't';
    else if (auto asTexture = dynamic_cast<const TextureTypeDecl*>(type))
        return has_flag(asTexture->flags(), TextureFlags::ReadWrite) ? L'u' : L't';
    else if (auto asSampler = dynamic_cast<const SamplerDecl*>(type))
        return L's';
    return L'0';
}

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

inline static bool NeedParens(const Stmt* stmt)
{
    if (auto parent = stmt->parent())
    {
        if (dynamic_cast<const BinaryExpr*>(parent) || 
            dynamic_cast<const UnaryExpr*>(parent) || 
            dynamic_cast<const CastExpr*>(parent))
        {
            return true;
        }
    }
    return false;
}

inline static String GetTypeName(const TypeDecl* type)
{
    if (auto asTexture = dynamic_cast<const TextureTypeDecl*>(type))
    {
        if (!has_flag(asTexture->flags(), TextureFlags::ReadWrite))
        {
            // remove <T> if is not RW
            const auto& name = type->name();
            const auto pos = name.find(L'<');
            if (pos != String::npos) 
                return name.substr(0, pos);
        }
    }
    return type->name();
}

inline static String GetStageName(ShaderStage stage)
{
    switch (stage)
    {
    case ShaderStage::Vertex:
        return L"vertex";
    case ShaderStage::Fragment:
        return L"pixel";
    case ShaderStage::Compute:
        return L"compute";
    default:
        assert(false && "Unknown shader stage");
        return L"unknown_stage";
    }
}

static const std::unordered_map<SemanticType, String> SystemValueMap = {
    { SemanticType::Invalid, L"" },
    { SemanticType::Position, L"SV_Position" },
    { SemanticType::ClipDistance, L"SV_ClipDistance" },
    { SemanticType::CullDistance, L"SV_CullDistance" },
    
    { SemanticType::RenderTarget0, L"SV_Target0" },
    { SemanticType::RenderTarget1, L"SV_Target1" },
    { SemanticType::RenderTarget2, L"SV_Target2" },
    { SemanticType::RenderTarget3, L"SV_Target3" },
    { SemanticType::RenderTarget4, L"SV_Target4" },
    { SemanticType::RenderTarget5, L"SV_Target5" },
    { SemanticType::RenderTarget6, L"SV_Target6" },
    { SemanticType::RenderTarget7, L"SV_Target7" },
    
    { SemanticType::Depth, L"SV_Depth" },
    { SemanticType::DepthGreaterEqual, L"SV_DepthGreaterEqual" },
    { SemanticType::DepthLessEqual, L"SV_DepthLessEqual" },
    { SemanticType::StencilRef, L"SV_StencilRef" },
    
    { SemanticType::VertexID, L"SV_VertexID" },
    { SemanticType::InstanceID, L"SV_InstanceID" },
    
    { SemanticType::GSInstanceID, L"SV_GSInstanceID" },
    { SemanticType::TessFactor, L"SV_TessFactor" },
    { SemanticType::InsideTessFactor, L"SV_InsideTessFactor" },
    { SemanticType::DomainLocation, L"SV_DomainLocation" },
    { SemanticType::ControlPointID, L"SV_ControlPointID" },
    
    { SemanticType::PrimitiveID, L"SV_PrimitiveID" },
    { SemanticType::IsFrontFace, L"SV_IsFrontFace" },
    { SemanticType::SampleIndex, L"SV_SampleIndex" },
    { SemanticType::SampleMask, L"SV_Coverage" },
    { SemanticType::Barycentrics, L"SV_Barycentrics" },
    
    { SemanticType::ThreadID, L"SV_DispatchThreadID" },
    { SemanticType::GroupID, L"SV_GroupID" },
    { SemanticType::ThreadPositionInGroup, L"SV_GroupThreadID" },
    { SemanticType::ThreadIndexInGroup, L"SV_GroupIndex" },
    
    { SemanticType::ViewID, L"SV_ViewID" }
};
static const String UnknownSystemValue = L"UnknownSystemValue";
inline static const String& GetSystemValueString(SemanticType Semantic)
{
    auto it = SystemValueMap.find(Semantic);
    if (it != SystemValueMap.end())
    {
        return it->second;
    }
    return UnknownSystemValue;
}

inline static bool RecordGlobalResource(SourceBuilderNew& sb, const skr::CppSL::VarDecl* var)
{
    if (auto asResource = dynamic_cast<const ResourceTypeDecl*>(&var->type()))
    {
        String content = GetTypeName(&var->type()) + L" " + var->name();
        if (const auto pushConstant = FindAttr<PushConstantAttr>(var->attrs()))
        {
            sb.append(L"[[vk::push_constant]]");
            sb.endline();
        }

        String vk_binding = L"";
        String reg_info = L"";
        if (const auto resourceBind = FindAttr<ResourceBindAttr>(var->attrs()))
        {
            const auto R = GetResourceRegisterCharacter(asResource);
            const auto binding = resourceBind->binding();
            const auto group = resourceBind->group();
            if (binding != ~0)
            {
                if (group != ~0)
                {
                    vk_binding = std::format(L"[[vk::binding({}, {})]]", binding, group);
                    reg_info = std::format(L"register({}{}, space{})", R, binding, group);
                }
                else
                {
                    vk_binding = std::format(L"[[vk::binding({}, {})]]", binding, 0);
                    reg_info = std::format(L"register({}{})", R, binding);
                }
            }
        }

        if (!vk_binding.empty())
        {
            sb.append(vk_binding);
            sb.endline();
        }
        sb.append(content);
        if(!reg_info.empty())
        {
            sb.append(L": " + reg_info);
        }
        return true;
    }
    return false;
}

void HLSLGenerator::visitExpr(SourceBuilderNew& sb, const skr::CppSL::Stmt* stmt)
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

    if (auto binary = dynamic_cast<const BinaryExpr*>(stmt))
    {
        const bool needParens = NeedParens(stmt);
        if (needParens)
            sb.append(L"(");

        auto ltype = binary->left()->type();
        auto rtype = binary->right()->type();
        bool is_vec_mat_op = (ltype->is_vector() && rtype->is_matrix()) || (ltype->is_matrix() && rtype->is_vector());
        if (is_vec_mat_op && (binary->op() == BinaryOp::MUL))
        {
            sb.append(L"mul(");
            visitExpr(sb, binary->left());
            sb.append(L", ");
            visitExpr(sb, binary->right());
            sb.append(L")");
        }
        else if (is_vec_mat_op && (binary->op() == BinaryOp::MUL_ASSIGN))
        {
            visitExpr(sb, binary->left());
            sb.append(L" = mul(");
            visitExpr(sb, binary->left());
            sb.append(L", ");
            visitExpr(sb, binary->right());
            sb.append(L")");
        }
        else
        {
            visitExpr(sb, binary->left());
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
            visitExpr(sb, binary->right());
        }

        if (needParens)
            sb.append(L")");
    }
    else if (auto bitwiseCast = dynamic_cast<const BitwiseCastExpr*>(stmt))
    {
        auto _type = bitwiseCast->type();
        sb.append(L"bit_cast<" + GetTypeName(_type) + L">(");
        visitExpr(sb, bitwiseCast->expr());
        sb.append(L")");
    }
    else if (auto breakStmt = dynamic_cast<const BreakStmt*>(stmt))
    {
        sb.append(L"break");
    }
    else if (auto block = dynamic_cast<const CompoundStmt*>(stmt))
    {
        sb.endline(L'{');
        sb.indent([&](){
            for (auto expr : block->children())
            {
                visitExpr(sb, expr);
            }
        });
        sb.append(L"}");
    }
    else if (auto condExpr = dynamic_cast<const ConditionalExpr*>(stmt))
    {
        visitExpr(sb, condExpr->cond());
        sb.append(L" ? ");
        visitExpr(sb, condExpr->then_expr());
        sb.append(L" : ");
        visitExpr(sb, condExpr->else_expr());
    }
    else if (auto callExpr = dynamic_cast<const CallExpr*>(stmt))
    {
        auto callee = callExpr->callee();
        if (auto callee_decl = dynamic_cast<const FunctionDecl*>(callee->decl()))
        {
            sb.append(callee_decl->name());
            sb.append(L"(");
            for (size_t i = 0; i < callExpr->args().size(); i++)
            {
                auto arg = callExpr->args()[i];
                if (i > 0)
                    sb.append(L", ");
                visitExpr(sb, arg);
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
            visitExpr(sb, caseStmt->cond());
            sb.append(L":");
        }
        else
        {
            sb.append(L"default:");
        }
        visitExpr(sb, caseStmt->body());
    }
    else if (auto methodCall = dynamic_cast<const MethodCallExpr*>(stmt))
    {
        auto callee = methodCall->callee();
        auto method = dynamic_cast<const MethodDecl*>(callee->member_decl());
        auto type = method->owner_type();
        if (auto as_buffer = dynamic_cast<const BufferTypeDecl*>(type) && method->name() == L"Store")
        {
            visitExpr(sb, callee->owner());
            sb.append(L"[");
            visitExpr(sb, methodCall->args()[0]);
            sb.append(L"] = ");
            visitExpr(sb, methodCall->args()[1]);
        }
        else
        {
            visitExpr(sb, callee);
            
            sb.append(L"(");
            for (size_t i = 0; i < methodCall->args().size(); i++)
            {
                auto arg = methodCall->args()[i];
                if (i > 0)
                    sb.append(L", ");
                visitExpr(sb, arg);
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
            sb.append(std::to_wstring(f->ieee.value()));
        }
        else
        {
            sb.append(L"UnknownConstant: ");
        }
    }
    else if (auto ctorExpr = dynamic_cast<const ConstructExpr*>(stmt))
    {
        std::span<Expr* const> args;
        std::vector<Expr*> modified_args;
        int32_t fillVectorArgsWithZero = 0;
        if (auto AsVector = dynamic_cast<const VectorTypeDecl*>(ctorExpr->type());
            AsVector && ((ctorExpr->args().size() == 0) || (ctorExpr->args().size() == 1))
        )
        {
            if (ctorExpr->args().size() == 0)
            {
                fillVectorArgsWithZero = AsVector->count();
            }   
            else if (dynamic_cast<const ScalarTypeDecl*>(ctorExpr->args()[0]->type()))
            {
                for (uint32_t i = 0; i < AsVector->count(); i++)
                    modified_args.emplace_back(ctorExpr->args()[0]);
                args = modified_args;
            }
        }
        else
        {
            args = ctorExpr->args();
        }

        sb.append(GetTypeName(ctorExpr->type()) + L"(");
        if (fillVectorArgsWithZero > 0)
        {
            for (int32_t j = 0; j < fillVectorArgsWithZero; j++)
            {
                if (j > 0)
                    sb.append(L", ");
                sb.append(L"0");
            }
        }
        else
        {
            for (size_t i = 0; i < args.size(); i++)
            {
                auto arg = args[i];
                if (i > 0)
                {
                    sb.append(L", ");
                }
                visitExpr(sb, arg);
            }
        }
        sb.append(L")");
    }
    else if (auto continueStmt = dynamic_cast<const ContinueStmt*>(stmt))
    {
        sb.append(L"continue;");
    }
    else if (auto defaultStmt = dynamic_cast<const DefaultStmt*>(stmt))
    {
        sb.append(L"default:");
        visitExpr(sb, defaultStmt->children()[0]);
    }
    else if (auto member = dynamic_cast<const MemberExpr*>(stmt))
    {
        auto owner = member->owner();
        if (auto _member = dynamic_cast<const NamedDecl*>(member->member_decl()))
        {
            if (auto fromThis = dynamic_cast<const ThisExpr*>(member->children()[0]))
                sb.append(L"/*this.*/" + _member->name());
            else
            {
                visitExpr(sb, owner);
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
            visitExpr(sb, forStmt->init());
        sb.append(L"; ");
        
        if (forStmt->cond())
            visitExpr(sb, forStmt->cond());
        sb.append(L"; ");

        if (forStmt->inc())
            visitExpr(sb, forStmt->inc());
        sb.append(L") ");

        visitExpr(sb, forStmt->body());
        sb.endline();
    }
    else if (auto ifStmt = dynamic_cast<const IfStmt*>(stmt))
    {
        sb.append(L"if (");
        visitExpr(sb, ifStmt->cond());
        sb.append(L")");
        sb.endline();
        visitExpr(sb, ifStmt->then_body());

        if (ifStmt->else_body())
        {
            sb.endline();
            sb.append(L"else");
            sb.endline();
            visitExpr(sb, ifStmt->else_body());
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
            visitExpr(sb, expr);
        }
        sb.append(L" }");
    }
    else if (auto implicitCast = dynamic_cast<const ImplicitCastExpr*>(stmt))
    {
        visitExpr(sb, implicitCast->expr());
    }
    else if (auto declRef = dynamic_cast<const DeclRefExpr*>(stmt))
    {
        if (auto decl = dynamic_cast<const VarDecl*>(declRef->decl()))
            sb.append(decl->name());
    }
    else if (auto returnStmt = dynamic_cast<const ReturnStmt*>(stmt))
    {
        sb.append(L"return");
        if (returnStmt->value())
        {
            sb.append(L" ");
            visitExpr(sb, returnStmt->value());
        }
    }
    else if (auto staticCast = dynamic_cast<const StaticCastExpr*>(stmt))
    {
        sb.append(L"((" + GetTypeName(staticCast->type()) + L")");
        visitExpr(sb, staticCast->expr());
        sb.append(L")");
    }
    else if (auto switchStmt = dynamic_cast<const SwitchStmt*>(stmt))
    {
        sb.append(L"switch (");
        visitExpr(sb, switchStmt->cond());
        sb.append(L")");
        sb.endline();
        sb.indent([&](){
            for (auto case_stmt : switchStmt->cases())
            {
                visitExpr(sb, case_stmt);
                sb.endline();
            }
        });
    }
    else if (auto unary = dynamic_cast<const UnaryExpr*>(stmt))
    {
        const bool needParens = NeedParens(stmt);
        if (needParens)
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

            visitExpr(sb, unary->expr());
            
            if (post)
                sb.append(op_name);
        }

        if (needParens)
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
            visitExpr(sb, dynamic_cast<const DeclStmt*>(decl));
            sb.append(L";");
        }
    }
    else if (auto whileStmt = dynamic_cast<const WhileStmt*>(stmt))
    {
        sb.append(L"while (");
        visitExpr(sb, whileStmt->cond());
        sb.append(L") ");
        visitExpr(sb, whileStmt->body());
    }
    else if (auto access = dynamic_cast<const AccessExpr*>(stmt))
    {
        auto to_access = dynamic_cast<const Expr*>(access->children()[0]);
        auto index = dynamic_cast<const Expr*>(access->children()[1]);
        auto type_to_access = to_access->type();

        if (auto AsArray = dynamic_cast<const ArrayTypeDecl*>(type_to_access))
        {
            visitExpr(sb, to_access);
            sb.append(L".data[");
            visitExpr(sb, index);
            sb.append(L"]");
        }
        else
        {
            visitExpr(sb, to_access);
            sb.append(L"[");
            visitExpr(sb, index);
            sb.append(L"]");
        }
    }
    else if (auto swizzle = dynamic_cast<const SwizzleExpr*>(stmt))
    {
        visitExpr(sb, swizzle->expr());
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

void HLSLGenerator::visit(SourceBuilderNew& sb, const skr::CppSL::TypeDecl* typeDecl)
{
    using namespace skr::CppSL;
    const bool DUMP_BUILTIN_TYPES = false;
    const bool IsStageInout = FindAttr<StageInoutAttr>(typeDecl->attrs());
    if (!typeDecl->is_builtin())
    {
        sb.append(L"struct " + GetTypeName(typeDecl));
        sb.endline(L'{');
        sb.indent([&] {
            for (auto field : typeDecl->fields())
            {
                sb.append(GetTypeName(&field->type()) + L" " + field->name());
                if (IsStageInout)
                    sb.append(L" : " + field->name());
                sb.endline(L';');
            }
            for (auto method : typeDecl->methods())
            {
                visit(sb, method);
            }
            for (auto ctor : typeDecl->ctors())
            {
                visit(sb, ctor);

                AST* pAST = const_cast<AST*>(&typeDecl->ast());
                std::vector<Expr*> param_refs;
                param_refs.reserve(ctor->parameters().size());
                for (auto param : ctor->parameters())
                {
                    param_refs.emplace_back(param->ref());
                }
                
                // HLSL: Type _this = (Type)0;
                auto _this = pAST->Variable(EVariableQualifier::None, typeDecl, L"_this", pAST->StaticCast(typeDecl, pAST->Constant(IntValue(0))));
                // HLSL: _this.__SSL_CTOR__(args...);
                auto _init = pAST->CallMethod(pAST->Method(_this->ref(), ctor), param_refs);
                // HLSL: return _this;
                auto _return = pAST->Return(_this->ref());

                auto WrapperBody = pAST->Block({ _this, _init, _return });
                sb.append(L"static ");
                // 只 declare 这些 method，但是不把他们加到类型里面，不然会被生成 method 的逻辑重复生成
                visit(sb, pAST->DeclareMethod(const_cast<skr::CppSL::TypeDecl*>(typeDecl), L"__CTOR__", typeDecl, ctor->parameters(), WrapperBody));
            }
        });
        sb.append(L"}");
        sb.endline(L';');
        sb.append(L"#define " + GetTypeName(typeDecl) + L"(__VA_ARGS__) " + GetTypeName(typeDecl) + L"::__CTOR__(__VA_ARGS__)");
        sb.endline();
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

void HLSLGenerator::visit(SourceBuilderNew& sb, const skr::CppSL::FunctionDecl* funcDecl)
{
    using namespace skr::CppSL;
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
                    RecordGlobalResource(sb, param);
                    sb.endline(L';');
                    params.erase(std::find(params.begin(), params.end(), param));
                }
            }
            // generate stage entry attributes
            sb.append(L"[shader(\"" + GetStageName(StageEntry->stage()) + L"\")]\n");
            // generate kernel size
            if (auto kernelSize = FindAttr<KernelSizeAttr>(funcDecl->attrs()))
            {
                sb.append(L"[numthreads(" + std::to_wstring(kernelSize->x()) + L", " + std::to_wstring(kernelSize->y()) + L", " + std::to_wstring(kernelSize->z()) + L")]");
                sb.endline();
            }
        }
        
        // generate signature
        {
            sb.append(GetTypeName(funcDecl->return_type()) + L" " + funcDecl->name() + L"(");
            for (size_t i = 0; i < params.size(); i++)
            {
                auto param = params[i];
                auto qualifier = param->qualifier();
                
                String semantic_string = L"";
                if (StageEntry)
                {
                    if (auto AsSemantic = FindAttr<SemanticAttr>(param->attrs()))
                    {
                        if (SemanticAttr::GetSemanticQualifier(AsSemantic->semantic(), StageEntry->stage(), qualifier))
                        {
                            semantic_string = L" : " + GetSystemValueString(AsSemantic->semantic());
                        }
                        else
                        {
                            param->ast().ReportFatalError(std::format(L"Invalid semantic {} for param {} within stage {}", 
                                GetSystemValueString(AsSemantic->semantic()), param->name(), GetStageName(StageEntry->stage()))
                            );
                        }
                    }
                }

                String prefix = L"";
                switch (qualifier) 
                {
                case EVariableQualifier::Const:
                    prefix = L"const ";
                    break;
                case EVariableQualifier::Out:
                    prefix = L"out ";
                    break;
                case EVariableQualifier::Inout:
                    prefix = L"inout ";
                    break;
                case EVariableQualifier::None:
                    prefix = L"";
                    break;
                }
                String content = prefix + GetTypeName(&param->type()) + L" " + param->name();
    
                if (i > 0)
                    content = L", " + content;
                sb.append(content);
                sb.append(semantic_string);
            }
            sb.append(L")");
            if (StageEntry && StageEntry->stage() == ShaderStage::Fragment)
            {
                // HLSL: SV_Position is the output of vertex shader, so we need to add it to fragment shader
                sb.append(L" : SV_Target");
            }
            sb.endline();
        }

        // generate body
        visitExpr(sb, funcDecl->body());
        sb.endline();
    }
}

void HLSLGenerator::visit(SourceBuilderNew& sb, const skr::CppSL::VarDecl* varDecl)
{
    const auto isGlobal = dynamic_cast<const skr::CppSL::GlobalVarDecl*>(varDecl);
    if (auto RecordAsResource = RecordGlobalResource(sb, varDecl))
    {

    }
    else
    {
        if (varDecl->qualifier() == EVariableQualifier::Const)
            sb.append(isGlobal ? L"static const " : L"const ");
        else if (varDecl->qualifier() == EVariableQualifier::Inout)
            sb.append(L"inout ");
    
        sb.append(GetTypeName(&varDecl->type()) + L" " + varDecl->name());
        if (auto init = varDecl->initializer())
        {
            sb.append(L" = ");
            visitExpr(sb, init);
        }
    }
}

static const skr::CppSL::String kHLSLHeader = LR"(
template <typename _ELEM> void buffer_write(RWStructuredBuffer<_ELEM> buffer, uint index, _ELEM value) { buffer[index] = value; }
template <typename _ELEM> _ELEM buffer_read(RWStructuredBuffer<_ELEM> buffer, uint index) { return buffer[index]; }
template <typename _ELEM, uint64_t N> struct array { _ELEM data[N]; };

template <typename _TEX> float4 texture2d_sample(_TEX tex, uint2 uv, uint filter, uint address) { return float4(1, 1, 1, 1); }
template <typename _TEX> float4 texture3d_sample(_TEX tex, uint3 uv, uint filter, uint address) { return float4(1, 1, 1, 1); }

template <typename _ELEM> _ELEM texture_read(Texture2D<_ELEM> tex, uint2 uv) { return tex.Load(uv); }
template <typename _ELEM> _ELEM texture_read(RWTexture2D<_ELEM> tex, uint2 uv) { return tex.Load(uv); }
template <typename _ELEM> _ELEM texture_write(RWTexture2D<_ELEM> tex, uint2 uv, _ELEM v) { return tex[uv] = v; }

template <typename _ELEM> uint2 texture_size(Texture2D<_ELEM> tex) { uint Width, Height, Mips; tex.GetDimensions(0, Width, Height, Mips); return uint2(Width, Height); }
template <typename _ELEM> uint2 texture_size(RWTexture2D<_ELEM> tex) { uint Width, Height; tex.GetDimensions(Width, Height); return uint2(Width, Height); }
template <typename _ELEM> uint3 texture_size(Texture3D<_ELEM> tex) { uint Width, Height, Depth, Mips; tex.GetDimensions(0, Width, Height, Depth, Mips); return uint3(Width, Height, Depth); }
template <typename _ELEM> uint3 texture_size(RWTexture3D<_ELEM> tex) { uint Width, Height, Depth; tex.GetDimensions(Width, Height, Depth); return uint3(Width, Height, Depth); }

float4 sample2d(SamplerState s, Texture2D t, float2 uv) { return t.Sample(s, uv); }

// TODO: DELETE
uint2 luisa__shader__dispatch_size() { return uint2(0, 0); }
uint2 luisa__shader__dispatch_id() { return uint2(0, 0); }
)";

String HLSLGenerator::generate_code(SourceBuilderNew& sb, const AST& ast)
{
    using namespace skr::CppSL;

    sb.append(kHLSLHeader);
    sb.endline();
    
    for (const auto& type : ast.types())
    {
        visit(sb, type);
        sb.endline();
    }
    sb.append(L" ");
    sb.endline();

    for (const auto& global : ast.global_vars())
    {
        visit(sb, global);
        sb.endline(L';');
    }
    sb.append(L" ");
    sb.endline();
    
    for (const auto& func : ast.funcs())
    {
        visit(sb, func);
    }
    sb.append(L" ");
    sb.endline();

    return sb.build(SourceBuilderNew::line_builder_code);
}

} // namespace skr::CppSL