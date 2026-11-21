//TargetManager.h
#pragma once
#include <QObject>
#include <QVector>
#include "core/Target.h"

class TargetManager : public QObject {
    Q_OBJECT
public:
    explicit TargetManager(QObject* parent = nullptr);

    // 查询
    int targetCount() const { return m_targets.size(); }
    const Target& targetAt(int index) const { return m_targets[index]; }
    const Target* findById(int id) const;

    // 增删
    int  addTarget(const Target& t);   // 返回分配到的 id
    void removeTarget(int id);

    // 修改位置（极坐标入口，内部转直角坐标）
    void setTargetPolar(int id, double rangeKm, double azimuthDeg);

    // 修改位置（直角坐标入口）
    void setTargetCartesian(int id, double xKm, double yKm);

    // 修改运动参数
    void setTargetMotion(int id, double speedKmh, double headingDeg);
    void setTargetAutoMove(int id, bool on);

    // 每帧推进（运动模型）
    void update(double dtSec);

    void setTargetName(int id, const QString& name);

    // D4：校验并改名（非空 + 唯一）。失败返回 false 并填 errorMsg。
    bool renameTarget(int id, const QString& newName, QString* errorMsg = nullptr);

    //量程与出界处理
    void setMaxRangeKm(double km);
    double maxRangeKm() const { return m_maxRangeKm; }

    int addTargetAt(double rangeKm, double azimuthDeg, const QString& name = {});

    // 航迹尾迹：每目标保留的历史点数上限（>=2）
    void setTrailMaxPoints(int n) { m_trailMaxPoints = qMax(2, n); }
    int  trailMaxPoints() const { return m_trailMaxPoints; }

signals:
    void targetChanged(int id);          // 单目标变更
    void targetListChanged();            // 增删导致列表结构变化
    // 新增：某个目标因出界被移除（可选，便于 UI 提示）
    void targetRemoved(int id);
    void maxRangeChanged(double);

private:
    Target* findByIdMutable(int id);
    void appendTrail(Target* t);     // 记录当前位置到航迹并裁剪
    QVector<Target> m_targets;
    int m_nextId = 1;
    double m_maxRangeKm = 10.0;
    int m_trailMaxPoints = 100;      // 每目标航迹点数上限
};