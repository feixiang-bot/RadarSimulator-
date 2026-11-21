//TargetManager.cpp
#include "core/targetmanager.h"
#include "core/simengine.h"
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
double deg2rad(double d) { return d * kPi / 180.0; }
}

TargetManager::TargetManager(QObject* parent) : QObject(parent) {}

const Target* TargetManager::findById(int id) const {
    for (const auto& t : m_targets)
        if (t.id == id) return &t;
    return nullptr;
}

Target* TargetManager::findByIdMutable(int id) {
    for (auto& t : m_targets)
        if (t.id == id) return &t;
    return nullptr;
}

int TargetManager::addTarget(const Target& t) {
    Target copy = t;
    copy.id = m_nextId++;
    m_targets.append(copy);
    emit targetListChanged();
    return copy.id;
}

void TargetManager::removeTarget(int id) {
    for (int i = 0; i < m_targets.size(); ++i) {
        if (m_targets[i].id == id) {
            m_targets.removeAt(i);
            emit targetListChanged();
            return;
        }
    }
}

void TargetManager::setTargetPolar(int id, double rangeKm, double azimuthDeg) {
    const double a = deg2rad(azimuthDeg);
    setTargetCartesian(id,
                       rangeKm * std::sin(a),   // x 东
                       rangeKm * std::cos(a));  // y 北
}

void TargetManager::setTargetCartesian(int id, double xKm, double yKm) {
    Target* t = findByIdMutable(id);
    if (!t) return;
    t->xKm = xKm;
    t->yKm = yKm;
    appendTrail(t);
    emit targetChanged(id);
}

void TargetManager::appendTrail(Target* t) {
    if (!t) return;
    SimEngine::appendTrailPoint(*t, m_trailMaxPoints);
}

void TargetManager::setTargetMotion(int id, double speedKmh, double headingDeg) {
    Target* t = findByIdMutable(id);
    if (!t) return;
    t->speedKmh    = speedKmh;
    t->headingDeg  = headingDeg;
    emit targetChanged(id);
}

void TargetManager::setTargetAutoMove(int id, bool on) {
    Target* t = findByIdMutable(id);
    if (!t) return;
    t->autoMove = on;
    emit targetChanged(id);
}

// TargetManager.cpp 修改 update：委托纯逻辑引擎 SimEngine 推进，再补发信号
void TargetManager::update(double dtSec) {
    const SimEngine::StepResult r = SimEngine::step(
        m_targets, dtSec, m_maxRangeKm, m_trailMaxPoints);

    // 按原信号时序补发：先全部单目标变更，再逐个出界移除
    for (const int id : r.changedIds)
        emit targetChanged(id);

    for (const int goneId : r.removedIds) {
        emit targetRemoved(goneId);
        emit targetListChanged();
    }
}

void TargetManager::setTargetName(int id, const QString& name) {
    Target* t = findByIdMutable(id);
    if (!t || t->name == name) return;
    t->name = name;
    emit targetChanged(id);
}

bool TargetManager::renameTarget(int id, const QString& newName, QString* errorMsg) {
    const QString trimmed = newName.trimmed();
    if (trimmed.isEmpty()) {
        if (errorMsg) *errorMsg = QString("目标名称不能为空");
        return false;
    }
    for (const auto& t : m_targets) {
        if (t.id == id) continue;   // 改名目标自身不计入重复
        if (t.name == trimmed) {
            if (errorMsg) *errorMsg = QString("目标名称 “%1” 已存在").arg(trimmed);
            return false;
        }
    }
    Target* t = findByIdMutable(id);
    if (!t) return false;
    if (t->name == trimmed) return true;   // 名称未变，视为成功
    t->name = trimmed;
    emit targetChanged(id);
    return true;
}

void TargetManager::setMaxRangeKm(double km) {
    if (qFuzzyCompare(m_maxRangeKm, km)) return;
    m_maxRangeKm = km;
    // 立即清理超出新量程的目标
    for (int i = m_targets.size() - 1; i >= 0; --i) {
        if (m_targets[i].rangeKm() > m_maxRangeKm) {
            const int goneId = m_targets[i].id;
            m_targets.removeAt(i);
            emit targetRemoved(goneId);
        }
    }
    // 量程切换：清空所有剩余目标的航迹
    for (auto& t : m_targets) t.trail.clear();
    emit targetListChanged();   // 统一发一次列表变更
    emit maxRangeChanged(m_maxRangeKm);  // 新增信号，供 UI 同步
}

int TargetManager::addTargetAt(double rangeKm, double azimuthDeg, const QString& name) {
    // 越界拒绝：距离必须在 [0, maxRangeKm] 内
    if (rangeKm < 0.0 || rangeKm > m_maxRangeKm) {
        return -1;   // 表示失败
    }
    Target t;
    t.name = name.isEmpty()
                 ? QString("TGT-%1").arg(m_nextId, 2, 10, QChar('0'))
                 : name;
    const double a = deg2rad(azimuthDeg);
    t.xKm = rangeKm * std::sin(a);
    t.yKm = rangeKm * std::cos(a);
    t.trail.append({t.rangeKm(), t.azimuthDeg()});  // 初始航迹点
    return addTarget(t);
}