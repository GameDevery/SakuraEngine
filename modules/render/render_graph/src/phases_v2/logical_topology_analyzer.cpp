#include "SkrRenderGraph/phases_v2/pass_dependency_analysis.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrCore/log.hpp"
#include <algorithm>

namespace skr {
namespace render_graph {

void LogicalTopologyAnalyzer::analyze_topology(const skr::FlatHashMap<PassNode*, PassDependencies>& pass_dependencies)
{
    // 清理之前的结果
    topology_result_.logical_levels.clear();
    topology_result_.logical_topological_order.clear();
    topology_result_.logical_critical_path.clear();
    pass_to_order_.clear();
    
    if (pass_dependencies.empty()) return;
    
    // Step 1: 拓扑排序
    perform_topological_sort(pass_dependencies);
    
    // Step 2: 计算依赖级别
    calculate_dependency_levels(pass_dependencies);
    
    // Step 3: 识别关键路径
    identify_critical_path(pass_dependencies);
    
    // Step 4: 收集统计信息
    collect_statistics();
}

void LogicalTopologyAnalyzer::perform_topological_sort(const skr::FlatHashMap<PassNode*, PassDependencies>& dependencies)
{
    skr::FlatHashSet<PassNode*> visited;
    skr::FlatHashSet<PassNode*> on_stack;
    bool has_cycle = false;

    topology_result_.logical_topological_order.clear();
    topology_result_.logical_topological_order.reserve(dependencies.size());

    // DFS from each unvisited node
    for (const auto& [pass, deps] : dependencies)
    {
        if (!visited.contains(pass))
        {
            topological_sort_dfs(pass, dependencies, visited, on_stack, topology_result_.logical_topological_order, has_cycle);
        }
    }

    // Reverse the order (DFS produces reverse topological order)
    std::reverse(topology_result_.logical_topological_order.begin(), topology_result_.logical_topological_order.end());

    // Build fast lookup cache
    for (size_t i = 0; i < topology_result_.logical_topological_order.size(); ++i)
    {
        pass_to_order_[topology_result_.logical_topological_order[i]] = static_cast<uint32_t>(i);
    }

    if (has_cycle)
    {
        SKR_LOG_ERROR(u8"LogicalTopologyAnalyzer: Circular dependency detected!");
    }
}

void LogicalTopologyAnalyzer::topological_sort_dfs(PassNode* node, 
                                                    const skr::FlatHashMap<PassNode*, PassDependencies>& dependencies,
                                                    skr::FlatHashSet<PassNode*>& visited,
                                                    skr::FlatHashSet<PassNode*>& on_stack,
                                                    skr::Vector<PassNode*>& sorted_passes,
                                                    bool& has_cycle)
{
    visited.insert(node);
    on_stack.insert(node);

    // Visit all nodes that depend on this node (forward edges)
    auto deps_it = dependencies.find(node);
    if (deps_it != dependencies.end())
    {
        for (auto* dependent : deps_it->second.dependent_by_passes)
        {
            if (!visited.contains(dependent))
            {
                topological_sort_dfs(dependent, dependencies, visited, on_stack, sorted_passes, has_cycle);
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

void LogicalTopologyAnalyzer::calculate_dependency_levels(const skr::FlatHashMap<PassNode*, PassDependencies>& dependencies)
{
    // 初始化临时级别映射
    skr::FlatHashMap<PassNode*, uint32_t> level_map;
    for (const auto& [pass, deps] : dependencies)
    {
        level_map[pass] = 0;
    }

    // 使用拓扑排序的顺序计算依赖级别
    for (auto* pass : topology_result_.logical_topological_order)
    {
        auto deps_it = dependencies.find(pass);
        if (deps_it == dependencies.end()) continue;

        uint32_t current_level = level_map[pass];

        // 更新所有依赖此Pass的后继Pass的级别
        for (auto* dependent : deps_it->second.dependent_by_passes)
        {
            auto level_it = level_map.find(dependent);
            if (level_it != level_map.end())
            {
                level_it->second = std::max(level_it->second, current_level + 1);
            }
        }
    }

    // 找到最大级别
    topology_result_.max_logical_dependency_depth = 0;
    for (const auto& [pass, level] : level_map)
    {
        topology_result_.max_logical_dependency_depth = std::max(topology_result_.max_logical_dependency_depth, level);
    }

    // 创建级别分组
    topology_result_.logical_levels.resize_default(topology_result_.max_logical_dependency_depth + 1);
    for (uint32_t i = 0; i <= topology_result_.max_logical_dependency_depth; ++i)
    {
        topology_result_.logical_levels[i].level = i;
        topology_result_.logical_levels[i].passes.clear();
    }

    // 分配Pass到对应级别
    for (const auto& [pass, level] : level_map)
    {
        topology_result_.logical_levels[level].passes.add(pass);
    }
}

void LogicalTopologyAnalyzer::identify_critical_path(const skr::FlatHashMap<PassNode*, PassDependencies>& dependencies)
{
    topology_result_.logical_critical_path.clear();
    
    // 计算每个节点的高度（到DAG末尾的最长距离）
    skr::FlatHashMap<PassNode*, uint32_t> height_map;
    
    // 按逆拓扑序处理
    for (int i = topology_result_.logical_topological_order.size() - 1; i >= 0; --i)
    {
        auto* pass = topology_result_.logical_topological_order[i];
        auto deps_it = dependencies.find(pass);
        if (deps_it == dependencies.end()) 
        {
            height_map[pass] = 0;
            continue;
        }
        
        // 如果没有后继节点，高度为0
        if (deps_it->second.dependent_by_passes.is_empty())
        {
            height_map[pass] = 0;
        }
        else
        {
            // 高度 = 1 + 所有后继节点的最大高度
            uint32_t max_height = 0;
            for (auto* dependent : deps_it->second.dependent_by_passes)
            {
                auto height_it = height_map.find(dependent);
                if (height_it != height_map.end())
                {
                    max_height = std::max(max_height, height_it->second);
                }
            }
            height_map[pass] = 1 + max_height;
        }
    }
    
    // 找到高度最大的起始节点
    PassNode* critical_start = nullptr;
    uint32_t max_height = 0;
    
    for (const auto& [pass, deps] : dependencies)
    {
        // 起始节点：没有依赖其他Pass
        if (deps.dependent_passes.is_empty())
        {
            auto height_it = height_map.find(pass);
            if (height_it != height_map.end() && height_it->second > max_height)
            {
                max_height = height_it->second;
                critical_start = pass;
            }
        }
    }
    
    // 沿着高度递减的路径追踪关键路径
    if (critical_start)
    {
        PassNode* current = critical_start;
        
        while (current)
        {
            topology_result_.logical_critical_path.add(current);
            
            // 在所有后继中选择高度最大的
            PassNode* next = nullptr;
            uint32_t next_height = 0;
            
            auto deps_it = dependencies.find(current);
            if (deps_it != dependencies.end())
            {
                for (auto* dependent : deps_it->second.dependent_by_passes)
                {
                    auto height_it = height_map.find(dependent);
                    if (height_it != height_map.end())
                    {
                        if (height_it->second > next_height || 
                           (height_it->second == next_height && !next))
                        {
                            next_height = height_it->second;
                            next = dependent;
                        }
                    }
                }
            }
            
            current = next;
        }
    }
}

void LogicalTopologyAnalyzer::collect_statistics()
{
    topology_result_.total_parallel_opportunities = 0;

    // 统计每个级别内的并行机会
    for (auto& level : topology_result_.logical_levels)
    {
        size_t pass_count = level.passes.size();
        if (pass_count > 1)
        {
            // 该级别内的并行对数
            topology_result_.total_parallel_opportunities += static_cast<uint32_t>((pass_count * (pass_count - 1)) / 2);
        }
        
        // 基础统计信息
        level.total_resources_accessed = 0;
        level.cross_level_dependencies = 0;
    }
}

uint32_t LogicalTopologyAnalyzer::get_topological_order(PassNode* pass) const
{
    auto it = pass_to_order_.find(pass);
    return (it != pass_to_order_.end()) ? it->second : UINT32_MAX;
}

uint32_t LogicalTopologyAnalyzer::get_dependency_level(PassNode* pass) const
{
    for (const auto& level : topology_result_.logical_levels)
    {
        for (auto* level_pass : level.passes)
        {
            if (level_pass == pass)
            {
                return level.level;
            }
        }
    }
    return 0;
}

uint32_t LogicalTopologyAnalyzer::get_critical_path_length(PassNode* pass) const
{
    // 简化实现：返回关键路径中的位置
    for (size_t i = 0; i < topology_result_.logical_critical_path.size(); ++i)
    {
        if (topology_result_.logical_critical_path[i] == pass)
        {
            return static_cast<uint32_t>(topology_result_.logical_critical_path.size() - i - 1);
        }
    }
    return 0;
}

bool LogicalTopologyAnalyzer::can_execute_in_parallel_logically(PassNode* pass1, PassNode* pass2) const
{
    if (!pass1 || !pass2 || pass1 == pass2) return false;
    
    uint32_t level1 = get_dependency_level(pass1);
    uint32_t level2 = get_dependency_level(pass2);
    
    // 同级别的Pass可以并行执行
    return level1 == level2;
}

void LogicalTopologyAnalyzer::dump_topology() const
{
    SKR_LOG_INFO(u8"========== Logical Topology Analysis ==========");
    SKR_LOG_INFO(u8"Max logical dependency depth: %u", topology_result_.max_logical_dependency_depth);
    SKR_LOG_INFO(u8"Total parallel opportunities: %u", topology_result_.total_parallel_opportunities);
    
    SKR_LOG_INFO(u8"\nLogical dependency levels:");
    for (const auto& level : topology_result_.logical_levels)
    {
        SKR_LOG_INFO(u8"Level %u (%u passes):", level.level, static_cast<uint32_t>(level.passes.size()));
        for (auto* pass : level.passes)
        {
            SKR_LOG_INFO(u8"  - %s", pass->get_name());
        }
    }
    SKR_LOG_INFO(u8"=============================================");
}

void LogicalTopologyAnalyzer::dump_critical_path() const
{
    SKR_LOG_INFO(u8"========== Logical Critical Path ==========");
    SKR_LOG_INFO(u8"Critical path length: %u", static_cast<uint32_t>(topology_result_.logical_critical_path.size()));
    
    for (size_t i = 0; i < topology_result_.logical_critical_path.size(); ++i)
    {
        auto* pass = topology_result_.logical_critical_path[i];
        uint32_t level = get_dependency_level(pass);
        SKR_LOG_INFO(u8"[%zu] %s (level: %u)", i, pass->get_name(), level);
    }
    SKR_LOG_INFO(u8"=========================================");
}

} // namespace render_graph
} // namespace skr