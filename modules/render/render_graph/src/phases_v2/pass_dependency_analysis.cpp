#include "SkrContainersDef/set.hpp"
#include "SkrRenderGraph/phases_v2/pass_dependency_analysis.hpp"
#include "SkrRenderGraph/phases_v2/pass_info_analysis.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrRenderGraph/frontend/resource_node.hpp"
#include "SkrCore/log.hpp"

namespace skr {
namespace render_graph {

PassDependencyAnalysis::PassDependencyAnalysis(const PassInfoAnalysis& pass_info_analysis)
    : pass_info_analysis(pass_info_analysis)
{
}

bool PassDependencies::has_dependency_on(PassNode* pass) const
{
    for (const auto& dep : resource_dependencies)
    {
        if (dep.dependent_pass == pass)
            return true;
    }
    return false;
}

// IRenderGraphPhase 接口实现
void PassDependencyAnalysis::on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    SKR_LOG_INFO(u8"PassDependencyAnalysis: analyzing {} passes", all_passes.size());
    
    analyze_pass_dependencies(graph);
    
    // NEW: 进行逻辑拓扑分析（基于依赖关系，一次计算永不变）
    perform_logical_topological_sort();
    calculate_logical_dependency_levels();
    identify_logical_critical_path();
    collect_logical_topology_statistics();
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
    logical_topology_.logical_levels.clear();
    logical_topology_.logical_topological_order.clear();
    logical_topology_.logical_critical_path.clear();
}

void PassDependencyAnalysis::analyze_pass_dependencies(RenderGraph* graph)
{
    pass_dependencies_.clear();

    // Analyze dependencies for each pass
    auto& all_passes = get_passes(graph);
    for (size_t i = 0; i < all_passes.size(); ++i)
    {
        PassNode* current_pass = all_passes[i];
        skr::span<PassNode*> previous_passes = skr::span<PassNode*>(all_passes.data(), i);
        PassDependencies& current_deps = pass_dependencies_[current_pass];
        
        // Get pre-computed resource access info from PassInfoAnalysis
        const auto* current_resource_info = pass_info_analysis.get_resource_info(current_pass);
        if (!current_resource_info) 
            return;

        // For each resource, find the nearest conflicting previous pass
        for (const auto& current_access : current_resource_info->all_resource_accesses)
        {
            auto resource = current_access.resource;
            PassNode* nearest_conflicting_pass = nullptr;
            EResourceAccessType previous_access = EResourceAccessType::Read;
            ECGPUResourceState previous_state = CGPU_RESOURCE_STATE_UNDEFINED;
            EResourceDependencyType dependency_type = EResourceDependencyType::RAR;
            
            // Search backwards to find the nearest conflicting pass
            for (int i = previous_passes.size() - 1; i >= 0; --i)
            {
                PassNode* prev_pass = previous_passes[i];
                EResourceAccessType prev_access_type = pass_info_analysis.get_resource_access_type(prev_pass, resource);
                ECGPUResourceState prev_state = pass_info_analysis.get_resource_state(prev_pass, resource);
                
                // If found a previous pass accessing the same resource, check for conflicts
                if (bool prev_accesses_resource = (prev_state != CGPU_RESOURCE_STATE_UNDEFINED))
                {
                    dependency_type = get_confict_type(current_access.access_type, prev_access_type);
                    nearest_conflicting_pass = prev_pass;
                    previous_access = prev_access_type;
                    previous_state = prev_state;
                    break; // Found the nearest conflict, stop searching
                }
            }
            
            // If found a dependency, record it
            if (nearest_conflicting_pass != nullptr)
            {
                ResourceDependency dep;
                dep.dependent_pass = nearest_conflicting_pass;
                dep.resource = resource;
                dep.dependency_type = dependency_type;
                dep.current_access = current_access.access_type;
                dep.previous_access = previous_access;
                dep.current_state = current_access.resource_state;
                dep.previous_state = previous_state;
                current_deps.resource_dependencies.add(dep);

                current_deps.dependent_passes.add_unique(dep.dependent_pass);
            }
        }
    }
    
    // Build dependent_by_passes (reverse dependencies)
    for (auto& [pass, deps] : pass_dependencies_)
    {
        for (PassNode* dep_pass : deps.dependent_passes)
        {
            pass_dependencies_[dep_pass].dependent_by_passes.add(pass);
        }
    }
}

EResourceDependencyType PassDependencyAnalysis::get_confict_type(EResourceAccessType current, EResourceAccessType previous)
{
    const bool prev_is_read = (previous == EResourceAccessType::Read);
    const bool prev_is_write = !prev_is_read;
    const bool current_is_read = (current == EResourceAccessType::Read);
    const bool current_is_write = !current_is_read;

    if (prev_is_read && current_is_read)
    {
        return EResourceDependencyType::RAR; // Read After Read
    }
    else if (prev_is_read && current_is_write)
    {
        return EResourceDependencyType::RAW; // Read After Write
    }
    else if (prev_is_write && prev_is_read)
    {
        return EResourceDependencyType::WAR; // Write After Read
    }
    else if (prev_is_write && current_is_write)
    {
        return EResourceDependencyType::WAW; // Write After Write
    }
    return EResourceDependencyType::RAR; // Default case, should not happen
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

// For ScheduleTimeline - get pass-level dependencies directly
const skr::Set<PassNode*>& PassDependencyAnalysis::get_dependent_passes(PassNode* pass) const
{
    static const skr::Set<PassNode*> empty_vector;
    const auto* deps = get_pass_dependencies(pass);
    return deps ? deps->dependent_passes : empty_vector;
}

const skr::Vector<PassNode*>& PassDependencyAnalysis::get_dependent_by_passes(PassNode* pass) const
{
    static const skr::Vector<PassNode*> empty_vector;
    const auto* deps = get_pass_dependencies(pass);
    return deps ? deps->dependent_by_passes : empty_vector;
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

// ===== 逻辑拓扑分析实现 =====

void PassDependencyAnalysis::perform_logical_topological_sort()
{
    skr::FlatHashSet<PassNode*> visited;
    skr::FlatHashSet<PassNode*> on_stack;
    bool has_cycle = false;

    logical_topology_.logical_topological_order.clear();
    logical_topology_.logical_topological_order.reserve(pass_dependencies_.size());

    // DFS from each unvisited node
    for (const auto& [pass, deps] : pass_dependencies_)
    {
        if (!visited.contains(pass))
        {
            logical_topological_sort_dfs(pass, visited, on_stack, logical_topology_.logical_topological_order, has_cycle);
        }
    }

    // Reverse the order (DFS produces reverse topological order)
    std::reverse(logical_topology_.logical_topological_order.begin(), logical_topology_.logical_topological_order.end());

    // Assign logical topological order indices
    for (size_t i = 0; i < logical_topology_.logical_topological_order.size(); ++i)
    {
        auto* pass = logical_topology_.logical_topological_order[i];
        auto it = pass_dependencies_.find(pass);
        if (it != pass_dependencies_.end())
        {
            it->second.logical_topological_order = static_cast<uint32_t>(i);
        }
    }

    if (has_cycle)
    {
        SKR_LOG_ERROR(u8"PassDependencyAnalysis: Circular dependency detected in logical dependency graph!");
    }
}

void PassDependencyAnalysis::logical_topological_sort_dfs(
    PassNode* node, skr::FlatHashSet<PassNode*>& visited, skr::FlatHashSet<PassNode*>& on_stack, 
    skr::Vector<PassNode*>& sorted_passes, bool& has_cycle)
{
    visited.insert(node);
    on_stack.insert(node);

    // Visit all nodes that depend on this node (forward edges)
    auto deps_it = pass_dependencies_.find(node);
    if (deps_it != pass_dependencies_.end())
    {
        for (auto* dependent : deps_it->second.dependent_by_passes)
        {
            if (!visited.contains(dependent))
            {
                logical_topological_sort_dfs(dependent, visited, on_stack, sorted_passes, has_cycle);
            }
            else if (on_stack.contains(dependent))
            {
                has_cycle = true;
                SKR_LOG_ERROR(u8"Circular dependency: %s -> %s", node->get_name(), dependent->get_name());
            }
        }
    }

    on_stack.erase(node);
    sorted_passes.add(node);
}

void PassDependencyAnalysis::calculate_logical_dependency_levels()
{
    // Initialize all levels to 0
    for (auto& [pass, deps] : pass_dependencies_)
    {
        deps.logical_dependency_level = 0;
    }

    // Calculate levels using longest path algorithm on logical topological order
    for (auto* pass : logical_topology_.logical_topological_order)
    {
        auto pass_it = pass_dependencies_.find(pass);
        if (pass_it == pass_dependencies_.end()) continue;

        uint32_t current_level = pass_it->second.logical_dependency_level;

        // Update the level of all passes that depend on this one
        for (auto* dependent : pass_it->second.dependent_by_passes)
        {
            auto dep_it = pass_dependencies_.find(dependent);
            if (dep_it != pass_dependencies_.end())
            {
                dep_it->second.logical_dependency_level = std::max(dep_it->second.logical_dependency_level, current_level + 1);
            }
        }
    }

    // Find max level
    logical_topology_.max_logical_dependency_depth = 0;
    for (const auto& [pass, deps] : pass_dependencies_)
    {
        logical_topology_.max_logical_dependency_depth = std::max(logical_topology_.max_logical_dependency_depth, deps.logical_dependency_level);
    }

    // Create level groups
    logical_topology_.logical_levels.resize_default(logical_topology_.max_logical_dependency_depth + 1);
    for (uint32_t i = 0; i <= logical_topology_.max_logical_dependency_depth; ++i)
    {
        logical_topology_.logical_levels[i].level = i;
        logical_topology_.logical_levels[i].passes.clear();
    }

    // Assign passes to levels
    for (const auto& [pass, deps] : pass_dependencies_)
    {
        logical_topology_.logical_levels[deps.logical_dependency_level].passes.add(pass);
    }
}

void PassDependencyAnalysis::identify_logical_critical_path()
{
    logical_topology_.logical_critical_path.clear();
    
    // Step 1: 计算每个节点的高度（到DAG末尾的最长距离）
    // 按逆拓扑序处理，确保处理节点时其所有后继都已处理
    for (int i = logical_topology_.logical_topological_order.size() - 1; i >= 0; --i)
    {
        auto* pass = logical_topology_.logical_topological_order[i];
        auto pass_it = pass_dependencies_.find(pass);
        if (pass_it == pass_dependencies_.end()) continue;
        
        // 如果没有后继节点，高度为0
        if (pass_it->second.dependent_by_passes.is_empty())
        {
            pass_it->second.logical_critical_path_length = 0;
        }
        else
        {
            // 高度 = 1 + 所有后继节点的最大高度
            uint32_t max_height = 0;
            for (auto* dependent : pass_it->second.dependent_by_passes)
            {
                auto dep_it = pass_dependencies_.find(dependent);
                if (dep_it != pass_dependencies_.end())
                {
                    max_height = std::max(max_height, dep_it->second.logical_critical_path_length);
                }
            }
            pass_it->second.logical_critical_path_length = 1 + max_height;
        }
    }
    
    // Step 2: 找到高度最大的起始节点（没有前驱的节点）
    PassNode* critical_start = nullptr;
    uint32_t max_height = 0;
    
    for (const auto& [pass, deps] : pass_dependencies_)
    {
        // 起始节点：没有依赖其他Pass
        if (deps.dependent_passes.is_empty() && deps.logical_critical_path_length > max_height)
        {
            max_height = deps.logical_critical_path_length;
            critical_start = pass;
        }
    }
    
    // Step 3: 沿着高度递减的路径追踪关键路径
    if (critical_start)
    {
        PassNode* current = critical_start;
        
        while (current)
        {
            logical_topology_.logical_critical_path.add(current);
            
            // 在所有后继中选择高度最大的
            PassNode* next = nullptr;
            uint32_t next_height = 0;
            
            auto it = pass_dependencies_.find(current);
            if (it != pass_dependencies_.end())
            {
                for (auto* dependent : it->second.dependent_by_passes)
                {
                    auto dep_it = pass_dependencies_.find(dependent);
                    if (dep_it != pass_dependencies_.end())
                    {
                        // 选择高度最大的后继（如果有多个相同高度的，选择第一个）
                        if (dep_it->second.logical_critical_path_length > next_height ||
                            (dep_it->second.logical_critical_path_length == next_height && !next))
                        {
                            next_height = dep_it->second.logical_critical_path_length;
                            next = dependent;
                        }
                    }
                }
            }
            
            current = next;
        }
    }
}

void PassDependencyAnalysis::collect_logical_topology_statistics()
{
    logical_topology_.total_parallel_opportunities = 0;

    // Count parallel opportunities within each logical level
    for (auto& level : logical_topology_.logical_levels)
    {
        size_t pass_count = level.passes.size();
        if (pass_count > 1)
        {
            // Number of parallel pairs in this level
            logical_topology_.total_parallel_opportunities += static_cast<uint32_t>((pass_count * (pass_count - 1)) / 2);
        }

        // TODO: 这些统计信息需要访问PassInfoAnalysis，暂时先设为0
        // 如果需要详细统计，可以在这里添加对PassInfoAnalysis的调用
        level.total_resources_accessed = 0;
        level.cross_level_dependencies = 0;
    }
}

// ===== 逻辑拓扑查询接口实现 =====

uint32_t PassDependencyAnalysis::get_logical_dependency_level(PassNode* pass) const
{
    auto it = pass_dependencies_.find(pass);
    return (it != pass_dependencies_.end()) ? it->second.logical_dependency_level : 0;
}

uint32_t PassDependencyAnalysis::get_logical_topological_order(PassNode* pass) const
{
    auto it = pass_dependencies_.find(pass);
    return (it != pass_dependencies_.end()) ? it->second.logical_topological_order : UINT32_MAX;
}

uint32_t PassDependencyAnalysis::get_logical_critical_path_length(PassNode* pass) const
{
    auto it = pass_dependencies_.find(pass);
    return (it != pass_dependencies_.end()) ? it->second.logical_critical_path_length : 0;
}

bool PassDependencyAnalysis::can_execute_in_parallel_logically(PassNode* pass1, PassNode* pass2) const
{
    if (!pass1 || !pass2 || pass1 == pass2) return false;

    auto it1 = pass_dependencies_.find(pass1);
    auto it2 = pass_dependencies_.find(pass2);

    if (it1 == pass_dependencies_.end() || it2 == pass_dependencies_.end()) return false;

    // Passes can execute in parallel logically if they are at the same logical dependency level
    // and neither depends on the other
    if (it1->second.logical_dependency_level != it2->second.logical_dependency_level) return false;

    // Check if pass1 depends on pass2
    for (auto* dep : it1->second.dependent_passes)
    {
        if (dep == pass2) return false;
    }

    // Check if pass2 depends on pass1
    for (auto* dep : it2->second.dependent_passes)
    {
        if (dep == pass1) return false;
    }

    return true;
}

// ===== 逻辑拓扑调试输出 =====

void PassDependencyAnalysis::dump_logical_topology() const
{
    SKR_LOG_INFO(u8"========== Logical Topology Analysis ==========");
    SKR_LOG_INFO(u8"Max logical dependency depth: %u", logical_topology_.max_logical_dependency_depth);
    SKR_LOG_INFO(u8"Total parallel opportunities: %u", logical_topology_.total_parallel_opportunities);
    SKR_LOG_INFO(u8"Logical topological order:");
    
    for (size_t i = 0; i < logical_topology_.logical_topological_order.size(); ++i)
    {
        auto* pass = logical_topology_.logical_topological_order[i];
        uint32_t level = get_logical_dependency_level(pass);
        
        SKR_LOG_INFO(u8"  [%zu] %s (logical level: %u)", i, pass->get_name(), level);
    }

    SKR_LOG_INFO(u8"\nLogical dependency levels:");
    for (const auto& level : logical_topology_.logical_levels)
    {
        SKR_LOG_INFO(u8"Level %u (%u passes):",
            level.level,
            static_cast<uint32_t>(level.passes.size()));

        for (auto* pass : level.passes)
        {
            uint32_t critical_length = get_logical_critical_path_length(pass);
            SKR_LOG_INFO(u8"  - %s (critical path length: %u)",
                pass->get_name(), critical_length);
        }
    }

    SKR_LOG_INFO(u8"=============================================");
}

void PassDependencyAnalysis::dump_logical_critical_path() const
{
    SKR_LOG_INFO(u8"========== Logical Critical Path ==========");
    SKR_LOG_INFO(u8"Logical critical path length: %u", static_cast<uint32_t>(logical_topology_.logical_critical_path.size()));

    for (size_t i = 0; i < logical_topology_.logical_critical_path.size(); ++i)
    {
        auto* pass = logical_topology_.logical_critical_path[i];
        uint32_t level = get_logical_dependency_level(pass);
        
        SKR_LOG_INFO(u8"[%zu] %s (logical level: %u)",
            i, pass->get_name(), level);
    }

    SKR_LOG_INFO(u8"=========================================");
}

} // namespace render_graph
} // namespace skr