 #include "ui/radarwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QtMath>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include "core/targetmanager.h"

RadarWidget::RadarWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(400, 400);
    setAutoFillBackground(false);

    m_target.id = 1;
    m_target.name = "TGT-01";

    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    m_timer->setInterval(8); // ~60fps
    connect(m_timer, &QTimer::timeout, this, &RadarWidget::onFrameTick);

    m_clock.start();
    m_lastNs = m_clock.nsecsElapsed();
    m_timer->start();
}

void RadarWidget::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    rebuildGeometry();
    ensurePhosphorSize();
}

void RadarWidget::rebuildGeometry() {
    const double w = width();
    const double h = height();
    const double side = qMin(w, h);

    // 留 8% 边距给方位刻度文字
    const double margin = side * 0.08;
    const double radius = side * 0.5 - margin;

    m_coord.setGeometry(QPointF(w * 0.5, h * 0.5), radius, maxRangeKm());
}

void RadarWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    drawBackground(p);       // 静态圆盘
    drawRangeRings(p);
    drawAzimuthTicks(p);
    drawCrosshair(p);

    // 荧光屏叠加：只画目标余辉，不含扫描线
    if (!m_phosphor.isNull()) {
        p.drawImage(0, 0, m_phosphor);
    }

    // C1：航迹尾迹（持久连线，不随余辉衰减），画在余辉之上
    drawTrails(p);

    // 扫描线最后画，压在余辉与航迹之上，且不进 phosphor
    drawSweepLine(p);
}

void RadarWidget::drawBackground(QPainter& p) {
    p.fillRect(rect(), QColor(0, 8, 0)); // 近黑的暗绿底，模拟荧光屏
}

void RadarWidget::drawRangeRings(QPainter& p) {
    const QPointF c = m_coord.centerPx();
    const double R  = m_coord.radiusPx();
    const double maxR = maxRangeKm();

    // 步长：量程/5 向上取整到 1/2/5 的整数
    double step = 2.0;
    if (maxR <= 5.0)       step = 1.0;
    else if (maxR <= 10.0) step = 2.0;
    else if (maxR <= 20.0) step = 5.0;
    else                   step = 10.0;

    p.setPen(QPen(QColor(0, 120, 0), 1.0));
    p.setBrush(Qt::NoBrush);

    for (double km = step; km <= maxR + 1e-6; km += step) {
        const double r = R * (km / maxR);
        p.drawEllipse(c, r, r);
    }

    p.setPen(QColor(0, 180, 0));
    QFont f = p.font();
    f.setPointSize(8);
    p.setFont(f);
    for (double km = step; km <= maxR + 1e-6; km += step) {
        const double r = R * (km / maxR);
        // 避免最外圈文字压到方位刻度：最外圈不标
        if (km >= maxR - 1e-6) continue;
        p.drawText(QPointF(c.x() + 4, c.y() - r - 2),
                   QString("%1km").arg(km, 0, 'g', 3));
    }
}

void RadarWidget::drawAzimuthTicks(QPainter& p) {
    const QPointF c = m_coord.centerPx();
    const double R  = m_coord.radiusPx();

    p.setPen(QPen(QColor(0, 150, 0), 1.0));

    // 每 10° 一个小刻度，每 30° 一个长刻度 + 文字
    for (int deg = 0; deg < 360; deg += 10) {
        const bool major = (deg % 30 == 0);
        const double len = major ? 10.0 : 5.0;

        const QPointF outer = m_coord.polarToScreen(maxRangeKm(), deg);
        const QPointF dir   = (outer - c) / R; // 单位方向
        const QPointF inner = outer - dir * len;

        p.drawLine(inner, outer);

        if (major) {
            const QPointF labelPos = outer + dir * 4.0;
            QString label = QString::number(deg);
            // 让文字以刻度点为中心
            QFontMetrics fm(p.font());
            const int tw = fm.horizontalAdvance(label);
            const int th = fm.height();
            p.drawText(QPointF(labelPos.x() - tw * 0.5,
                               labelPos.y() + th * 0.25), label);
        }
    }
}

void RadarWidget::drawCrosshair(QPainter& p) {
    const QPointF c = m_coord.centerPx();
    const double R  = m_coord.radiusPx();

    p.setPen(QPen(QColor(0, 80, 0), 1.0, Qt::DashLine));
    p.drawLine(QPointF(c.x() - R, c.y()), QPointF(c.x() + R, c.y()));
    p.drawLine(QPointF(c.x(), c.y() - R), QPointF(c.x(), c.y() + R));
    p.setPen(QPen(QColor(0, 80, 0), 1.0));
    p.drawEllipse(c, R, R); // 外圈
}

void RadarWidget::onFrameTick() {
    const qint64 nowNs = m_clock.nsecsElapsed();
    double dt = (nowNs - m_lastNs) / 1e9;
    m_lastNs = nowNs;
    if (dt > 0.25) dt = 0.25;

    // 1) 推进运动模型
    if (m_mgr) m_mgr->update(dt);

    // 2) 推进扫描线
    const double prevAngle = m_sweepAngleDeg;
    m_sweepAngleDeg += m_sweepSpeedDegPerSec * dt;
    if (m_sweepAngleDeg >= 360.0) m_sweepAngleDeg -= 360.0;

    emit sweepAngleChanged(m_sweepAngleDeg);

    // 3) 所有落在本帧扫过区间的目标，点亮
    if (m_mgr) {
        for (int i = 0; i < m_mgr->targetCount(); ++i) {
            const Target& t = m_mgr->targetAt(i);
            if (angleInSweep(t.azimuthDeg(), prevAngle, m_sweepAngleDeg)) {
                paintHitIntoTarget(t);
            }
        }
    }

    // 4) 衰减
    decayPhosphor();

    update();
}

void RadarWidget::drawSweepLine(QPainter& p) {
    const QPointF c = m_coord.centerPx();
    const QPointF tip = m_coord.polarToScreen(maxRangeKm(), m_sweepAngleDeg);

    // 用渐变色模拟"扫过时亮、身后稍暗"的过渡，这一轮先简单一条亮线
    QPen pen(QColor(120, 255, 120), 2.0);
    p.setPen(pen);
    p.drawLine(c, tip);
}

void RadarWidget::drawTarget(QPainter& p, const Target& t) {
    const QPointF pt = m_coord.polarToScreen(t.rangeKm(), t.azimuthDeg());

    // 亮绿实心点 + 外圈
    p.setPen(QPen(QColor(180, 255, 180), 1.5));
    p.setBrush(QColor(80, 255, 80));
    p.drawEllipse(pt, 4.0, 4.0);

    // 小标签
    p.setPen(QColor(180, 255, 180));
    QFont f = p.font();
    f.setPointSize(8);
    p.setFont(f);
    p.drawText(pt + QPointF(8, -8), t.name);
}

void RadarWidget::drawTrails(QPainter& p) {
    if (!m_mgr) return;

    // 亮青色，与绿色荧光余辉区分
    for (int i = 0; i < m_mgr->targetCount(); ++i) {
        const Target& t = m_mgr->targetAt(i);
        if (t.trail.size() < 2) continue;

        QPainterPath path;
        for (int k = 0; k < t.trail.size(); ++k) {
            const QPointF pt = m_coord.polarToScreen(
                t.trail[k].rangeKm, t.trail[k].azimuthDeg);
            if (k == 0) path.moveTo(pt);
            else        path.lineTo(pt);
        }
        p.setPen(QPen(QColor(0, 220, 255, 200), 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);

        // 末端当前目标点
        const QPointF cur = m_coord.polarToScreen(t.rangeKm(), t.azimuthDeg());
        p.setPen(QPen(QColor(0, 240, 255), 1.5));
        p.setBrush(QColor(0, 240, 255));
        p.drawEllipse(cur, 3.0, 3.0);
        p.drawText(cur + QPointF(8, -8), t.name);
    }
}

bool RadarWidget::angleInSweep(double az, double from, double to) {
    // 保证 from, to 都在 [0,360)
    // 判断 az 是否落在 from→to 顺时针的区间内
    // 分两种情况：未跨0° / 跨0°

    auto norm = [](double a) {
        while (a < 0) a += 360.0;
        while (a >= 360.0) a -= 360.0;
        return a;
    };
    az   = norm(az);
    from = norm(from);
    to   = norm(to);

    if (from <= to) {
        // 未跨 0
        return az >= from && az <= to;
    } else {
        // 跨 0：区间是 [from,360) ∪ [0,to]
        return az >= from || az <= to;
    }
}

void RadarWidget::paintHitIntoTarget(const Target& t) {
    if (m_phosphor.isNull()) return;
    QPainter p(&m_phosphor);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QPointF pt = m_coord.polarToScreen(t.rangeKm(), t.azimuthDeg());
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 255, 0, 90));
    p.drawEllipse(pt, 8.0, 8.0);
    p.setBrush(QColor(120, 255, 120, 255));
    p.drawEllipse(pt, 4.0, 4.0);
    p.setBrush(QColor(255, 255, 255, 255));
    p.drawEllipse(pt, 1.8, 1.8);
}

void RadarWidget::decayPhosphor() {
    if (m_phosphor.isNull()) return;

    QPainter p(&m_phosphor);
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    // alpha 越大，拖尾越长（保留率越高）。
    // 240 ≈ 保留94%/帧，半衰约0.18s；想更长调到 248~250。
    p.fillRect(m_phosphor.rect(), QColor(0, 0, 0, m_decayAlpha));
}

void RadarWidget::ensurePhosphorSize() {
    if (m_phosphor.size() == size() * devicePixelRatioF()) return; // 已匹配
    // 重建会清空拖尾，可接受
    m_phosphor = QImage(size() * devicePixelRatioF(),
                        QImage::Format_ARGB32_Premultiplied);
    m_phosphor.setDevicePixelRatio(devicePixelRatioF());
    m_phosphor.fill(Qt::transparent);
}

void RadarWidget::setTargetManager(TargetManager* mgr) {
    if (m_mgr == mgr) return;
    if (m_mgr) disconnect(m_mgr, nullptr, this, nullptr);
    m_mgr = mgr;
    if (m_mgr) {
        connect(m_mgr, &TargetManager::targetChanged,
                this, &RadarWidget::onTargetChanged);
        connect(m_mgr, &TargetManager::targetListChanged,
                this, &RadarWidget::onTargetListChanged); // 复用，忽略 id
        connect(m_mgr, &TargetManager::maxRangeChanged,
                this, &RadarWidget::onMaxRangeChanged);
    }
    update();
}

void RadarWidget::onTargetChanged(int /*id*/) {
    update();  // 简单起见直接重绘，后面可按 id 局部刷新
}

void RadarWidget::onTargetListChanged(){
    update();
}

void RadarWidget::onMaxRangeChanged(double) {
    rebuildGeometry();          // 重算 m_coord 的 maxRangeKm
    if (!m_phosphor.isNull())
        m_phosphor.fill(Qt::transparent);  // 清空旧余辉
    update();
}

int RadarWidget::hitTestTarget(const QPointF& screenPt) const {
    if (!m_mgr) return -1;
    // 倒序遍历：后添加的目标优先命中（视觉上在上层）
    for (int i = m_mgr->targetCount() - 1; i >= 0; --i) {
        const Target& t = m_mgr->targetAt(i);
        const QPointF pt = m_coord.polarToScreen(t.rangeKm(), t.azimuthDeg());
        const QPointF d = pt - screenPt;
        if (std::hypot(d.x(), d.y()) <= kHitRadiusPx)
            return t.id;
    }
    return -1;
}

void RadarWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton || !m_mgr) {
        QWidget::mousePressEvent(e);
        return;
    }
    const QPointF pos = e->position();
    const int hitId = hitTestTarget(pos);

    if (hitId >= 0) {
        // 命中目标：进入拖拽候选
        m_dragTargetId = hitId;
        m_pressPos = pos;
        m_dragging = false;
        const Target* t = m_mgr->findById(hitId);
        if (t) {
            const QPointF targetPt = m_coord.polarToScreen(t->rangeKm(), t->azimuthDeg());
            m_dragOffset = targetPt - pos;
        }
        setCursor(Qt::ClosedHandCursor);
        grabMouse();  // D3：拖拽候选即抓取鼠标，移出窗口仍能收到 move/release
    } else {
        // 空白处：记录按下位置，等释放时决定是否生成
        m_dragTargetId = -1;
        m_pressPos = pos;
        m_dragging = false;
    }
    e->accept();
}

void RadarWidget::mouseMoveEvent(QMouseEvent* e) {
    if (!m_mgr) { QWidget::mouseMoveEvent(e); return; }

    const QPointF pos = e->position();

    double mouseRangeKm = 0.0, mouseAzDeg = 0.0;
    m_coord.screenToPolar(pos, mouseRangeKm, mouseAzDeg);
    emit cursorPolarChanged(mouseRangeKm, mouseAzDeg);

    if (m_dragTargetId >= 0) {
        // 已在拖拽目标
        if (!m_dragging) {
            const QPointF d = pos - m_pressPos;
            if (std::hypot(d.x(), d.y()) > kDragThresholdPx)
                m_dragging = true;
        }
        if (m_dragging) {
            const QPointF targetPos = pos + m_dragOffset;
            double r, a;
            m_coord.screenToPolar(targetPos, r, a);
            const double maxR = m_mgr->maxRangeKm();
            if (r < 0) r = 0;
            if (r > maxR) r = maxR;
            m_mgr->setTargetPolar(m_dragTargetId, r, a);
            e->accept();
        }
        return;
    }

    // 没在拖拽：更新光标（悬停在目标上变手型）
    const int hoverId = hitTestTarget(pos);
    setCursor(hoverId >= 0 ? Qt::OpenHandCursor : Qt::ArrowCursor);
    QWidget::mouseMoveEvent(e);
}

void RadarWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton || !m_mgr) {
        QWidget::mouseReleaseEvent(e);
        return;
    }
    releaseMouse();  // D3：结束拖拽抓取（未抓取时为空操作）
    const QPointF pos = e->position();

    if (m_dragTargetId >= 0) {
        // 拖拽结束
        m_dragTargetId = -1;
        m_dragging = false;
        setCursor(Qt::ArrowCursor);
    } else {
        // 空白处释放：若没怎么移动，视为点击 → 生成目标
        const QPointF d = pos - m_pressPos;
        if (std::hypot(d.x(), d.y()) <= kDragThresholdPx) {
            double r, a;
            m_coord.screenToPolar(pos, r, a);
            const double maxR = m_mgr->maxRangeKm();
            if (r >= 0 && r <= maxR) {
                const int n = m_mgr->targetCount();
                m_mgr->addTargetAt(r, a);
            }
        }
    }
    e->accept();
}

void RadarWidget::contextMenuEvent(QContextMenuEvent* e) {
    if (!m_mgr) return;

    const int id = hitTestTarget(QPointF(e->pos()));
    if (id < 0) return;

    QMenu menu(this);
    QAction* actDel  = menu.addAction("删除目标");
    QAction* actEdit = menu.addAction("属性编辑...");

    const QAction* chosen = menu.exec(e->globalPos());
    if (chosen == actDel) {
        emit targetRemoveRequested(id);
    } else if (chosen == actEdit) {
        emit targetEditRequested(id);
    }
}