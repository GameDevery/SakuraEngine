#include "CppSL/langs/HLSLGenerator.hpp"
#include <unordered_set>
#include <unordered_map>
#include <tuple>

namespace skr::CppSL::HLSL
{
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
    else if (auto asAS = dynamic_cast<const AccelTypeDecl*>(type))
        return L't';
    return L'0';
}

static const std::unordered_map<InterpolationMode, String> InterpolationMap = {
    { InterpolationMode::linear, L"linear" },
    { InterpolationMode::nointerpolation, L"nointerpolation" },
    { InterpolationMode::centroid, L"centroid" },
    { InterpolationMode::sample, L"sample" },
    { InterpolationMode::noperspective, L"noperspective" }
};
static const String UnknownInterpolation = L"UnknownInterpolation";
inline static const String& GetInterpolationString(InterpolationMode Interpolation)
{
    auto it = InterpolationMap.find(Interpolation);
    if (it != InterpolationMap.end())
    {
        return it->second;
    }
    return UnknownInterpolation;
}

String HLSLGenerator::GetTypeName(const TypeDecl* type)
{
    if (auto asTexture = dynamic_cast<const TextureTypeDecl*>(type))
    {
        // cast <{T}> to <{T}4>
        const auto& name = type->name();
        const auto pos = name.find(L'>');
        if (pos != String::npos)
        {
            String result = name;
            result.insert(pos, L"4");
            return result;
        }
    }
    else if (auto array = dynamic_cast<const ArrayTypeDecl*>(type))
    {
        if (array->element_type()->is_resource() && (array->count() == 0))
        {
            return std::format(L"Bindless< {} >", GetQualifiedTypeName(array->element_type()));
        }
        return std::format(L"array<{}, {}>", GetQualifiedTypeName(array->element_type()), array->count());
    }
    else if (auto cbuffer = dynamic_cast<const ConstantBufferTypeDecl*>(type))
    {
        return L"ConstantBuffer<" + GetQualifiedTypeName(cbuffer->element_type()) + L">";
    }
    else if (auto asRayQuery = dynamic_cast<const RayQueryTypeDecl*>(type))
    {
        const auto flags = asRayQuery->flags();
        String FlagText = L"RAY_FLAG_NONE";
        if (has_flag(flags, RayQueryFlags::ForceOpaque))
            FlagText += L" | RAY_FLAG_FORCE_OPAQUE";
        if (has_flag(flags, RayQueryFlags::ForceNonOpaque))
            FlagText += L" | RAY_FLAG_FORCE_NON_OPAQUE";
        if (has_flag(flags, RayQueryFlags::AcceptFirstAndEndSearch))
            FlagText += L" | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH ";
        if (has_flag(flags, RayQueryFlags::CullBackFace))
            FlagText += L" | RAY_FLAG_CULL_BACK_FACING_TRIANGLES";
        if (has_flag(flags, RayQueryFlags::CullFrontFace))
            FlagText += L" | RAY_FLAG_CULL_FRONT_FACING_TRIANGLES";
        if (has_flag(flags, RayQueryFlags::CullOpaque))
            FlagText += L" | RAY_FLAG_CULL_OPAQUE";
        if (has_flag(flags, RayQueryFlags::CullNonOpaque))
            FlagText += L" | RAY_FLAG_CULL_NON_OPAQUE";
        if (has_flag(flags, RayQueryFlags::CullTriangle))
            FlagText += L" | RAY_FLAG_CULL_TRIANGLES";
        if (has_flag(flags, RayQueryFlags::CullProcedural))
            FlagText += L" | RAY_FLAG_CULL_PROCEDURAL_PRIMITIVES";
        return L"RayQuery<" + FlagText + L">";
    }
    else if (auto asArray = dynamic_cast<const ArrayTypeDecl*>(type))
    {
        if (auto asBdlsArray = asArray->element_type()->is_resource())
        {
            return L"Bindless<" + GetTypeName(asArray->element_type()) + L">";
        }
    }
    return type->name();
}

void HLSLGenerator::VisitAccessExpr(SourceBuilderNew& sb, const AccessExpr* expr)
{
    auto to_access = dynamic_cast<const Expr*>(expr->children()[0]);
    auto index = dynamic_cast<const Expr*>(expr->children()[1]);
    visitStmt(sb, to_access);

    auto asRef = dynamic_cast<const DeclRefExpr*>(to_access);
    const auto asArray = dynamic_cast<const ArrayTypeDecl*>(to_access->type());

    const bool isGlobalResourceBind = asRef && FindAttr<ResourceBindAttr>(asRef->decl()->attrs());
    const auto asBdlsResource = asArray && asArray->element_type()->is_resource() && (asArray->count() == 0);
    if ((!asBdlsResource && !isGlobalResourceBind) && to_access->type()->is_array())
        sb.append(L".data");

    sb.append(L"[");
    if (asBdlsResource)
        sb.append(L"NonUniformResourceIndex(");
    visitStmt(sb, index);
    if (asBdlsResource)
        sb.append(L")");
    sb.append(L"]");
}

void HLSLGenerator::VisitShaderResource(SourceBuilderNew& sb, const skr::CppSL::VarDecl* var)
{
    // Handle array of resources - need C-style array syntax
    String typeName = GetTypeName(&var->type());
    String varName = var->name();
    String arrayDimensions = L"";
    // Check if this is an array type
    const TypeDecl* asResource = &var->type();
    while (auto asArray = dynamic_cast<const ArrayTypeDecl*>(asResource))
    {
        // Get the element type
        asResource = asArray->element_type();

        // Collect array dimensions for C-style syntax
        if (asArray->count() > 0)
            arrayDimensions += L"[" + std::to_wstring(asArray->count()) + L"]";
        else
            arrayDimensions += L"[]"; // Unbounded array

        // Update typeName to be the base element type
        if (auto elementResource = dynamic_cast<const ResourceTypeDecl*>(asResource))
        {
            typeName = GetTypeName(asResource);
        }
    }
    String content = typeName + L" " + varName + arrayDimensions;

    const auto asPushConstant = FindAttr<PushConstantAttr>(var->attrs());
    if (asPushConstant)
    {
        sb.append(L"[[vk::push_constant]]");
        sb.endline();
    }

    String vk_binding = L"";
    String reg_info = L"";
    // 使用 GenerateSRTs 生成的结果（所有全局资源必须已收录）
    if (auto it = binding_table_.find(var); it != binding_table_.end())
    {
        const auto& b = it->second;
        const auto R = GetResourceRegisterCharacter(dynamic_cast<const ResourceTypeDecl*>(asResource));
        vk_binding = std::format(L"[[vk::binding({}, {})]]", b.binding, b.space);
        reg_info = std::format(L"register({}{}, space{})", R, b.binding, b.space);
    }
    else
    {
    }

    if (!asPushConstant && !vk_binding.empty())
    {
        sb.append(vk_binding);
        sb.endline();
    }
    sb.append(content);
    if (!reg_info.empty())
    {
        sb.append(L": " + reg_info);
    }
    sb.endline(L';');
}

void HLSLGenerator::VisitBinaryExpr(SourceBuilderNew& sb, const BinaryExpr* binary)
{
    auto ltype = binary->left()->type();
    auto rtype = binary->right()->type();
    bool is_vec_mat_op = (ltype->is_vector() && rtype->is_matrix()) || (ltype->is_matrix() && rtype->is_vector());
    if (is_vec_mat_op && (binary->op() == BinaryOp::MUL))
    {
        sb.append(L"mul(");
        visitStmt(sb, binary->left());
        sb.append(L", ");
        visitStmt(sb, binary->right());
        sb.append(L")");
    }
    else if (is_vec_mat_op && (binary->op() == BinaryOp::MUL_ASSIGN))
    {
        visitStmt(sb, binary->left());
        sb.append(L" = mul(");
        visitStmt(sb, binary->left());
        sb.append(L", ");
        visitStmt(sb, binary->right());
        sb.append(L")");
    }
    else
    {
        CppLikeShaderGenerator::VisitBinaryExpr(sb, binary);
    }
}

void HLSLGenerator::VisitConstructExpr(SourceBuilderNew& sb, const ConstructExpr* ctorExpr)
{
    if (auto AsRayQuery = dynamic_cast<const RayQueryTypeDecl*>(ctorExpr->type()))
    {
        sb.append(L"[RayQuery SHOULD NEVER BE INITIALIZED IN HLSL]");
    }
    else if (auto AsArray = dynamic_cast<const ArrayTypeDecl*>(ctorExpr->type()))
    {
        const auto N = AsArray->size() / AsArray->element_type()->size();
        sb.append(L"make_array" + std::to_wstring(N) + L"<" + GetQualifiedTypeName(AsArray->element_type()) + L", " + std::to_wstring(N) + L">(");
        ;
        for (size_t i = 0; i < ctorExpr->args().size(); i++)
        {
            auto arg = ctorExpr->args()[i];
            if (i > 0)
            {
                sb.append(L", ");
            }
            visitStmt(sb, arg);
        }
        sb.append(L")");
    }
    else
    {
        std::span<Expr* const> args;
        std::vector<Expr*> modified_args;
        int32_t fillArgsWithZero = 0;
        if (auto AsVector = dynamic_cast<const VectorTypeDecl*>(ctorExpr->type());
            AsVector && ((ctorExpr->args().size() == 0) || (ctorExpr->args().size() == 1)))
        {
            if (ctorExpr->args().size() == 0)
            {
                fillArgsWithZero = AsVector->count();
            }
            else if (dynamic_cast<const ScalarTypeDecl*>(ctorExpr->args()[0]->type()))
            {
                for (uint32_t i = 0; i < AsVector->count(); i++)
                    modified_args.emplace_back(ctorExpr->args()[0]);
                args = modified_args;
            }
        }
        else if (ctorExpr->type()->is_builtin())
        {
            if (ctorExpr->args().size() == 0)
            {
                fillArgsWithZero = 1;
            }
        }

        if (args.empty())
            args = ctorExpr->args();

        if (ctorExpr->type()->is_builtin())
            sb.append(GetQualifiedTypeName(ctorExpr->type()) + L"(");
        else
            sb.append(GetQualifiedTypeName(ctorExpr->type()) + L"::New(");

        if (fillArgsWithZero > 0)
        {
            for (int32_t j = 0; j < fillArgsWithZero; j++)
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
                visitStmt(sb, arg);
            }
        }
        sb.append(L")");
    }
}

void HLSLGenerator::VisitVariable(SourceBuilderNew& sb, const skr::CppSL::VarDecl* varDecl)
{
    const auto isGlobal = dynamic_cast<const skr::CppSL::GlobalVarDecl*>(varDecl);
    if (varDecl->qualifier() == EVariableQualifier::Const)
        sb.append(isGlobal ? L"static const " : L"const ");
    else if (varDecl->qualifier() == EVariableQualifier::Inout)
    {
        // buffer/texture/... can't pass with inout arg
        if (!varDecl->type().is_resource())
            sb.append(L"inout ");
    }
    else if (varDecl->qualifier() == EVariableQualifier::GroupShared)
        sb.append(L"groupshared ");

    sb.append(GetQualifiedTypeName(&varDecl->type()) + L" " + varDecl->name());
    if (auto init = varDecl->initializer())
    {
        if (auto asRayQuery = dynamic_cast<const RayQueryTypeDecl*>(init->type()))
        {
            // do nothing because ray query should not initialize in HLSL
        }
        else
        {
            sb.append(L" = ");
            visitStmt(sb, init);
        }
    }
}

void HLSLGenerator::VisitParameter(SourceBuilderNew& sb, const skr::CppSL::FunctionDecl* funcDecl, const skr::CppSL::ParamVarDecl* param)
{
    const StageAttr* StageEntry = FindAttr<StageAttr>(funcDecl->attrs());
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
                    GetSystemValueString(AsSemantic->semantic()),
                    param->name(),
                    (uint32_t)StageEntry->stage()));
            }
        }
    }

    String prefix = L"";
    switch (qualifier)
    {
    case EVariableQualifier::None:
        prefix = L"";
        break;
    case EVariableQualifier::Const:
        prefix = L"const ";
        break;
    case EVariableQualifier::Out:
        prefix = L"out ";
        break;
    case EVariableQualifier::Inout:
    {
        // buffer/texture/... can't pass with inout arg
        if (!param->type().is_resource())
            prefix = L"inout ";
    }
    break;
    case EVariableQualifier::GroupShared:
        prefix = L"groupshared ";
        break;
    }
    String content = prefix + GetQualifiedTypeName(&param->type()) + L" " + param->name();

    sb.append(content);
    sb.append(semantic_string);
}

void HLSLGenerator::VisitField(SourceBuilderNew& sb, const skr::CppSL::TypeDecl* typeDecl, const skr::CppSL::FieldDecl* field)
{
    const bool IsStageInout = FindAttr<StageInoutAttr>(typeDecl->attrs());

    if (auto interpolation = FindAttr<InterpolationAttr>(field->attrs()))
    {
        sb.append(GetInterpolationString(interpolation->mode()) + L" ");
    }

    sb.append(GetQualifiedTypeName(&field->type()) + L" " + field->name());
    if (IsStageInout)
        sb.append(L" : " + field->name());
    sb.endline(L';');
}

void HLSLGenerator::VisitConstructor(SourceBuilderNew& sb, const ConstructorDecl* ctor, FunctionStyle style)
{
    CppLikeShaderGenerator::VisitConstructor(sb, ctor, FunctionStyle::SignatureOnly);

    auto typeDecl = ctor->owner_type();
    AST* pAST = const_cast<AST*>(&typeDecl->ast());
    std::vector<Expr*> param_refs;
    param_refs.reserve(ctor->parameters().size());
    for (auto param : ctor->parameters())
    {
        param_refs.emplace_back(param->ref());
    }

    // HLSL: Type _this = (Type)0;
    auto _this = pAST->Variable(EVariableQualifier::None, typeDecl, L"_this");
    // HLSL: _this.__SSL_CTOR__(args...);
    auto _init = pAST->CallMethod(pAST->Method(_this->ref(), ctor), param_refs);
    // HLSL: return _this;
    auto _return = pAST->Return(_this->ref());

    auto WrapperBody = pAST->Block({ _this, _init, _return });
    sb.append(L"static ");
    // 只 declare 这些 method，但是不把他们加到类型里面，不然会被生成 method 的逻辑重复生成
    visit(sb, pAST->DeclareMethod(const_cast<skr::CppSL::TypeDecl*>(typeDecl), L"New", typeDecl, ctor->parameters(), WrapperBody), FunctionStyle::Normal);
}

void HLSLGenerator::GenerateStmtAttributes(SourceBuilderNew& sb, const skr::CppSL::Stmt* stmt)
{
    if (const auto Loop = FindAttr<LoopAttr>(stmt->attrs()))
    {
        sb.append(L"[loop]");
    }
    if (const auto Unroll = FindAttr<UnrollAttr>(stmt->attrs()))
    {
        if (Unroll->count() != UINT32_MAX)
            sb.append(std::format(L"[unroll({})]", Unroll->count()));
        else
            sb.append(L"[unroll]");
    }
    if (const auto Branch = FindAttr<BranchAttr>(stmt->attrs()))
    {
        sb.append(L"[branch]");
    }
    if (const auto Flatten = FindAttr<FlattenAttr>(stmt->attrs()))
    {
        sb.append(L"[flatten]");
    }
}

void HLSLGenerator::GenerateFunctionAttributes(SourceBuilderNew& sb, const FunctionDecl* funcDecl)
{
    if (const StageAttr* StageEntry = FindAttr<StageAttr>(funcDecl->attrs()))
    {
        // generate stage entry attributes
        sb.append(L"[shader(\"" + GetStageName(StageEntry->stage()) + L"\")]\n");
        // generate kernel size
        if (auto kernelSize = FindAttr<KernelSizeAttr>(funcDecl->attrs()))
        {
            sb.append(L"[numthreads(" + std::to_wstring(kernelSize->x()) + L", " + std::to_wstring(kernelSize->y()) + L", " + std::to_wstring(kernelSize->z()) + L")]");
            sb.endline();
        }
    }
}

void HLSLGenerator::GenerateFunctionSignaturePostfix(SourceBuilderNew& sb, const FunctionDecl* funcDecl)
{
    const StageAttr* StageEntry = FindAttr<StageAttr>(funcDecl->attrs());
    if (StageEntry && StageEntry->stage() == ShaderStage::Fragment)
    {
        // HLSL: SV_Position is the output of vertex shader, so we need to add it to fragment shader
        sb.append(L" : SV_Target");
    }
}

bool HLSLGenerator::SupportConstructor() const
{
    return false;
}

static const skr::CppSL::String kHLSLHeader = LR"(
using uint64 = uint64_t;

template<typename T> T fract(T x){return x - floor(x);}
template<typename T> float length_squared(T x) { return dot(x,x); }

template <typename T, uint64_t N> struct array { T data[N]; };
template <typename T> using Bindless = T[];

template <typename B, typename T> T atomic_fetch_add(B buffer, uint offset, T value) { T prev = 0; InterlockedAdd(buffer[offset], value, prev); return prev; }
template <typename G, typename T> T atomic_fetch_add(inout G shared_v, T value) { T prev = 0; InterlockedAdd(shared_v, value, prev); return prev; }

// template <typename TEX> float4 texture2d_sample(TEX tex, uint2 uv, uint filter, uint address) { return float4(1, 1, 1, 1); }
// template <typename TEX> float4 texture3d_sample(TEX tex, uint3 uv, uint filter, uint address) { return float4(1, 1, 1, 1); }
)";

extern const wchar_t* kHLSLBitCast;
extern const wchar_t* kHLSLBufferIntrinsics;
extern const wchar_t* kHLSLTextureIntrinsics;
extern const wchar_t* kHLSLRayIntrinsics;

void HLSLGenerator::RecordBuiltinHeader(SourceBuilderNew& sb, const AST& ast)
{
    GenerateSRTs(ast);

    bool HasBitCast = false;

    for (auto stmt : ast.stmts())
    {
        if (auto as_call = dynamic_cast<CppSL::CallExpr*>(stmt))
        {
            auto called_function = dynamic_cast<const FunctionDecl*>(as_call->callee()->decl());
            if (called_function && ast.IsIntrinsic(called_function))
            {
                if (called_function->name().starts_with(L"bit_cast"))
                {
                    HasBitCast = true;
                }
            }    
        }
    }

    if (HasBitCast)
    {
        sb.append(kHLSLBitCast);
    }
    sb.append(kHLSLHeader);
    sb.append(kHLSLBufferIntrinsics);
    sb.append(kHLSLTextureIntrinsics);
    sb.append(kHLSLRayIntrinsics);
    sb.endline();

    GenerateArrayHelpers(sb, ast);
}

void HLSLGenerator::GenerateArrayHelpers(SourceBuilderNew& sb, const AST& ast)
{
    std::set<uint32_t> array_dims;
    for (auto&& [element, array] : ast.array_types())
    {
        const auto N = array->size() / element.first->size();
        if (N != 0 && !array_dims.contains(N))
        {
            String args = L"";
            String exprs = L"";
            for (uint32_t i = 0; i < N; i++)
            {
                if (i > 0)
                    args += L", ";
                args += L"T a" + std::to_wstring(i);
                exprs += L"a.data[" + std::to_wstring(i) + L"] = a" + std::to_wstring(i) + L";";
            }
            {
                String _template = L"template <typename T, uint64_t N> array<T, N>";
                auto _signature = L"make_array" + std::to_wstring(N) + L"(" + args + L")";
                auto impl = L"{ array<T, N> a; " + exprs + L"; return a; }";
                sb.append(_template + L" " + _signature + L" " + impl);
                sb.endline();
            }
            {
                String _template = L"template <typename T, uint64_t N> array<T, N>";
                auto _signature = L"make_array" + std::to_wstring(N) + L"()";
                auto impl = L"{ array<T, N> a; return a; }";
                sb.append(_template + L" " + _signature + L" " + impl);
                sb.endline();
            }

            array_dims.insert(N);
        }
    }
    if (array_dims.size() > 0)
    {
        sb.append_line();
    }
}

struct SparseSequence {
    std::set<uint32_t> used_numbers;
    
    bool TryAllocate(uint32_t number) {
        auto [it, inserted] = used_numbers.insert(number);
        return inserted; // 如果成功插入返回 true，否则返回 false
    }
    
    uint32_t Allocate() {
        uint32_t candidate = 0;
        for (uint32_t used : used_numbers) {
            if (used > candidate) {
                break; // 找到了空隙
            }
            candidate = used + 1;
        }
        used_numbers.insert(candidate);
        return candidate;
    }
    
    bool IsUsed(uint32_t number) const {
        return used_numbers.find(number) != used_numbers.end();
    }
};

struct BindTable {
    SparseSequence space_allocator;
    std::map<uint32_t, SparseSequence> register_allocators;
    uint32_t shared_space = UINT32_MAX; // 用于非 unique_space 的共享 space
    
    bool RegisterBinding(uint32_t space, uint32_t reg) 
    {
        // 1. 处理 space 分配
        if (space != UINT32_MAX && reg != UINT32_MAX) {
            // 1. 用户完全指定了 space 和 register
            bool success1 = space_allocator.TryAllocate(space);
            bool success2 = register_allocators[space].TryAllocate(reg);
            return success1 && success2;
        }
        return false;
    }

    std::pair<uint32_t, uint32_t> AllocateBinding(bool unique_space, uint32_t space, uint32_t reg) {
        uint32_t final_space = space;
        uint32_t final_register = reg;
        
        // 2. 自动分配 space
        if (space == UINT32_MAX) {
            reg = UINT32_MAX;
            if (unique_space) {
                // 2.1 unique_space: 每次分配新的 space
                final_space = space_allocator.Allocate();
            } else {
                // 2.1 非 unique_space: 使用共享 space
                if (shared_space == UINT32_MAX) {
                    // 第一次分配共享 space
                    shared_space = space_allocator.Allocate();
                }
                final_space = shared_space;
            }
        } else {
            // space 已指定，确保它被占用
            space_allocator.TryAllocate(space);
            final_space = space;
        }
        
        // 2.2 自动分配 register
        if (reg == UINT32_MAX) {
            final_register = register_allocators[final_space].Allocate();
        } else {
            // register 已指定，在对应 space 中占用
            register_allocators[final_space].TryAllocate(reg);
            final_register = reg;
        }
        
        return {final_space, final_register};
    }
};

void HLSLGenerator::GenerateSRTs(const AST& ast)
{
    // 清空绑定表
    binding_table_.clear();
    
    BindTable alloc_table;
    
    // 用于存储待分配的资源
    std::vector<const VarDecl*> regular_resources;
    std::vector<const VarDecl*> push_resources;
    std::vector<const VarDecl*> bindless_resources;
    std::map<const VarDecl*, std::pair<uint32_t, uint32_t>> fixed_bindings; // var -> (space, binding)
    std::map<const VarDecl*, std::pair<uint32_t, uint32_t>> auto_bindings; // var -> (space, binding)
    
    // 第一遍：收集所有全局资源并分类
    for (auto var : ast.global_vars())
    {
        const TypeDecl* varType = &var->type();
        if (auto asResource = varType->is_resource())
        {
            bool is_bindless = false;
            bool is_push = false;
            if (auto asArray = dynamic_cast<const ArrayTypeDecl*>(varType))
            {
                if (asArray->count() == 0 && dynamic_cast<const ResourceTypeDecl*>(asArray->element_type()))
                    is_bindless = true;
            }
            is_push = FindAttr<PushConstantAttr>(var->attrs());
            if (const auto resourceBind = FindAttr<ResourceBindAttr>(var->attrs()))
            {
                if (is_push)
                    push_resources.push_back(var);
                else if (is_bindless)
                    bindless_resources.push_back(var);
                else
                    regular_resources.push_back(var);

                uint32_t space = resourceBind->group();
                uint32_t binding = resourceBind->binding();
                
                // 检查是否有固定槽位
                if (space != ~0 && binding != ~0)
                {
                    alloc_table.RegisterBinding(space, binding);
                    fixed_bindings[var] = {space, binding};
                }
                else
                {
                    auto_bindings[var] = { space, binding };
                }
            }
        }

    }
    
    // 第一步：处理有固定槽位的资源
    for (const auto& [var, slot] : fixed_bindings)
    {
        binding_table_[var] = {slot.second, slot.first, false, false};
    }
    
    // 第二步：分配常规资源
    for (auto var : regular_resources)
    {
        if (const auto resourceBind = FindAttr<ResourceBindAttr>(var->attrs()))
        {
            uint32_t space = resourceBind->group();
            uint32_t binding = resourceBind->binding();
            if ((space != UINT32_MAX) || (binding == UINT32_MAX))
            {
                auto [new_space, new_binding] = alloc_table.AllocateBinding(false, space, binding);
                binding_table_[var] = {new_binding, new_space, false, false};
            }
        }
    }
    
    // 3.1 分配 bindless 资源
    for (auto var : bindless_resources)
    {
        if (const auto resourceBind = FindAttr<ResourceBindAttr>(var->attrs()))
        {
            uint32_t space = resourceBind->group();
            uint32_t binding = resourceBind->binding();
            if ((space != UINT32_MAX) || (binding == UINT32_MAX))
            {
                auto [new_space, new_binding] = alloc_table.AllocateBinding(true, space, binding);
                binding_table_[var] = {new_binding, new_space, false, true};
            }
        }
    }
    
    // 3.2 分配 push constant
    for (auto var : push_resources)
    {
        if (const auto resourceBind = FindAttr<ResourceBindAttr>(var->attrs()))
        {
            uint32_t space = resourceBind->group();
            uint32_t binding = resourceBind->binding();
            if ((space != UINT32_MAX) || (binding == UINT32_MAX))
            {
                auto [new_space, new_binding] = alloc_table.AllocateBinding(true, space, binding);
                binding_table_[var] = {new_binding, new_space, true, false};
            }
        }
    }

    // 第四步：检查 push 和 bindless 的 space 中是否有其他资源
    // 收集每个 space 中的资源
    std::map<uint32_t, std::vector<const VarDecl*>> space_resources;
    std::map<uint32_t, const VarDecl*> push_spaces;      // space -> push resource
    std::map<uint32_t, const VarDecl*> bindless_spaces;  // space -> bindless resource
    
    for (const auto& [var, binding_info] : binding_table_)
    {
        space_resources[binding_info.space].push_back(var);
        
        // 记录 push 和 bindless 占用的 space
        if (binding_info.is_push)
        {
            push_spaces[binding_info.space] = var;
        }
        else if (binding_info.is_bindless)
        {
            bindless_spaces[binding_info.space] = var;
        }
    }
    
    // 检查 push constant space 中是否有其他资源
    for (const auto& [space, push_var] : push_spaces)
    {
        const auto& resources_in_space = space_resources[space];
        if (resources_in_space.size() > 1)
        {
            // Push constant 的 space 中有其他资源，报错
            String error_msg = L"Push constant '" + push_var->name() + 
                             L"' at space " + std::to_wstring(space) + 
                             L" conflicts with other resources: ";
            for (auto res : resources_in_space)
            {
                if (res != push_var)
                {
                    error_msg += L"'" + res->name() + L"' ";
                }
            }
            ast.ReportFatalError(error_msg);
        }
    }
    
    // 检查 bindless resource space 中是否有其他资源
    for (const auto& [space, bindless_var] : bindless_spaces)
    {
        const auto& resources_in_space = space_resources[space];
        if (resources_in_space.size() > 1)
        {
            // Bindless resource 的 space 中有其他资源，报错
            String error_msg = L"Bindless resource '" + bindless_var->name() + 
                             L"' at space " + std::to_wstring(space) + 
                             L" conflicts with other resources: ";
            for (auto res : resources_in_space)
            {
                if (res != bindless_var)
                {
                    error_msg += L"'" + res->name() + L"' ";
                }
            }
            ast.ReportFatalError(error_msg);
        }
    }

}

} // namespace skr::CppSL::HLSL