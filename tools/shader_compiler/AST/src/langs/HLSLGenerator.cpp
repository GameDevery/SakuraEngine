#include "CppSL/langs/HLSLGenerator.hpp"

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
    return type->name();
}

void HLSLGenerator::VisitAccessExpr(SourceBuilderNew& sb, const AccessExpr* expr)
{
    auto to_access = dynamic_cast<const Expr*>(expr->children()[0]);
    auto index = dynamic_cast<const Expr*>(expr->children()[1]);
    visitExpr(sb, to_access);
    if (to_access->type()->is_array())
        sb.append(L".data");
    sb.append(L"[");
    visitExpr(sb, index);
    sb.append(L"]");
}

void HLSLGenerator::VisitGlobalResource(SourceBuilderNew& sb, const skr::CppSL::VarDecl* var)
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
        if (!reg_info.empty())
        {
            sb.append(L": " + reg_info);
        }
        sb.endline(L';');
    }
}

void HLSLGenerator::VisitBinaryExpr(SourceBuilderNew& sb, const BinaryExpr* binary)
{
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
        const auto N = AsArray->size() / AsArray->element()->size();
        sb.append(L"make_array" + std::to_wstring(N) + L"<" + GetQualifiedTypeName(AsArray->element()) + L", " + std::to_wstring(N) + L">(");
        ;
        for (size_t i = 0; i < ctorExpr->args().size(); i++)
        {
            auto arg = ctorExpr->args()[i];
            if (i > 0)
            {
                sb.append(L", ");
            }
            visitExpr(sb, arg);
        }
        sb.append(L")");
    }
    else
    {
        std::span<Expr* const> args;
        std::vector<Expr*> modified_args;
        int32_t fillVectorArgsWithZero = 0;
        if (auto AsVector = dynamic_cast<const VectorTypeDecl*>(ctorExpr->type());
            AsVector && ((ctorExpr->args().size() == 0) || (ctorExpr->args().size() == 1)))
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

        if (args.empty())
            args = ctorExpr->args();

        if (ctorExpr->type()->is_builtin())
            sb.append(GetQualifiedTypeName(ctorExpr->type()) + L"(");
        else
            sb.append(GetQualifiedTypeName(ctorExpr->type()) + L"::New(");

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
}

void HLSLGenerator::VisitVariable(SourceBuilderNew& sb, const skr::CppSL::VarDecl* varDecl)
{
    const auto isGlobal = dynamic_cast<const skr::CppSL::GlobalVarDecl*>(varDecl);
    if (varDecl->qualifier() == EVariableQualifier::Const)
        sb.append(isGlobal ? L"static const " : L"const ");
    else if (varDecl->qualifier() == EVariableQualifier::Inout)
        sb.append(L"inout ");

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
            visitExpr(sb, init);
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
template<typename T> T fract(T x){return x - floor(x);}

template <typename T, uint64_t N> struct array { T data[N]; };

template <typename T> void buffer_write(RWStructuredBuffer<T> buffer, uint index, T value) { buffer[index] = value; }
template <typename T> T buffer_read(RWStructuredBuffer<T> buffer, uint index) { return buffer[index]; }
template <typename T> T buffer_read(StructuredBuffer<T> buffer, uint index) { return buffer[index]; }

#define byte_buffer_load(b, i)  (b).Load((i))
#define byte_buffer_load2(b, i) (b).Load2((i))
#define byte_buffer_load3(b, i) (b).Load3((i))
#define byte_buffer_load4(b, i) (b).Load4((i))

#define byte_buffer_store(b, i, v)  (b).Store((i), (v))
#define byte_buffer_store2(b, i, v) (b).Store2((i), (v))
#define byte_buffer_store3(b, i, v) (b).Store3((i), (v))
#define byte_buffer_store4(b, i, v) (b).Store4((i), (v))

template <typename T> T byte_buffer_read(ByteAddressBuffer b, uint i) { return b.Load<T>(i); }
template <typename T> T byte_buffer_read(RWByteAddressBuffer b, uint i) { return b.Load<T>(i); }
template <typename T> void byte_buffer_write(RWByteAddressBuffer b, uint i, T v) { b.Store<T>(i, v); }

// template <typename TEX> float4 texture2d_sample(TEX tex, uint2 uv, uint filter, uint address) { return float4(1, 1, 1, 1); }
// template <typename TEX> float4 texture3d_sample(TEX tex, uint3 uv, uint filter, uint address) { return float4(1, 1, 1, 1); }

template <typename T> T texture_read(Texture2D<T> tex, uint2 loc) { return tex.Load(uint3(loc, 0)); }
template <typename T> T texture_read(RWTexture2D<T> tex, uint2 loc) { return tex.Load(uint3(loc, 0)); }
template <typename T> T texture_read(Texture2D<T> tex, uint3 loc_and_mip) { return tex.Load(loc_and_mip); }
template <typename T> T texture_read(RWTexture2D<T> tex, uint3 loc_and_mip) { return tex.Load(loc_and_mip); }
template <typename T> T texture_write(RWTexture2D<T> tex, uint2 uv, T v) { return tex[uv] = v; }

template <typename T> uint2 texture_size(Texture2D<T> tex) { uint Width, Height, Mips; tex.GetDimensions(0, Width, Height, Mips); return uint2(Width, Height); }
template <typename T> uint2 texture_size(RWTexture2D<T> tex) { uint Width, Height; tex.GetDimensions(Width, Height); return uint2(Width, Height); }
template <typename T> uint3 texture_size(Texture3D<T> tex) { uint Width, Height, Depth, Mips; tex.GetDimensions(0, Width, Height, Depth, Mips); return uint3(Width, Height, Depth); }
template <typename T> uint3 texture_size(RWTexture3D<T> tex) { uint Width, Height, Depth; tex.GetDimensions(Width, Height, Depth); return uint3(Width, Height, Depth); }

float4 sample2d(SamplerState s, Texture2D t, float2 uv) { return t.Sample(s, uv); }

using AccelerationStructure = RaytracingAccelerationStructure;
using uint64 = uint64_t;

RayDesc create_ray(float3 origin, float3 dir, float tmin, float tmax) { RayDesc r; r.Origin = origin; r.Direction = dir; r.TMin = tmin; r.TMax = tmax; return r; }
#define ray_query_trace_ray_inline(q, as, mask, ray) (q).TraceRayInline((as), RAY_FLAG_NONE, (mask), create_ray((ray).origin(), (ray).dir(), (ray).tmin(), (ray).tmax()))
#define ray_query_proceed(q) (q).Proceed()
#define ray_query_committed_status(q) (q).CommittedStatus()
#define ray_query_committed_triangle_bary(q) (q).CommittedTriangleBarycentrics()
#define ray_query_committed_instance_id(q) (q).CommittedInstanceID()
#define ray_query_committed_ray_t(q) (q).CommittedRayT()
#define ray_query_world_ray_origin(q) (q).WorldRayOrigin()
#define ray_query_world_ray_direction(q) (q).WorldRayDirection()
)";

void HLSLGenerator::RecordBuiltinHeader(SourceBuilderNew& sb, const AST& ast)
{
    sb.append(kHLSLHeader);
    sb.endline();

    GenerateArrayHelpers(sb, ast);
}

void HLSLGenerator::GenerateArrayHelpers(SourceBuilderNew& sb, const AST& ast)
{
    std::set<uint32_t> array_dims;
    for (auto&& [element, array] : ast.array_types())
    {
        const auto N = array->size() / element.first->size();
        if (!array_dims.contains(N))
        {
            String args = L"";
            String exprs = L"";
            for (uint32_t i = 0; i < N; i++)
            {
                if (i > 0)
                    args += L", ";
                args += L"T a" + std::to_wstring(i) + L" = (T)0";
                exprs += L"a.data[" + std::to_wstring(i) + L"] = a" + std::to_wstring(i) + L";";
            }
            String _template = L"template <typename T, uint64_t N> array<T, N>";
            auto _signature = L"make_array" + std::to_wstring(N) + L"(" + args + L")";
            auto impl = L"{ array<T, N> a; " + exprs + L"; return a; }";
            sb.append(_template + L" " + _signature + L" " + impl);
            sb.endline();

            array_dims.insert(N);
        }
    }
    if (array_dims.size() > 0)
    {
        sb.append_line();
    }
}
} // namespace skr::CppSL::HLSL