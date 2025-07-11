#include "SkrRenderGraph/phases_v2/pass_dependency_analysis.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrRenderGraph/frontend/resource_node.hpp"
#include "SkrCore/log.hpp"

namespace skr {
namespace render_graph {

bool PassDependencies::has_dependency_on(PassNode* pass) const
{
    for (const auto& dep : resource_dependencies)
    {
        if (dep.dependent_pass == pass)
            return true;
    }
    return false;
}

skr::Vector<ResourceDependency> PassDependencies::get_dependencies_on(PassNode* pass) const
{
    skr::Vector<ResourceDependency> result;
    for (const auto& dep : resource_dependencies)
    {
        if (dep.dependent_pass == pass)
            result.add(dep);
    }
    return result;
}

// IRenderGraphPhase 接口实现
void PassDependencyAnalysis::on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    pass_dependencies_.clear();
    
    // Get all passes (in declaration order) - 使用IRenderGraphPhase提供的helper
    auto& all_passes = get_passes(graph);
    
    SKR_LOG_INFO(u8"PassDependencyAnalysis: analyzing {} passes", all_passes.size());
    
    // Analyze dependencies for each pass
    for (size_t i = 0; i < all_passes.size(); ++i)
    {
        PassNode* current_pass = all_passes[i];
        
        // Get all passes before this one
        skr::Vector<PassNode*> previous_passes;
        for (size_t j = 0; j < i; ++j)
        {
            previous_passes.add(all_passes[j]);
        }
        
        // Analyze current pass dependencies on previous passes
        analyze_pass_dependencies(current_pass, previous_passes);
    }
}

void PassDependencyAnalysis::on_initialize(RenderGraph* graph) SKR_NOEXCEPT
{
    // 预分配容量，避免后续扩容
    pass_dependencies_.reserve(64);
}

void PassDependencyAnalysis::on_finalize(RenderGraph* graph) SKR_NOEXCEPT
{
    // 清理分析结果
    pass_dependencies_.clear();
}

void PassDependencyAnalysis::analyze_pass_dependencies(PassNode* current_pass, const skr::Vector<PassNode*>& previous_passes)
{
    PassDependencies& current_deps = pass_dependencies_[current_pass];
    
    // Collect all resources accessed by current pass
    skr::FlatHashMap<ResourceNode*, EResourceAccessType> current_resources;
    skr::FlatHashMap<ResourceNode*, ECGPUResourceState> current_states;
    
    // Analyze texture access
    current_pass->foreach_textures([&](TextureNode* texture, TextureEdge* edge) {
        EResourceAccessType access_type = get_resource_access_type(current_pass, texture);
        ECGPUResourceState state = get_resource_state(current_pass, texture);
        current_resources[texture] = access_type;
        current_states[texture] = state;
        
        // Check for graphics resource dependency while we're already iterating
        if (texture && (texture->get_desc().descriptors & CGPU_RESOURCE_TYPE_RENDER_TARGET ||
                       texture->get_desc().descriptors & CGPU_RESOURCE_TYPE_DEPTH_STENCIL)) {
            current_deps.has_graphics_resource_dependency = true;
        }
        
        return true;
    });
    
    // Analyze buffer access
    current_pass->foreach_buffers([&](BufferNode* buffer, BufferEdge* edge) {
        EResourceAccessType access_type = get_resource_access_type(current_pass, buffer);
        ECGPUResourceState state = get_resource_state(current_pass, buffer);
        current_resources[buffer] = access_type;
        current_states[buffer] = state;
        return true;
    });
    
    // For each resource, find the nearest conflicting previous pass
    for (const auto& [resource, current_access] : current_resources)
    {
        PassNode* nearest_conflicting_pass = nullptr;
        EResourceAccessType previous_access = EResourceAccessType::Read;
        ECGPUResourceState previous_state = CGPU_RESOURCE_STATE_UNDEFINED;
        EResourceDependencyType dependency_type;
        
        // Search backwards to find the nearest conflicting pass
        for (int i = previous_passes.size() - 1; i >= 0; --i)
        {
            PassNode* prev_pass = previous_passes[i];
            
            // Check if this previous pass accesses the same resource
            bool prev_accesses_resource = false;
            EResourceAccessType prev_access_type = EResourceAccessType::Read;
            ECGPUResourceState prev_state = CGPU_RESOURCE_STATE_UNDEFINED;
            
            // Check texture access
            if (resource->get_type() == EObjectType::Texture)
            {
                prev_pass->foreach_textures([&](TextureNode* texture, TextureEdge* edge) {
                    if (texture == resource)
                    {
                        prev_accesses_resource = true;
                        prev_access_type = get_resource_access_type(prev_pass, texture);
                        prev_state = get_resource_state(prev_pass, texture);
                        return false; // Found it, stop iteration
                    }
                    return true;
                });
            }
            // Check buffer access
            else if (resource->get_type() == EObjectType::Buffer)
            {
                prev_pass->foreach_buffers([&](BufferNode* buffer, BufferEdge* edge) {
                    if (buffer == resource)
                    {
                        prev_accesses_resource = true;
                        prev_access_type = get_resource_access_type(prev_pass, buffer);
                        prev_state = get_resource_state(prev_pass, buffer);
                        return false; // Found it, stop iteration
                    }
                    return true;
                });
            }
            
            // If found a previous pass accessing the same resource, check for conflicts
            if (prev_accesses_resource)
            {
                if (has_resource_conflict(current_access, prev_access_type, dependency_type))
                {
                    nearest_conflicting_pass = prev_pass;
                    previous_access = prev_access_type;
                    previous_state = prev_state;
                    break; // Found the nearest conflict, stop searching
                }
            }
        }
        
        // If found a dependency, record it
        if (nearest_conflicting_pass != nullptr)
        {
            ResourceDependency dep;
            dep.dependent_pass = nearest_conflicting_pass;
            dep.resource = resource;
            dep.dependency_type = dependency_type;
            dep.current_access = current_access;
            dep.previous_access = previous_access;
            dep.current_state = current_states[resource];
            dep.previous_state = previous_state;
            
            current_deps.resource_dependencies.add(dep);
        }
    }
}

EResourceAccessType PassDependencyAnalysis::get_resource_access_type(PassNode* pass, ResourceNode* resource)
{
    bool has_read = false;
    bool has_write = false;
    
    if (resource->get_type() == EObjectType::Texture)
    {
        TextureNode* texture = static_cast<TextureNode*>(resource);
        pass->foreach_textures([&](TextureNode* tex, TextureEdge* edge) {
            if (tex == texture)
            {
                // Determine access mode based on edge type
                if (edge->type == ERelationshipType::TextureRead)
                {
                    has_read = true;
                }
                else if (edge->type == ERelationshipType::TextureWrite)
                {
                    has_write = true;
                }
                else if (edge->type == ERelationshipType::TextureReadWrite)
                {
                    has_read = true;
                    has_write = true;
                }
            }
            return true;
        });
    }
    else if (resource->get_type() == EObjectType::Buffer)
    {
        BufferNode* buffer = static_cast<BufferNode*>(resource);
        pass->foreach_buffers([&](BufferNode* buf, BufferEdge* edge) {
            if (buf == buffer)
            {
                // Determine access mode based on edge type
                if (edge->type == ERelationshipType::BufferRead)
                {
                    has_read = true;
                }
                else if (edge->type == ERelationshipType::BufferReadWrite)
                {
                    has_read = true;
                    has_write = true;
                }
            }
            return true;
        });
    }
    
    if (has_read && has_write)
        return EResourceAccessType::ReadWrite;
    else if (has_write)
        return EResourceAccessType::Write;
    else
        return EResourceAccessType::Read;
}

ECGPUResourceState PassDependencyAnalysis::get_resource_state(PassNode* pass, ResourceNode* resource)
{
    ECGPUResourceState state = CGPU_RESOURCE_STATE_UNDEFINED;
    
    if (resource->get_type() == EObjectType::Texture)
    {
        TextureNode* texture = static_cast<TextureNode*>(resource);
        pass->foreach_textures([&](TextureNode* tex, TextureEdge* edge) {
            if (tex == texture)
            {
                // Get requested state from edge
                if (edge->get_type() == ERelationshipType::TextureRead)
                {
                    auto rw_edge = static_cast<TextureReadEdge*>(edge);
                    state = rw_edge->requested_state;;
                }
                else if (edge->get_type() == ERelationshipType::TextureWrite)
                {
                    auto write_edge = static_cast<TextureRenderEdge*>(edge);
                    state = write_edge->requested_state;
                }
                else if (edge->get_type() == ERelationshipType::TextureReadWrite)
                {
                    auto rw_edge = static_cast<TextureReadWriteEdge*>(edge);
                    state = rw_edge->requested_state;
                }
                return false; // Found it, stop iteration
            }
            return true;
        });
    }
    else if (resource->get_type() == EObjectType::Buffer)
    {
        BufferNode* buffer = static_cast<BufferNode*>(resource);
        pass->foreach_buffers([&](BufferNode* buf, BufferEdge* edge) {
            if (buf == buffer)
            {
                // Get requested state from edge
                auto* buffer_edge = static_cast<BufferEdge*>(edge);
                state = buffer_edge->requested_state;
                return false; // Found it, stop iteration
            }
            return true;
        });
    }
    
    return state;
}

bool PassDependencyAnalysis::has_resource_conflict(EResourceAccessType current, EResourceAccessType previous, 
                                                   EResourceDependencyType& out_dependency_type)
{
    // RAR: Read After Read - usually no sync needed, but record dependency
    if (current == EResourceAccessType::Read && previous == EResourceAccessType::Read)
    {
        out_dependency_type = EResourceDependencyType::RAR;
        return true;
    }
    
    // RAW: Read After Write - true dependency
    if (current == EResourceAccessType::Read && 
        (previous == EResourceAccessType::Write || previous == EResourceAccessType::ReadWrite))
    {
        out_dependency_type = EResourceDependencyType::RAW;
        return true;
    }
    
    // WAR: Write After Read - anti dependency
    if ((current == EResourceAccessType::Write || current == EResourceAccessType::ReadWrite) && 
        previous == EResourceAccessType::Read)
    {
        out_dependency_type = EResourceDependencyType::WAR;
        return true;
    }
    
    // WAW: Write After Write - output dependency
    if ((current == EResourceAccessType::Write || current == EResourceAccessType::ReadWrite) && 
        (previous == EResourceAccessType::Write || previous == EResourceAccessType::ReadWrite))
    {
        out_dependency_type = EResourceDependencyType::WAW;
        return true;
    }
    
    // ReadWrite access conflicts with any access
    if (current == EResourceAccessType::ReadWrite || previous == EResourceAccessType::ReadWrite)
    {
        if (current == EResourceAccessType::ReadWrite && previous == EResourceAccessType::Read)
            out_dependency_type = EResourceDependencyType::WAR;
        else if (current == EResourceAccessType::Read && previous == EResourceAccessType::ReadWrite)
            out_dependency_type = EResourceDependencyType::RAW;
        else
            out_dependency_type = EResourceDependencyType::WAW;
        return true;
    }
    
    return false;
}

const PassDependencies* PassDependencyAnalysis::get_pass_dependencies(PassNode* pass) const
{
    auto it = pass_dependencies_.find(pass);
    return (it != pass_dependencies_.end()) ? &it->second : nullptr;
}

bool PassDependencyAnalysis::has_dependencies(PassNode* pass) const
{
    const auto* deps = get_pass_dependencies(pass);
    return deps != nullptr && !deps->resource_dependencies.is_empty();
}

void PassDependencyAnalysis::dump_dependencies() const
{
    SKR_LOG_INFO(u8"========== Pass Dependency Analysis Results ==========");
    
    for (const auto& [pass, deps] : pass_dependencies_)
    {
        if (deps.resource_dependencies.is_empty())
            continue;
            
        SKR_LOG_INFO(u8"Pass: %s depends on:", pass->get_name());
        
        for (const auto& dep : deps.resource_dependencies)
        {
            const char8_t* dep_type_str = u8"Unknown";
            switch (dep.dependency_type)
            {
                case EResourceDependencyType::RAR: dep_type_str = u8"RAR"; break;
                case EResourceDependencyType::RAW: dep_type_str = u8"RAW"; break;
                case EResourceDependencyType::WAR: dep_type_str = u8"WAR"; break;
                case EResourceDependencyType::WAW: dep_type_str = u8"WAW"; break;
            }
            
            const char8_t* current_access_str = u8"Unknown";
            const char8_t* previous_access_str = u8"Unknown";
            
            switch (dep.current_access)
            {
                case EResourceAccessType::Read: current_access_str = u8"Read"; break;
                case EResourceAccessType::Write: current_access_str = u8"Write"; break;
                case EResourceAccessType::ReadWrite: current_access_str = u8"ReadWrite"; break;
            }
            
            switch (dep.previous_access)
            {
                case EResourceAccessType::Read: previous_access_str = u8"Read"; break;
                case EResourceAccessType::Write: previous_access_str = u8"Write"; break;
                case EResourceAccessType::ReadWrite: previous_access_str = u8"ReadWrite"; break;
            }
            
            SKR_LOG_INFO(u8"  -> Pass: %s, Resource: %s, Type: %s (%s->%s), State: %d->%d", 
                        dep.dependent_pass->get_name(),
                        dep.resource->get_name(),
                        dep_type_str,
                        previous_access_str,
                        current_access_str,
                        (int)dep.previous_state,
                        (int)dep.current_state);
        }
    }
    
    SKR_LOG_INFO(u8"==========================================");
}

} // namespace render_graph
} // namespace skr