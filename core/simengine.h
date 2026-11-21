//SimEngine.h
#pragma once
#include <QVector>
#include "core/Target.h"

// 仿真推进引擎：纯逻辑、线程无关，不依赖 Qt 事件循环/信号槽。
// 目标：把运动模型推进与出界清理从 GUI 线程解耦，
// 后续可整体移入独立工作线程（QThread）——渲染层只消费计算出的快照。
namespace SimEngine {

// 单步推进结果：本帧发生变动的目标 id 与被出界清理的目标 id
struct StepResult {
    QVector<int> changedIds;
    QVector<int> removedIds;
};

// 把目标当前位置追加到航迹并裁剪到 maxPoints（供外部手动改位置复用）
void appendTrailPoint(Target& t, int maxPoints);

// 执行一帧推进：
//  - 自动运动目标按 speedKmh/headingDeg 前进 dtSec
//  - 航迹追加/裁剪
//  - 超出 maxRangeKm 的目标被移除
StepResult step(QVector<Target>& targets, double dtSec,
                double maxRangeKm, int trailMaxPoints);

} // namespace SimEngine