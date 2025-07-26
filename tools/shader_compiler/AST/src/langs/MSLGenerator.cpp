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
    else if (auto asCB = dynamic_cast<const ConstantBufferTypeDecl*>(type))
    {
        return type->name();
    }
    return type->name();
}

String MSLGenerator::GetFunctionName(const FunctionDecl* funcDecl)
{
    if (const StageAttr* StageEntry = FindAttr<StageAttr>(funcDecl->attrs()))
    {
        return funcDecl->name() + L"____impl____";
    }
    return funcDecl->name();
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
 
// MSLGenerator generates semantics at wrapper so we need not deal with system value attributes here
void MSLGenerator::VisitParameter(SourceBuilderNew& sb, const skr::CppSL::FunctionDecl* funcDecl, const skr::CppSL::ParamVarDecl* param)
{
    auto qualifier = param->qualifier();
    String prefix = L"", postfix = L"";
    switch (qualifier)
    {
    case EVariableQualifier::Const:
        prefix = L"const ";
        break;
    case EVariableQualifier::Out:
        prefix = L"thread ";
        postfix = L"&";
        break;
    case EVariableQualifier::Inout:
        prefix = L"thread ";
        postfix = L"&";
        break;
    case EVariableQualifier::None:
        prefix = L"";
        break;
    }
    String content = prefix + GetQualifiedTypeName(&param->type()) + postfix + L" " + param->name();
    sb.append(content);
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
            // Check if this is a push constant
            if (auto cbuffer = dynamic_cast<const ConstantBufferTypeDecl*>(&var->type()))
            {
                sb.append(std::format(L"srt{}.{}.cgpu_buffer_data", set->second, var->name()));
            }
            else
            {
                sb.append(std::format(L"srt{}.{}", set->second, var->name()));
            }
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

    if (auto AsArray = dynamic_cast<const ArrayTypeDecl*>(ctorExpr->type()))
    {
        const auto N = AsArray->size() / AsArray->element()->size();
        sb.append(GetTypeName(AsArray) + L"{");
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
        sb.append(L"}");
    }
    else
    {
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
}

void MSLGenerator::BeforeGenerateFunctionImplementations(SourceBuilderNew& sb, const AST& ast)
{
    GenerateSRTs(sb, ast);
}

static const skr::CppSL::String kMSLHeader = LR"(
#include <metal_stdlib>
#include <simd/simd.h>

template <typename T> struct ConstantBuffer { constant T& cgpu_buffer_data; };
template <typename T> struct PushConstant { T cgpu_buffer_data; };

template <typename T, metal::access a> struct Buffer;
template <typename T> struct Buffer<T, metal::access::read> { constant T* cgpu_buffer_data; uint64_t cgpu_buffer_size; };
template <typename T> struct Buffer<T, metal::access::read_write> { device T* cgpu_buffer_data; uint64_t cgpu_buffer_size; };
template <typename T> using RWStructuredBuffer = Buffer<T, metal::access::read_write>;

template <typename T> void buffer_write(RWStructuredBuffer<T> buffer, uint index, T value) { buffer.cgpu_buffer_data[index] = value; }
template <typename T> T buffer_read(RWStructuredBuffer<T> buffer, uint index) { return buffer.cgpu_buffer_data[index]; }

template <typename T, metal::access a = metal::access::sample> struct Texture2D { metal::texture2d<T, a> cgpu_texture; };
template <typename T> using RWTexture2D = Texture2D<T, metal::access::write>;
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

// Texture functions
template<typename T>
metal::vec<T, 4> sample2d(SamplerState sampler, Texture2D<T, metal::access::sample> texture, float2 uv) { 
    return texture.cgpu_texture.sample(sampler.cgpu_sampler, uv); 
}

template<typename T>
metal::vec<T, 4> texture_read(Texture2D<T, metal::access::read> texture, uint2 coord) {
    return texture.cgpu_texture.read(coord);
}

template<typename T>
metal::vec<T, 4> texture_read(Texture2D<T, metal::access::sample> texture, uint2 coord) {
    return texture.cgpu_texture.read(coord);
}

template<typename T>
void texture_write(Texture2D<T, metal::access::write> texture, uint2 coord, metal::vec<T, 4> value) {
    texture.cgpu_texture.write(value, coord);
}

// Ray tracing support
struct AccelerationStructure { metal::raytracing::instance_acceleration_structure as; };
using QueryFlags = uint32_t;

template <QueryFlags f>
struct RayQuery {
    thread metal::raytracing::intersection_query<metal::raytracing::triangle_data, metal::raytracing::instancing>* query;
    metal::raytracing::ray current_ray;
    bool initialized = false;
};

// Ray query macros
// Ray query initialization - creates intersection_query on the stack
#define ray_query_trace_ray_inline(q, _as, mask, r) \
    float3 _ray_origin_##__LINE__ = (r).origin(); \
    float3 _ray_direction_##__LINE__ = (r).dir(); \
    (q).current_ray = metal::raytracing::ray(_ray_origin_##__LINE__, _ray_direction_##__LINE__, (r).tmin(), (r).tmax()); \
    metal::raytracing::intersection_query<metal::raytracing::triangle_data, metal::raytracing::instancing> _query_##__LINE__((q).current_ray, (_as).as, mask); \
    (q).query = &_query_##__LINE__; \
    (q).initialized = true

#define ray_query_proceed(q) \
    ([&]() { \
        bool has_candidate = (q).query->next(); \
        if (has_candidate && (q).query->get_candidate_intersection_type() == metal::raytracing::intersection_type::triangle) { \
            (q).query->commit_triangle_intersection(); \
        } \
        return has_candidate; \
    }())

#define ray_query_committed_status(q) \
    ((q).query->get_committed_intersection_type() == metal::raytracing::intersection_type::triangle ? HitType__HitTriangle : HitType__Miss)

#define ray_query_committed_triangle_bary(q) \
    float2((q).query->get_committed_triangle_barycentric_coord().x, (q).query->get_committed_triangle_barycentric_coord().y)

#define ray_query_committed_instance_id(q) \
    ((q).query->get_committed_instance_id())

#define ray_query_committed_primitive_id(q) \
    ((q).query->get_committed_primitive_id())

#define ray_query_committed_ray_t(q) \
    ((q).query->get_committed_distance())

#define ray_query_world_ray_origin(q) \
    float3((q).current_ray.origin)

#define ray_query_world_ray_direction(q) \
    float3((q).current_ray.direction)
)";

void MSLGenerator::RecordBuiltinHeader(SourceBuilderNew& sb, const AST& ast)
{
    sb.append(kMSLHeader);
    sb.endline();
}

void MSLGenerator::GenerateKernelWrapper(SourceBuilderNew& sb, const skr::CppSL::FunctionDecl* funcDecl)
{
    const StageAttr* stageAttr = FindAttr<StageAttr>(funcDecl->attrs());
    if (!stageAttr)
        return;
    
    // Step 1: Extract all parameters and flatten struct fields
    struct FieldInfo {
        const ParamVarDecl* param;
        const FieldDecl* field;  // null if not from struct
        String fullName;
        EVariableQualifier qualifier;
        const TypeDecl* type;
        SemanticType semantic;
        bool hasInterpolation;
        InterpolationMode interpolation;
    };
    
    std::vector<FieldInfo> allFields;
    std::vector<const ParamVarDecl*> resourceParams;
    
    // Extract fields from all parameters
    for (auto param : funcDecl->parameters())
    {
        if (auto structType = dynamic_cast<const StructureTypeDecl*>(&param->type()))
        {
            if (dynamic_cast<const ResourceTypeDecl*>(structType))
            {
                // Resource parameters stay as-is
                resourceParams.push_back(param);
            }
            else if (FindAttr<StageInoutAttr>(structType->attrs()))
            {
                // Flatten struct fields for stage_inout structs
                for (auto field : structType->fields())
                {
                    FieldInfo info;
                    info.param = param;
                    info.field = field;
                    info.fullName = param->name() + L"_" + field->name();
                    info.qualifier = param->qualifier();
                    info.type = &field->type();
                    
                    // Get semantic from field
                    if (auto semantic = FindAttr<SemanticAttr>(field->attrs()))
                    {
                        info.semantic = semantic->semantic();
                    }
                    else
                    {
                        info.semantic = SemanticType::Invalid;
                    }
                    
                    // Get interpolation from field
                    if (auto interp = FindAttr<InterpolationAttr>(field->attrs()))
                    {
                        info.hasInterpolation = true;
                        info.interpolation = interp->mode();
                    }
                    else
                    {
                        info.hasInterpolation = false;
                    }
                    
                    allFields.push_back(info);
                }
            }
        }
        else if (dynamic_cast<const ResourceTypeDecl*>(&param->type()))
        {
            // Resource parameters stay as-is
            resourceParams.push_back(param);
        }
        else
        {
            // Non-struct parameters
            FieldInfo info;
            info.param = param;
            info.field = nullptr;
            info.fullName = param->name();
            info.qualifier = param->qualifier();
            info.type = &param->type();
            
            // Get semantic from parameter
            if (auto semantic = FindAttr<SemanticAttr>(param->attrs()))
            {
                info.semantic = semantic->semantic();
            }
            else
            {
                info.semantic = SemanticType::Invalid;
            }
            
            info.hasInterpolation = false;
            
            allFields.push_back(info);
        }
    }
    
    // Always generate wrapper for consistency
    
    // Step 2: Categorize fields based on semantics and qualifiers
    std::vector<FieldInfo> inputFields;
    std::vector<FieldInfo> outputFields;
    std::vector<FieldInfo> standaloneFields; // Fields that can't be in structs (compute shader attributes)
    
    // Check if certain semantics must be standalone (can't be in structs)
    auto isStandaloneSemantic = [](SemanticType semantic, ShaderStage stage) -> bool {
        if (stage == ShaderStage::Vertex)
            return semantic == SemanticType::VertexID;
        if (stage == ShaderStage::Fragment)
            return true;
        if (stage == ShaderStage::Compute)
        {
            // Compute shader thread/group attributes must be standalone
            return semantic == SemanticType::ThreadID ||
                   semantic == SemanticType::GroupID ||
                   semantic == SemanticType::ThreadPositionInGroup ||
                   semantic == SemanticType::ThreadIndexInGroup;
        }
        return false;
    };
    
    for (const auto& field : allFields)
    {
        // Check if this field must be standalone
        if (field.semantic != SemanticType::Invalid && 
            isStandaloneSemantic(field.semantic, stageAttr->stage()))
        {
            standaloneFields.push_back(field);
            continue;
        }
        
        bool isInput = false;
        bool isOutput = false;
        
        // Use SemanticAttr::GetSemanticQualifier to determine direction
        if (field.semantic != SemanticType::Invalid)
        {
            EVariableQualifier semanticQual = field.qualifier;
            if (SemanticAttr::GetSemanticQualifier(field.semantic, stageAttr->stage(), semanticQual))
            {
                if (semanticQual == EVariableQualifier::None || semanticQual == EVariableQualifier::Const)
                {
                    isInput = true;
                }
                else if (semanticQual == EVariableQualifier::Out || semanticQual == EVariableQualifier::Inout)
                {
                    isOutput = true;
                }
                if (semanticQual == EVariableQualifier::Inout)
                {
                    isInput = true;
                }
            }
        }
        else
        {
            // No semantic, use parameter qualifier
            if (field.qualifier == EVariableQualifier::None || field.qualifier == EVariableQualifier::Const)
            {
                isInput = true;
            }
            else if (field.qualifier == EVariableQualifier::Out || field.qualifier == EVariableQualifier::Inout)
            {
                isOutput = true;
            }
            if (field.qualifier == EVariableQualifier::Inout)
            {
                isInput = true;
            }
        }
        
        if (isInput)
            inputFields.push_back(field);
        if (isOutput)
            outputFields.push_back(field);
    }
    
    // Generate input struct
    String inputStructName = funcDecl->name() + L"_in";
    if (!inputFields.empty())
    {
        sb.append(L"struct ");
        sb.append(inputStructName);
        sb.endline(L" {");
        sb.indent([&]() {
            uint32_t attrIndex = 0;
            for (const auto& field : inputFields)
            {
                sb.append(GetQualifiedTypeName(field.type));
                sb.append(L" ");
                sb.append(field.fullName);
                
                // Add attributes
                if (field.semantic != SemanticType::Invalid)
                {
                    sb.append(L" ");
                    sb.append(GetSystemValueString(field.semantic));
                }
                else if (stageAttr->stage() == ShaderStage::Vertex)
                {
                    sb.append(L" [[attribute(");
                    sb.append(std::to_wstring(attrIndex++));
                    sb.append(L")]]");
                }
                else
                {
                    // Fragment/compute inputs need user() attribute
                    if (field.hasInterpolation)
                    {
                        sb.append(L" ");
                        sb.append(GetInterpolationString(field.interpolation));
                    }
                    sb.append(L" [[user(");
                    sb.append(field.fullName);
                    sb.append(L")]]");
                }
                
                sb.append(L";");
                sb.endline();
            }
        });
        sb.append(L"};");
        sb.endline();
    }
    
    // Generate output struct
    String outputStructName = funcDecl->name() + L"_out";
    bool hasOutput = !outputFields.empty() || funcDecl->return_type()->name() != L"void";
    
    if (hasOutput)
    {
        sb.append(L"struct ");
        sb.append(outputStructName);
        sb.endline(L" {");
        sb.indent([&]() {
            // Add output fields
            for (const auto& field : outputFields)
            {
                sb.append(GetQualifiedTypeName(field.type));
                sb.append(L" ");
                sb.append(field.fullName);
                
                // Add attributes
                if (field.hasInterpolation)
                {
                    sb.append(L" ");
                    sb.append(GetInterpolationString(field.interpolation));
                }
                
                if (field.semantic != SemanticType::Invalid)
                {
                    sb.append(L" ");
                    sb.append(GetSystemValueString(field.semantic));
                }
                else
                {
                    sb.append(L" [[user(");
                    sb.append(field.fullName);
                    sb.append(L")]]");
                }
                
                sb.append(L";");
                sb.endline();
            }
            
            // Add return value if not void
            if (funcDecl->return_type()->name() != L"void")
            {
                // Check if return type is a struct that needs to be flattened
                if (auto returnStruct = dynamic_cast<const StructureTypeDecl*>(funcDecl->return_type()))
                {
                    // Flatten struct fields from return value
                    for (auto field : returnStruct->fields())
                    {
                        sb.append(GetQualifiedTypeName(&field->type()));
                        sb.append(L" result_");
                        sb.append(field->name());
                        
                        // Add attributes from field
                        if (auto interpolation = FindAttr<InterpolationAttr>(field->attrs()))
                        {
                            sb.append(L" ");
                            sb.append(GetInterpolationString(interpolation->mode()));
                        }
                        
                        if (auto semantic = FindAttr<SemanticAttr>(field->attrs()))
                        {
                            sb.append(L" ");
                            sb.append(GetSystemValueString(semantic->semantic()));
                        }
                        else
                        {
                            sb.append(L" [[user(result_");
                            sb.append(field->name());
                            sb.append(L")]]");
                        }
                        
                        sb.append(L";");
                        sb.endline();
                    }
                }
                else
                {
                    // Non-struct return type
                    sb.append(GetQualifiedTypeName(funcDecl->return_type()));
                    sb.append(L" result");
                    
                    // Add semantic for return value
                    if (stageAttr->stage() == ShaderStage::Fragment)
                    {
                        sb.append(L" [[color(0)]]");
                    }
                    else if (stageAttr->stage() == ShaderStage::Vertex)
                    {
                        sb.append(L" [[position]]");
                    }
                    
                    sb.append(L";");
                    sb.endline();
                }
            }
        });
        sb.append(L"};");
        sb.endline();
    }
    
    // Generate wrapper function
    sb.append(GetStageName(stageAttr->stage()));
    sb.append(L" ");
    sb.append(hasOutput ? outputStructName : L"void");
    sb.append(L" ");
    sb.append(funcDecl->name());
    sb.append(L"(");
    
    bool first = true;
    
    // Add input struct parameter
    if (!inputFields.empty())
    {
        sb.append(inputStructName);
        sb.append(L" in [[stage_in]]");
        first = false;
    }
    
    // Add standalone parameters (compute shader thread/group attributes)
    for (const auto& field : standaloneFields)
    {
        if (!first) sb.append(L", ");
        first = false;
        
        sb.append(GetQualifiedTypeName(field.type));
        sb.append(L" ");
        sb.append(field.fullName);
        sb.append(L" ");
        sb.append(GetSystemValueString(field.semantic));
    }
    
    // Add resource parameters
    for (auto param : resourceParams)
    {
        if (!first) sb.append(L", ");
        first = false;
        VisitParameter(sb, funcDecl, param);
    }
    
    // Add SRT parameters as placeholders for debugger
    std::set<uint32_t> used_sets;
    for (const auto& [var, set] : set_of_vars)
    {
        used_sets.insert(set);
    }
    
    for (uint32_t set : used_sets)
    {
        if (!first) sb.append(L", ");
        first = false;
        
        auto SRTTypeName = L"SRT" + std::to_wstring(set);
        auto SRTVarName = L"srt" + std::to_wstring(set);
        
        sb.append(L"constant ");
        sb.append(SRTTypeName);
        sb.append(L"& ");
        sb.append(SRTVarName);
        sb.append(L" [[buffer(");
        sb.append(std::to_wstring(set));
        sb.append(L")]]");
    }
    
    sb.append(L")");
    sb.endline();
    sb.append(L"{");
    sb.endline();
    sb.indent([&]() {
        // Initialize output struct if needed
        if (hasOutput)
        {
            sb.append(outputStructName);
            sb.append(L" out = {};");
            sb.endline();
        }
        
        // Prepare struct parameters
        sb.append(L"// Reconstruct struct parameters");
        sb.endline();
        for (auto param : funcDecl->parameters())
        {
            if (auto structType = dynamic_cast<const StructureTypeDecl*>(&param->type()))
            {
                if (!dynamic_cast<const ResourceTypeDecl*>(structType))
                {
                    // Create struct instance
                    sb.append(GetQualifiedTypeName(&param->type()));
                    sb.append(L" _");
                    sb.append(param->name());
                    sb.append(L";");
                    sb.endline();
                    
                    // Assign each field
                    for (auto field : structType->fields())
                    {
                        String fieldFullName = param->name() + L"_" + field->name();
                        
                        // Find this field in our categorized lists
                        bool isOutput = false;
                        bool isStandalone = false;
                        
                        for (const auto& outField : outputFields)
                        {
                            if (outField.fullName == fieldFullName)
                            {
                                isOutput = true;
                                break;
                            }
                        }
                        
                        for (const auto& standaloneField : standaloneFields)
                        {
                            if (standaloneField.fullName == fieldFullName)
                            {
                                isStandalone = true;
                                break;
                            }
                        }
                        
                        // Skip pure output fields
                        if (isOutput && param->qualifier() == EVariableQualifier::Out)
                            continue;
                        
                        sb.append(L"_");
                        sb.append(param->name());
                        sb.append(L".");
                        sb.append(field->name());
                        sb.append(L" = ");
                        
                        if (isStandalone)
                        {
                            sb.append(fieldFullName);
                        }
                        else
                        {
                            sb.append(L"in.");
                            sb.append(fieldFullName);
                        }
                        sb.append(L";");
                        sb.endline();
                    }
                }
            }
        }
        
        // Call original function
        sb.append(L"// Call original function");
        sb.endline();
        
        // Handle return value
        bool hasStructReturn = false;
        if (funcDecl->return_type()->name() != L"void")
        {
            if (auto returnStruct = dynamic_cast<const StructureTypeDecl*>(funcDecl->return_type()))
            {
                hasStructReturn = true;
                sb.append(L"auto _result = ");
            }
            else
            {
                if (hasOutput)
                    sb.append(L"out.result = ");
                else
                    sb.append(L"return ");
            }
        }
        
        sb.append(funcDecl->name() + L"____impl____(");
        
        // Build parameter list for original function call
        bool firstParam = true;
        for (auto param : funcDecl->parameters())
        {
            if (dynamic_cast<const ResourceTypeDecl*>(&param->type()))
            {
                // Pass resource parameters directly
                if (!firstParam) sb.append(L", ");
                firstParam = false;
                sb.append(param->name());
            }
            else if (auto structType = dynamic_cast<const StructureTypeDecl*>(&param->type()))
            {
                if (!dynamic_cast<const ResourceTypeDecl*>(structType))
                {
                    // Pass the prepared struct variable
                    if (!firstParam) sb.append(L", ");
                    firstParam = false;
                    sb.append(L"_");
                    sb.append(param->name());
                }
            }
            else
            {
                // Non-struct parameters
                if (!firstParam) sb.append(L", ");
                firstParam = false;
                
                // Find this parameter in our lists
                bool foundInInput = false;
                bool foundInStandalone = false;
                
                // Check input fields
                for (const auto& field : inputFields)
                {
                    if (field.param == param && field.field == nullptr)
                    {
                        sb.append(L"in.");
                        sb.append(param->name());
                        foundInInput = true;
                        break;
                    }
                }
                
                // Check standalone fields
                if (!foundInInput)
                {
                    for (const auto& field : standaloneFields)
                    {
                        if (field.param == param && field.field == nullptr)
                        {
                            sb.append(field.fullName);
                            foundInStandalone = true;
                            break;
                        }
                    }
                }
                
                // Check output fields
                if (!foundInInput && !foundInStandalone)
                {
                    // Must be output-only
                    sb.append(L"out.");
                    sb.append(param->name());
                }
            }
        }
        
        sb.append(L");");
        sb.endline();
        
        // Handle struct return value - copy fields to output
        if (hasStructReturn)
        {
            if (auto returnStruct = dynamic_cast<const StructureTypeDecl*>(funcDecl->return_type()))
            {
                sb.append(L"// Copy return struct fields to output");
                sb.endline();
                for (auto field : returnStruct->fields())
                {
                    sb.append(L"out.result_");
                    sb.append(field->name());
                    sb.append(L" = _result.");
                    sb.append(field->name());
                    sb.append(L";");
                    sb.endline();
                }
            }
        }
        
        // Copy output values from parameters to output struct
        for (const auto& field : outputFields)
        {
            if (field.param->qualifier() == EVariableQualifier::Out || 
                field.param->qualifier() == EVariableQualifier::Inout)
            {
                // This field needs to be copied to output
                // (Already handled by struct return for now)
            }
        }
        
        // Return output struct
        if (hasOutput)
        {
            sb.append(L"return out;");
            sb.endline();
        }
    });
    sb.append(L"}");
    sb.endline();
}

bool MSLGenerator::SupportConstructor() const
{
    return true;
}

void MSLGenerator::GenerateSRTs(SourceBuilderNew& sb, const AST& ast)
{
    using SRT = std::map<uint32_t, const VarDecl*>;
    std::map<uint32_t, SRT> srts;
    SRT anonymous_srt;
    const VarDecl* push_constant = nullptr;
    uint32_t anonymous_srt_bind = 0;
    
    for (auto var : ast.global_vars())
    {
        // Check if this is a push constant
        if (FindAttr<PushConstantAttr>(var->attrs()))
        {
            push_constant = var;
        }
        else if (auto asResource = dynamic_cast<const ResourceTypeDecl*>(&var->type()))
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
                if (auto asPushConstant = FindAttr<PushConstantAttr>(resource->attrs()))
                {
                    auto Type = dynamic_cast<const ConstantBufferTypeDecl*>(&resource->type());
                    sb.append(L"PushConstant<" + GetTypeName(Type->element_type()) + L">");
                }
                else
                {
                    sb.append(GetTypeName(&resource->type()));
                }

                sb.append(L" ");
                sb.append(resource->name());
                sb.endline(L';');

                set_of_vars[resource] = set;
            }
        });
        sb.append(L"};");
        sb.endline();

        sb.append(
            std::format(L"constant {}& constant {} [[buffer({})]]", SRTTypeName, SRTVarName, set)
        );
        sb.endline(L';');
    };

    // Generate regular SRTs
    uint32_t next_set = 0;
    for (const auto& [set, srt] : srts)
    {
        gen_srt(set, srt);
        next_set = set + 1;
    }
    if (!anonymous_srt.empty())
    {
        gen_srt(next_set, anonymous_srt);
        next_set++;
    }
    if (push_constant != nullptr)
    {
        SRT push_constant_srt;
        push_constant_srt[0] = push_constant;
        gen_srt(next_set, push_constant_srt);
        next_set++;
    }
}

} // end namespace skr::CppSL::MSL
