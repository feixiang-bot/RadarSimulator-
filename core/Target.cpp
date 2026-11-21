//Target.cpp
#include "core/Target.h"
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
}

double Target::rangeKm() const {
    return std::hypot(xKm, yKm);
}

double Target::azimuthDeg() const {
    // 正北 0°，顺时针：atan2(x, y)
    double a = std::atan2(xKm, yKm) * 180.0 / kPi;
    if (a < 0) a += 360.0;
    return a;
}