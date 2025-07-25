#include "CppSL/langs/MSLGenerator.hpp"

namespace skr::CppSL::MSL
{
inline static String GetStageName(ShaderStage stage)
{
    switch (stage)
    {
    case ShaderStage::Vertex:
        return L"vertex";
    case ShaderStage::Fragment:
        return L"fragment";
    case ShaderStage::Compute:
        return L"kernel";
    default:
        assert(false && "Unknown shader stage");
        return L"unknown_stage";
    }
}

static const std::unordered_map<InterpolationMode, String> InterpolationMap = {
    { InterpolationMode::linear, L"[[perspective]]" },
    { InterpolationMode::nointerpolation, L"[[flat]]" },
    { InterpolationMode::centroid, L"[[centroid_perspective]]" },
    { InterpolationMode::sample, L"[[sample_perspective]]" },
    { InterpolationMode::noperspective, L"[[center_no_perspective]]" }
};
static const String UnknownInterpolation = L"[[UnknownInterpolation]]";
inline static const String& GetInterpolationString(InterpolationMode Interpolation)
{
    auto it = InterpolationMap.find(Interpolation);
    if (it != InterpolationMap.end())
    {
        return it->second;
    }
    return UnknownInterpolation;
}

static const std::unordered_map<SemanticType, String> SystemValueMap = {
    { SemanticType::Invalid, L"" },
    { SemanticType::Position, L"[[position]]" },
    { SemanticType::ClipDistance, L"[[clip_distance]]" },
    { SemanticType::CullDistance, L"[[cull_distance]]" },
    
    { SemanticType::RenderTarget0, L"[[color(0)]]" },
    { SemanticType::RenderTarget1, L"[[color(1)]]" },
    { SemanticType::RenderTarget2, L"[[color(2)]]" },
    { SemanticType::RenderTarget3, L"[[color(3)]]" },
    { SemanticType::RenderTarget4, L"[[color(4)]]" },
    { SemanticType::RenderTarget5, L"[[color(5)]]" },
    { SemanticType::RenderTarget6, L"[[color(6)]]" },
    { SemanticType::RenderTarget7, L"[[color(7)]]" },
    
    { SemanticType::Depth, L"[[depth(any)]]" },
    { SemanticType::DepthGreaterEqual, L"[[depth(greater)]]" },
    { SemanticType::DepthLessEqual, L"[[depth(less)]]" },
    { SemanticType::StencilRef, L"[[stencil]]" },
    
    { SemanticType::VertexID, L"[[vertex_id]]" },
    { SemanticType::InstanceID, L"[[instance_id]]" },
    
    { SemanticType::GSInstanceID, L"[[render_target_array_index]]" },
    { SemanticType::TessFactor, L"[[patch_control_point]]" },
    { SemanticType::InsideTessFactor, L"[[patch_control_point]]" },
    { SemanticType::DomainLocation, L"[[position_in_patch]]" },
    { SemanticType::ControlPointID, L"[[patch_id]]" },
    
    { SemanticType::PrimitiveID, L"[[primitive_id]]" },
    { SemanticType::IsFrontFace, L"[[front_facing]]" },
    { SemanticType::SampleIndex, L"[[sample_id]]" },
    { SemanticType::SampleMask, L"[[sample_mask]]" },
    { SemanticType::Barycentrics, L"[[barycentric_coord]]" },
    
    { SemanticType::ThreadID, L"[[thread_position_in_grid]]" },
    { SemanticType::GroupID, L"[[threadgroup_position_in_grid]]" },
    { SemanticType::ThreadPositionInGroup, L"[[thread_position_in_threadgroup]]" },
    { SemanticType::ThreadIndexInGroup, L"[[thread_index_in_threadgroup]]" },
    
    { SemanticType::ViewID, L"[[render_target_array_index]]" }
};
static const String UnknownSystemValue = L"[[UnknownSystemValue]]";
inline static const String& GetSystemValueString(SemanticType Semantic)
{
    auto it = SystemValueMap.find(Semantic);
    if (it != SystemValueMap.end())
    {
        return it->second;
    }
    return UnknownSystemValue;
}

String MSLGenerator::GetTypeName(const TypeDecl* type)
{
    if (auto asArray = dynamic_cast<const ArrayTypeDecl*>(type))
    {
        return L"metal::" + asArray->name();
    }
    else if (auto asMatrix = dynamic_cast<const MatrixTypeDecl*>(type))
    {
        return L"metal::" + asMatrix->name();
    }
    return type->name();
}

void MSLGenerator::VisitGlobalResource(SourceBuilderNew& sb, const skr::CppSL::VarDecl* var)
{
    // null option
    // since we have already extract and dumped SRT data as builtin header
}

void MSLGenerator::VisitVariable(SourceBuilderNew& sb, const skr::CppSL::VarDecl* varDecl) 
{
    const auto isGlobal = dynamic_cast<const skr::CppSL::GlobalVarDecl*>(varDecl);
    auto _typename = GetQualifiedTypeName(&varDecl->type());
    String prefix = L"", postfix = L"";
    if (varDecl->qualifier() == EVariableQualifier::Const)
    {
        prefix = isGlobal ? L"static constant " : L"const ";
    }
    else if ((varDecl->qualifier() == EVariableQualifier::Inout) || (varDecl->qualifier() == EVariableQualifier::Out))
    {
        prefix = L"thread ";
        postfix = L"&";
    }
    sb.append( std::format(L"{}{}{} {}", prefix, _typename, postfix, varDecl->name()));
    if (auto init = varDecl->initializer())
    {
        if (auto asRayQuery = dynamic_cast<const RayQueryTypeDecl*>(init->type()))
        {

        }
        else
        {
            sb.append(L" = ");
            visitExpr(sb, init);
        }
    }
}

void MSLGenerator::VisitParameter(SourceBuilderNew& sb, const skr::CppSL::FunctionDecl* funcDecl, const skr::CppSL::ParamVarDecl* param)
{
    const StageAttr* StageEntry = FindAttr<StageAttr>(funcDecl->attrs());
    auto AsSemantic = FindAttr<SemanticAttr>(param->attrs());
    auto qualifier = param->qualifier();

    String semantic_string = L"";
    if (StageEntry && AsSemantic)
    {
        if (SemanticAttr::GetSemanticQualifier(AsSemantic->semantic(), StageEntry->stage(), qualifier))
        {
            semantic_string = L" " + GetSystemValueString(AsSemantic->semantic());
        }
        else
        {
            param->ast().ReportFatalError(std::format(L"Invalid semantic {} for param {} within stage {}",
                GetSystemValueString(AsSemantic->semantic()),
                param->name(),
                (uint32_t)StageEntry->stage()));
        }
    }

    String prefix = L"", postfix = L"";
    switch (qualifier)
    {
    case EVariableQualifier::Const:
        prefix = L"const ";
        break;
    case EVariableQualifier::Out:
        prefix = !AsSemantic ? L"thread " : L"";
        postfix = !AsSemantic ? L"&" : L"";
        break;
    case EVariableQualifier::Inout:
        prefix = !AsSemantic ? L"thread " : L"";
        postfix = !AsSemantic ? L"&" : L"";
        break;
    case EVariableQualifier::None:
        prefix = L"";
        break;
    }
    String content = prefix + GetQualifiedTypeName(&param->type()) + postfix + L" " + param->name();
    sb.append(content);
    sb.append(semantic_string);
}

void MSLGenerator::VisitField(SourceBuilderNew& sb, const skr::CppSL::TypeDecl* typeDecl, const skr::CppSL::FieldDecl* field)
{
    const bool IsStageInout = FindAttr<StageInoutAttr>(typeDecl->attrs());
    
    // Normal field handling
    sb.append(GetQualifiedTypeName(&field->type()) + L" " + field->name());

    if (auto interpolation = FindAttr<InterpolationAttr>(field->attrs()))
        sb.append(GetInterpolationString(interpolation->mode()));

    if (IsStageInout)
        sb.append(L"[[user(" + field->name() + L")]]");

    sb.endline(L';');
}

void MSLGenerator::VisitDeclRef(SourceBuilderNew& sb, const DeclRefExpr* declRef)
{
    if (auto var = dynamic_cast<const VarDecl*>(declRef->decl()))
    {
        auto set = set_of_vars.find(var);
        if (set != set_of_vars.end())
        {
            if (auto cbuffer = dynamic_cast<const ConstantBufferTypeDecl*>(&var->type()))
                sb.append(std::format(L"srt{}.{}.cgpu_buffer_data", set->second, var->name()));
            else
                sb.append(std::format(L"srt{}.{}", set->second, var->name()));
        }
        else
        {
            sb.append(var->name());
        }
    }
}

void MSLGenerator::VisitConstructExpr(SourceBuilderNew& sb, const ConstructExpr* ctorExpr)
{
    std::span<Expr* const> args;

    if (args.empty())
        args = ctorExpr->args();

    sb.append(GetQualifiedTypeName(ctorExpr->type()) + L"(");
    for (size_t i = 0; i < args.size(); i++)
    {
        auto arg = args[i];
        if (i > 0)
        {
            sb.append(L", ");
        }
        visitExpr(sb, arg);
    }
    sb.append(L")");
}

void MSLGenerator::GenerateFunctionAttributes(SourceBuilderNew& sb, const FunctionDecl* funcDecl)
{
    if (const StageAttr* StageEntry = FindAttr<StageAttr>(funcDecl->attrs()))
    {
        sb.append(GetStageName(StageEntry->stage()));
        sb.append(L" ");
    }
}

static const skr::CppSL::String kMSLHeader = LR"(
#include <metal_stdlib>
#include <simd/simd.h>

template <typename T> struct ConstantBuffer { constant T& cgpu_buffer_data; };

template <typename T, metal::access a> struct Buffer;
template <typename T> struct Buffer<T, metal::access::read> { constant T* cgpu_buffer_data; uint64_t cgpu_buffer_size; };
template <typename T> struct Buffer<T, metal::access::read_write> { device T* cgpu_buffer_data; uint64_t cgpu_buffer_size; };
template <typename T> using RWStructuredBuffer = Buffer<T, metal::access::read_write>;

template <typename T> void buffer_write(RWStructuredBuffer<T> buffer, uint index, T value) { buffer.cgpu_buffer_data[index] = value; }
template <typename T> T buffer_read(RWStructuredBuffer<T> buffer, uint index) { return buffer.cgpu_buffer_data[index]; }

template <typename T, metal::access a = metal::access::read>
struct Texture2D { metal::texture2d<T, a> cgpu_texture; };

struct SamplerState { metal::sampler cgpu_sampler; };

// Math intrinsics
using metal::abs; using metal::min; using metal::max; using metal::clamp; using metal::all; using metal::any; using metal::select;
using metal::sin; using metal::sinh; using metal::cos; using metal::cosh; using metal::atan; using metal::atanh; using metal::tan; using metal::tanh;
using metal::acos; using metal::acosh; using metal::asin; using metal::asinh; using metal::exp; using metal::exp2; using metal::log; using metal::log2;
using metal::log10; using metal::exp10; using metal::sqrt; using metal::rsqrt; using metal::ceil; using metal::floor; using metal::fract; using metal::trunc;
using metal::round; using metal::length; using metal::saturate; using metal::pow;
using metal::copysign; using metal::atan2; using metal::step; using metal::fma; using metal::smoothstep; using metal::normalize; using metal::dot; using metal::cross;
using metal::faceforward; using metal::reflect; using metal::transpose; using metal::determinant;

// Integer intrinsics  
using metal::clz; using metal::ctz; using metal::popcount;

// Function aliases
template<typename T> T lerp(T a, T b, T t) { return metal::mix(a, b, t); }
template<typename T> T ddx(T p) { return metal::dfdx(p); }
template<typename T> T ddy(T p) { return metal::dfdy(p); }
template<typename T> auto is_inf(T x) { return metal::isinf(x); }
template<typename T> auto is_nan(T x) { return metal::isnan(x); }
template<typename T> auto length_squared(T v) { return metal::dot(v, v); }

// Texture sampling functions
template<typename T, metal::access a>
metal::vec<T, 4> sample2d(SamplerState sampler, Texture2D<T, a> texture, float2 uv) { return texture.cgpu_texture.sample(sampler.cgpu_sampler, uv); }

)";

void MSLGenerator::RecordBuiltinHeader(SourceBuilderNew& sb, const AST& ast)
{
    sb.append(kMSLHeader);
    sb.endline();

    GenerateSRTs(sb, ast);
}

void MSLGenerator::GenerateKernelWrapper(SourceBuilderNew& sb, const skr::CppSL::FunctionDecl* funcDecl)
{

}

void MSLGenerator::GenerateSRTs(SourceBuilderNew& sb, const AST& ast)
{
    using SRT = std::map<uint32_t, const VarDecl*>;
    std::map<uint32_t, SRT> srts;
    SRT anonymous_srt;
    uint32_t anonymous_srt_bind = 0;
    for (auto var : ast.global_vars())
    {
        if (auto asResource = dynamic_cast<const ResourceTypeDecl*>(&var->type()))
        {
            uint32_t set = 0, bind = 0;
            if (const auto resourceBind = FindAttr<ResourceBindAttr>(var->attrs()))
            {
                set = resourceBind->group();
                bind = resourceBind->binding();

                if ((set == ~0) && (bind == ~0))
                    anonymous_srt.insert({anonymous_srt_bind++, var});
                else
                    srts[set][bind] = var;
            }
            else
            {
                anonymous_srt.insert({anonymous_srt_bind++, var});
            }
        }
    }

    auto gen_srt = [&](uint32_t set, const SRT& srt){
        auto SRTTypeName = L"SRT" + std::to_wstring(set);
        auto SRTVarName = L"srt" + std::to_wstring(set);
        sb.append(L"struct " + SRTTypeName+ L" {");
        sb.endline();
        sb.indent([&](){
            for (const auto& [bind, resource] : srt)
            {
                sb.append(GetTypeName(&resource->type()));
                sb.append(L" ");
                sb.append(resource->name());
                sb.endline(L';');

                set_of_vars[resource] = set;
            }
        });
        sb.append(L"};");
        sb.endline();

        sb.append(
            std::format(L"device {}& constant {} [[buffer({})]]", SRTTypeName, SRTVarName, set)
        );
        sb.endline(L';');
    };

    uint32_t next_set = 0;
    for (const auto& [set, srt] : srts)
    {
        gen_srt(set, srt);
        next_set = set + 1;
    }
    if (!anonymous_srt.empty())
        gen_srt(next_set, anonymous_srt);
}

} // end namespace skr::CppSL::MSL
