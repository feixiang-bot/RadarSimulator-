//Target.h
#pragma once
#include <QString>
#include <QVector>

// 数据层：内部用直角坐标（km），对外提供极坐标。
// 坐标约定：x 正东，y 正北，单位 km。
// 方位角正北 0°，顺时针。
struct Target {
    int     id        = 0;
    QString name;

    // 内部存储（km）
    double  xKm       = 0.0;
    double  yKm       = 0.0;

    // 运动参数
    double  speedKmh  = 0.0;
    double  headingDeg = 0.0;   // 航向，正北 0° 顺时针
    bool    autoMove  = false;

    // 航迹尾迹：目标历史位置（极坐标，旧→新），区别于荧光余辉
    struct TrailPoint {
        double rangeKm    = 0.0;
        double azimuthDeg = 0.0;
    };
    QVector<TrailPoint> trail;

    // ---- 极坐标便捷访问 ----
    double rangeKm() const;
    double azimuthDeg() const;
};