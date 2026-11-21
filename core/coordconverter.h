//全局唯一坐标换算
#pragma once
#include <QPointF>

// 约定：
//  - 数据层：极坐标 (rangeKm, azimuthDeg)，方位角正北 0°，顺时针增大
//  - 屏幕层：像素坐标，x 向右，y 向下
//  - 圆心 centerPx，雷达半径 radiusPx 对应 maxRangeKm

class CoordConverter
{
public:
    CoordConverter() = default;

    void setGeometry(const QPointF& centerPx, double radiusPx, double maxRangeKm);

    //极坐标 -> 屏幕像素
    QPointF polarToScreen(double rangeKm, double azimuthDeg) const;

    //屏幕像素 -> 极坐标(鼠标交互，点击目标时使用)
    void screenToPolar(const QPointF& screenPt,double& rangeKm,double& azimuthDeg) const;

    double radiusPx() const {return m_radiusPx;}
    double maxRangeKm() const {return m_maxRangeKm;}
    QPointF centerPx() const {return m_centerPx;}
private:
    double m_radiusPx{0.0};
    double m_maxRangeKm{0.0};
    QPointF m_centerPx{0,0};
};

