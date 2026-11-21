#include "core/coordconverter.h"
#include <cmath>

namespace{
constexpr double kPi = 3.14159265358979323846;
//度转弧度
double degToRad(double d) {return d * kPi / 180.0;}
}

void CoordConverter::setGeometry(const QPointF& centerPx, double radiusPx, double maxRangeKm){
    m_centerPx = centerPx;
    m_radiusPx = radiusPx;
    m_maxRangeKm = maxRangeKm;
}

//极坐标 -> 屏幕像素
QPointF CoordConverter::polarToScreen(double rangeKm, double azimuthDeg) const{
    const double rPx = (rangeKm / m_maxRangeKm) * m_radiusPx;
    const double a   = degToRad(azimuthDeg);
    // 正北朝上：x 用 sin，y 取负用 cos
    return QPointF(m_centerPx.x() + rPx * std::sin(a),
                   m_centerPx.y() - rPx * std::cos(a));
}

//屏幕像素 -> 极坐标(鼠标交互，点击目标时使用)
void CoordConverter::screenToPolar(const QPointF& screenPt,double& rangeKm,double& azimuthDeg) const{
    const double dx = screenPt.x() - m_centerPx.x();
    const double dy = screenPt.y() - m_centerPx.y();
    const double rPx = std::hypot(dx, dy);

    rangeKm = (rPx / m_radiusPx) * m_maxRangeKm;

    // atan2(dx, -dy)：正北 0°，顺时针
    double a = std::atan2(dx, -dy) * 180.0 / kPi;
    if (a < 0) a += 360.0;
    azimuthDeg = a;
}