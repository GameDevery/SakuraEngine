#include "SkrContainersDef/set.hpp"
#include "SkrRenderGraph/phases_v2/pass_dependency_analysis.hpp"
#include "SkrRenderGraph/phases_v2/pass_info_analysis.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrRenderGraph/frontend/resource_node.hpp"
#include "SkrCore/log.hpp"
#include <algorithm>

namespace skr
{
namespace render_graph
{

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
    dependency_levels_.levels.clear();
    dependency_levels_.topological_order.clear();
    dependency_levels_.critical_path.clear();

    // Get all passes (in declaration order) - 使用IRenderGraphPhase提供的helper
    auto& all_passes = get_passes(graph);

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

    // Build pass-level dependencies after all resource dependencies are analyzed
    build_pass_level_dependencies();

    // NEW: Perform dependency level analysis
    perform_topological_sort();
    calculate_dependency_levels();
    identify_critical_path();
    collect_level_statistics();
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

    // Get pre-computed resource access info from PassInfoAnalysis
    const auto* current_resource_info = pass_info_analysis.get_resource_info(current_pass);
    if (!current_resource_info) return;

    // Build lookup map from pre-computed access info
    skr::FlatHashMap<ResourceNode*, EResourceAccessType> current_resources;
    skr::FlatHashMap<ResourceNode*, ECGPUResourceState> current_states;

    for (const auto& access : current_resource_info->all_resource_accesses)
    {
        current_resources[access.resource] = access.access_type;
        current_states[access.resource] = access.resource_state;
    }

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

            // Use PassInfoAnalysis to get access type (much faster)
            EResourceAccessType prev_access_type = pass_info_analysis.get_resource_access_type(prev_pass, resource);
            ECGPUResourceState prev_state = pass_info_analysis.get_resource_state(prev_pass, resource);

            // Check if previous pass actually accesses this resource
            bool prev_accesses_resource = (prev_state != CGPU_RESOURCE_STATE_UNDEFINED);

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

void PassDependencyAnalysis::build_pass_level_dependencies()
{
    // Build pass-level dependency info from resource dependencies
    for (auto& [pass, deps] : pass_dependencies_)
    {
        skr::InlineSet<PassNode*, 8> unique_dependencies;
        skr::InlineSet<PassNode*, 8> unique_dependents;

        // Extract unique passes from resource dependencies
        for (const auto& resource_dep : deps.resource_dependencies)
        {
            unique_dependencies.add(resource_dep.dependent_pass);
        }

        // Convert to vectors
        deps.dependent_passes.clear();
        deps.dependent_passes.reserve(unique_dependencies.size());
        for (PassNode* dep_pass : unique_dependencies)
        {
            deps.dependent_passes.add(dep_pass);
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

bool PassDependencyAnalysis::has_resource_conflict(EResourceAccessType current, EResourceAccessType previous, EResourceDependencyType& out_dependency_type)
{
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

// For ScheduleTimeline - get pass-level dependencies directly
const skr::Vector<PassNode*>& PassDependencyAnalysis::get_dependent_passes(PassNode* pass) const
{
    static const skr::Vector<PassNode*> empty_vector;
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
            case EResourceDependencyType::RAR:
                dep_type_str = u8"RAR";
                break;
            case EResourceDependencyType::RAW:
                dep_type_str = u8"RAW";
                break;
            case EResourceDependencyType::WAR:
                dep_type_str = u8"WAR";
                break;
            case EResourceDependencyType::WAW:
                dep_type_str = u8"WAW";
                break;
            }

            const char8_t* current_access_str = u8"Unknown";
            const char8_t* previous_access_str = u8"Unknown";

            switch (dep.current_access)
            {
            case EResourceAccessType::Read:
                current_access_str = u8"Read";
                break;
            case EResourceAccessType::Write:
                current_access_str = u8"Write";
                break;
            case EResourceAccessType::ReadWrite:
                current_access_str = u8"ReadWrite";
                break;
            }

            switch (dep.previous_access)
            {
            case EResourceAccessType::Read:
                previous_access_str = u8"Read";
                break;
            case EResourceAccessType::Write:
                previous_access_str = u8"Write";
                break;
            case EResourceAccessType::ReadWrite:
                previous_access_str = u8"ReadWrite";
                break;
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

// NEW: Dependency level analysis implementation
void PassDependencyAnalysis::perform_topological_sort()
{
    skr::FlatHashSet<PassNode*> visited;
    skr::FlatHashSet<PassNode*> on_stack;
    bool has_cycle = false;

    dependency_levels_.topological_order.clear();
    dependency_levels_.topological_order.reserve(pass_dependencies_.size());

    // DFS from each unvisited node
    for (const auto& [pass, deps] : pass_dependencies_)
    {
        if (!visited.contains(pass))
        {
            topological_sort_dfs(pass, visited, on_stack, dependency_levels_.topological_order, has_cycle);
        }
    }

    // Reverse the order (DFS produces reverse topological order)
    std::reverse(dependency_levels_.topological_order.begin(), dependency_levels_.topological_order.end());

    // Assign topological order indices
    for (size_t i = 0; i < dependency_levels_.topological_order.size(); ++i)
    {
        auto* pass = dependency_levels_.topological_order[i];
        if (auto it = pass_dependencies_.find(pass); it != pass_dependencies_.end())
        {
            it->second.topological_order = static_cast<uint32_t>(i);
        }
    }

    if (has_cycle)
    {
        SKR_LOG_ERROR(u8"PassDependencyAnalysis: Circular dependency detected in render graph!");
    }
}

void PassDependencyAnalysis::topological_sort_dfs(
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
                topological_sort_dfs(dependent, visited, on_stack, sorted_passes, has_cycle);
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

void PassDependencyAnalysis::calculate_dependency_levels()
{
    // Initialize all levels to 0
    for (auto& [pass, deps] : pass_dependencies_)
    {
        deps.dependency_level = 0;
    }

    // Calculate levels using longest path algorithm
    // Process passes in topological order
    for (auto* pass : dependency_levels_.topological_order)
    {
        auto pass_it = pass_dependencies_.find(pass);
        if (pass_it == pass_dependencies_.end()) continue;

        uint32_t current_level = pass_it->second.dependency_level;

        // Update the level of all passes that depend on this one
        for (auto* dependent : pass_it->second.dependent_by_passes)
        {
            auto dep_it = pass_dependencies_.find(dependent);
            if (dep_it != pass_dependencies_.end())
            {
                dep_it->second.dependency_level = std::max(dep_it->second.dependency_level, current_level + 1);
            }
        }
    }

    // Find max level and group passes by level
    dependency_levels_.max_dependency_depth = 0;
    for (const auto& [pass, deps] : pass_dependencies_)
    {
        dependency_levels_.max_dependency_depth = std::max(dependency_levels_.max_dependency_depth, deps.dependency_level);
    }

    // Create level groups
    dependency_levels_.levels.resize_default(dependency_levels_.max_dependency_depth + 1);
    for (uint32_t i = 0; i <= dependency_levels_.max_dependency_depth; ++i)
    {
        dependency_levels_.levels[i].level = i;
    }

    // Assign passes to levels
    for (const auto& [pass, deps] : pass_dependencies_)
    {
        dependency_levels_.levels[deps.dependency_level].passes.add(pass);
    }
}

void PassDependencyAnalysis::identify_critical_path()
{
    dependency_levels_.critical_path.clear();
    
    // Step 1: 计算每个节点的高度（到DAG末尾的最长距离）
    // 按逆拓扑序处理，确保处理节点时其所有后继都已处理
    for (int i = dependency_levels_.topological_order.size() - 1; i >= 0; --i)
    {
        auto* pass = dependency_levels_.topological_order[i];
        auto pass_it = pass_dependencies_.find(pass);
        if (pass_it == pass_dependencies_.end()) continue;
        
        // 如果没有后继节点，高度为0
        if (pass_it->second.dependent_by_passes.is_empty())
        {
            pass_it->second.critical_path_length = 0;
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
                    max_height = std::max(max_height, dep_it->second.critical_path_length);
                }
            }
            pass_it->second.critical_path_length = 1 + max_height;
        }
    }
    
    // Step 2: 找到高度最大的起始节点（没有前驱的节点）
    PassNode* critical_start = nullptr;
    uint32_t max_height = 0;
    
    for (const auto& [pass, deps] : pass_dependencies_)
    {
        // 起始节点：没有依赖其他Pass
        if (deps.dependent_passes.is_empty() && deps.critical_path_length > max_height)
        {
            max_height = deps.critical_path_length;
            critical_start = pass;
        }
    }
    
    // Step 3: 沿着高度递减的路径追踪关键路径
    if (critical_start)
    {
        PassNode* current = critical_start;
        
        while (current)
        {
            dependency_levels_.critical_path.add(current);
            
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
                        if (dep_it->second.critical_path_length > next_height ||
                            (dep_it->second.critical_path_length == next_height && !next))
                        {
                            next_height = dep_it->second.critical_path_length;
                            next = dependent;
                        }
                    }
                }
            }
            
            current = next;
        }
    }
}

void PassDependencyAnalysis::collect_level_statistics()
{
    dependency_levels_.total_parallel_opportunities = 0;

    // Count parallel opportunities within each level
    for (auto& level : dependency_levels_.levels)
    {
        size_t pass_count = level.passes.size();
        if (pass_count > 1)
        {
            // Number of parallel pairs in this level
            dependency_levels_.total_parallel_opportunities += static_cast<uint32_t>((pass_count * (pass_count - 1)) / 2);
        }

        // Count resources accessed at this level
        skr::FlatHashSet<ResourceNode*> level_resources;
        for (auto* pass : level.passes)
        {
            const auto* pass_info = pass_info_analysis.get_resource_info(pass);
            if (pass_info)
            {
                for (const auto& access : pass_info->all_resource_accesses)
                {
                    level_resources.insert(access.resource);
                }
            }
        }
        level.total_resources_accessed = static_cast<uint32_t>(level_resources.size());

        // Count cross-level dependencies
        level.cross_level_dependencies = 0;
        for (auto* pass : level.passes)
        {
            auto it = pass_dependencies_.find(pass);
            if (it != pass_dependencies_.end())
            {
                for (auto* dep : it->second.dependent_passes)
                {
                    auto dep_it = pass_dependencies_.find(dep);
                    if (dep_it != pass_dependencies_.end() && dep_it->second.dependency_level < level.level - 1)
                    {
                        level.cross_level_dependencies++;
                    }
                }
            }
        }
    }
}

// Dependency level query implementations
uint32_t PassDependencyAnalysis::get_pass_dependency_level(PassNode* pass) const
{
    auto it = pass_dependencies_.find(pass);
    return (it != pass_dependencies_.end()) ? it->second.dependency_level : 0;
}

uint32_t PassDependencyAnalysis::get_pass_topological_order(PassNode* pass) const
{
    auto it = pass_dependencies_.find(pass);
    return (it != pass_dependencies_.end()) ? it->second.topological_order : UINT32_MAX;
}

bool PassDependencyAnalysis::can_execute_in_parallel(PassNode* pass1, PassNode* pass2) const
{
    if (!pass1 || !pass2 || pass1 == pass2) return false;

    auto it1 = pass_dependencies_.find(pass1);
    auto it2 = pass_dependencies_.find(pass2);

    if (it1 == pass_dependencies_.end() || it2 == pass_dependencies_.end()) return false;

    // Passes can execute in parallel if they are at the same dependency level
    // and neither depends on the other
    if (it1->second.dependency_level != it2->second.dependency_level) return false;

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

void PassDependencyAnalysis::dump_dependency_levels() const
{
    SKR_LOG_INFO(u8"========== Dependency Level Analysis ==========");
    SKR_LOG_INFO(u8"Max dependency depth: %u", dependency_levels_.max_dependency_depth);
    SKR_LOG_INFO(u8"Total parallel opportunities: %u", dependency_levels_.total_parallel_opportunities);

    for (const auto& level : dependency_levels_.levels)
    {
        SKR_LOG_INFO(u8"\nLevel %u (%u passes, %u resources):",
            level.level,
            static_cast<uint32_t>(level.passes.size()),
            level.total_resources_accessed);

        for (auto* pass : level.passes)
        {
            auto it = pass_dependencies_.find(pass);
            if (it != pass_dependencies_.end())
            {
                SKR_LOG_INFO(u8"  - %s (critical path length: %u)",
                    pass->get_name(),
                    it->second.critical_path_length);
            }
        }

        if (level.cross_level_dependencies > 0)
        {
            SKR_LOG_INFO(u8"  Cross-level dependencies: %u", level.cross_level_dependencies);
        }
    }

    SKR_LOG_INFO(u8"===============================================");
}

void PassDependencyAnalysis::dump_critical_path() const
{
    SKR_LOG_INFO(u8"========== Critical Path ==========");
    SKR_LOG_INFO(u8"Critical path length: %u", static_cast<uint32_t>(dependency_levels_.critical_path.size()));

    for (size_t i = 0; i < dependency_levels_.critical_path.size(); ++i)
    {
        auto* pass = dependency_levels_.critical_path[i];
        auto it = pass_dependencies_.find(pass);
        if (it != pass_dependencies_.end())
        {
            SKR_LOG_INFO(u8"[%zu] %s (level: %u)",
                i,
                pass->get_name(),
                it->second.dependency_level);
        }
    }

    SKR_LOG_INFO(u8"===================================");
}

} // namespace render_graph
} // namespace skr