# Timeline Phase Stress Tests

这个目录包含了两个综合性的测试程序，用于验证 TimelinePhase 的稳定性和正确性。

## 测试程序

### 1. timeline_stress_test.cpp - 基础压力测试
模拟一个复杂的延迟渲染管线，包含：
- **几何通道**: Shadow Map + G-Buffer 渲染
- **异步计算**: 视锥剔除、粒子模拟、物理模拟
- **光照计算**: 延迟光照（混合Graphics和Compute）
- **拷贝操作**: 独立的纹理拷贝（Copy队列）
- **后处理**: Bloom + Tone Mapping + UI
- **最终输出**: Present Pass

**预期结果**:
- Graphics队列: 5个Pass（Render + Present）
- AsyncCompute队列: 3-5个Pass（独立计算）
- Copy队列: 2个Pass（独立拷贝）
- 跨队列同步: 2-4个同步需求

### 2. advanced_timeline_test.cpp - 高级场景测试
包含5个专门的测试场景：

#### 测试1: 大量并行计算 (Massively Parallel)
- 20个独立计算Pass + 1个Graphics Pass
- 验证异步计算队列的并行调度能力

#### 测试2: 深度依赖链 (Deep Dependency Chain)
- Graphics → Compute → Graphics → Compute 依赖链
- 验证跨队列同步的正确性

#### 测试3: 混合工作负载 (Mixed Workload)
- 10个Render + 15个Compute + 5个Copy Pass
- 验证多队列负载均衡

#### 测试4: 资源密集型 (Resource Heavy)
- 50个纹理 + 30个缓冲区 + 25个随机Pass
- 验证大量资源情况下的调度稳定性

#### 测试5: 边缘情况 (Edge Cases)
- 空RenderGraph
- 只有Present Pass
- 禁用所有异步队列
- 验证极端情况下的鲁棒性

## 运行方法

### 构建
```bash
# 基础压力测试
dotnet run SB build --target timeline-stress-test --mode debug

# 高级场景测试
dotnet run SB build --target advanced-timeline-test --mode debug
```

### 执行
```bash
# 基础测试
.build/clang/debug/bin/timeline-stress-test

# 高级测试
.build/clang/debug/bin/advanced-timeline-test
```

## 关键验证点

### ✅ 调度正确性
- [ ] Pass类型正确分配到对应队列
- [ ] Present/Render Pass强制Graphics队列
- [ ] 无Graphics依赖的Compute Pass分配到AsyncCompute
- [ ] 标记为lone的Copy Pass分配到Copy队列

### ✅ 同步正确性
- [ ] 跨队列依赖正确识别
- [ ] 同步需求包含正确的signal/wait Pass
- [ ] 每个同步需求都分配了Fence对象
- [ ] 没有多余的同步开销

### ✅ 资源管理
- [ ] 每帧数据正确清理
- [ ] Fence对象正确分配和释放
- [ ] 内存无泄漏

### ✅ 边缘情况处理
- [ ] 空图处理
- [ ] 单Pass处理
- [ ] 队列禁用处理
- [ ] 错误恢复

## 性能指标

### 延迟要求
- TimelinePhase执行时间 < 1ms（小规模图）
- TimelinePhase执行时间 < 5ms（大规模图，100+ Pass）

### 内存要求
- 峰值内存使用量与Pass数量线性增长
- 帧间无内存积累

### 正确性要求
- 所有Pass都被正确分配
- 跨队列依赖100%识别
- 同步点数量最小化

## 调试输出

测试程序会输出详细的调度结果，包括：
- 📅 队列调度信息（每个队列的Pass序列）
- 🔄 同步需求（跨队列依赖和Fence分配）
- 📊 Pass分配统计（Graphics/Compute/Copy分布）

## 故障排除

### 常见问题
1. **编译错误**: 确认SkrRenderGraph模块已正确构建
2. **运行时崩溃**: 检查Pass创建过程中的资源引用
3. **调度异常**: 查看日志中的队列能力检测结果

### 调试技巧
1. 使用`dump_timeline_result()`查看详细调度信息
2. 检查`SKR_LOG_DEBUG`输出的队列能力
3. 验证同步需求中的Fence分配

## 扩展测试

可以通过修改以下参数来创建更复杂的测试场景：
- Pass数量和类型比例
- 资源大小和数量
- 依赖关系复杂度
- 队列配置（启用/禁用异步队列）

这些测试确保TimelinePhase在各种真实和极端场景下都能正确工作，为整个RenderGraph Phase系统提供可靠的调度基础。