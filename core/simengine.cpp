//SimEngine.cpp
#include "core/simengine.h"
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
double deg2rad(double d) { return d * kPi / 180.0; }
}

namespace SimEngine {

void appendTrailPoint(Target& t, int maxPoints) {
    t.trail.append({t.rangeKm(), t.azimuthDeg()});
    while (t.trail.size() > maxPoints)
        t.trail.removeFirst();
}

StepResult step(QVector<Target>& targets, double dtSec,
                double maxRangeKm, int trailMaxPoints) {
    StepResult res;

    // 1) 推进自动目标 + 记录航迹
    for (auto& t : targets) {
        if (!t.autoMove || t.speedKmh <= 0.0) continue;
        const double distKm = t.speedKmh * (dtSec / 3600.0);
        const double h = deg2rad(t.headingDeg);
        t.xKm += distKm * std::sin(h);
        t.yKm += distKm * std::cos(h);
        appendTrailPoint(t, trailMaxPoints);
        res.changedIds.append(t.id);
    }

    // 2) 出界清理（倒序删除安全）
    for (int i = targets.size() - 1; i >= 0; --i) {
        if (targets[i].rangeKm() > maxRangeKm) {
            res.removedIds.append(targets[i].id);
            targets.removeAt(i);
        }
    }

    return res;
}

} // namespace SimEngine