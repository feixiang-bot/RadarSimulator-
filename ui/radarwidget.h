// src/ui/RadarWidget.h
#pragma once
#include <QWidget>
#include <QElapsedTimer>
#include "core/coordconverter.h"
#include "core/Target.h"
#include "core/targetmanager.h"

class QTimer;

class RadarWidget : public QWidget {
    Q_OBJECT
public:
    explicit RadarWidget(QWidget* parent = nullptr);

    void setTargetManager(TargetManager* mgr);   // 新增：注入数据源

    void setSweepSpeedDegPerSec(double v) { m_sweepSpeedDegPerSec = v; }
    double sweepSpeedDegPerSec() const { return m_sweepSpeedDegPerSec; }

    void setDecayAlpha(int a) { m_decayAlpha = a; }
    int  decayAlpha() const { return m_decayAlpha; }

signals:
    // D1：状态栏数据
    void sweepAngleChanged(double deg);                    // 每帧当前扫描方位
    void cursorPolarChanged(double rangeKm, double azDeg); // 鼠标位置极坐标
    // D2：右键菜单
    void targetRemoveRequested(int id);
    void targetEditRequested(int id);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;    // 新增
    void mouseMoveEvent(QMouseEvent* event) override;     // 新增
    void mouseReleaseEvent(QMouseEvent* event) override;  // 新增
    void contextMenuEvent(QContextMenuEvent* event) override; // D2：右键菜单

private slots:
    void onFrameTick();               // 帧驱动：推进扫描线、请求重绘
    void onTargetChanged(int id);                // 数据变更 → 请求重绘
    void onTargetListChanged();
    void onMaxRangeChanged(double km);   // 新增

private:
    void rebuildGeometry();
    void drawBackground(QPainter& p);
    void drawRangeRings(QPainter& p);
    void drawAzimuthTicks(QPainter& p);
    void drawCrosshair(QPainter& p);
    void drawTarget(QPainter& p, const Target& t);  // 新增
    void drawSweepLine(QPainter& p);                // 新增
    void drawTrails(QPainter& p);                   // C1：航迹尾迹连线
    void ensurePhosphorSize();                    // 尺寸变化时重建
    void decayPhosphor();                         // 整体压暗
    void paintHitIntoTarget(const Target& t);           // 若被扫到，点亮
    static bool angleInSweep(double az, double from, double to); // 跨0°判定

    QImage m_phosphor;        // 荧光屏画布（ARGB32_Premultiplied）
    double m_prevSweepAngleDeg = 0.0;

    CoordConverter m_coord;

    double maxRangeKm() const { return m_mgr ? m_mgr->maxRangeKm() : 10.0; }

    // --- 帧驱动 ---
    QTimer*        m_timer = nullptr;
    QElapsedTimer  m_clock;
    qint64         m_lastNs = 0;

    // --- 扫描线 ---
    double m_sweepAngleDeg = 0.0;      // 当前扫描线方位
    double m_sweepSpeedDegPerSec = 30.0; // 12 秒/圈

    // --- 目标（临时硬编码，下一轮换 TargetManager）---
    Target m_target;

    TargetManager*  m_mgr = nullptr;

    int m_decayAlpha = 240;   // 新增：余辉衰减 alpha

    // 命中测试：返回鼠标位置命中的目标 id，未命中返回 -1
    int hitTestTarget(const QPointF& screenPt) const;

    // 拖拽状态
    int    m_dragTargetId = -1;      // -1 表示未拖拽
    QPointF m_pressPos;              // 按下位置（屏幕坐标）
    bool   m_dragging = false;       // 是否已进入拖拽（移动超过阈值）

    static constexpr double kHitRadiusPx = 12.0;  // 命中半径
    static constexpr double kDragThresholdPx = 3.0; // 拖拽启动阈值
    //记录按下时的鼠标偏移量
    QPointF m_dragOffset;
};