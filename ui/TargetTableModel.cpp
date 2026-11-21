// src/ui/TargetTableModel.cpp
#include "ui/TargetTableModel.h"
#include "core/targetmanager.h"
#include <QTimer>
#include <QMessageBox>
#include <QWidget>

namespace {
enum Col { ColName = 0, ColRange, ColAzimuth, ColSpeed, ColHeading, ColAuto, ColCount };
}

TargetTableModel::TargetTableModel(TargetManager* mgr, QObject* parent)
    : QAbstractTableModel(parent), m_mgr(mgr) {
    connect(m_mgr, &TargetManager::targetChanged,
            this, &TargetTableModel::onTargetChanged);
    connect(m_mgr, &TargetManager::targetListChanged,
            this, &TargetTableModel::onTargetListChanged);

    // C2：表格刷新节流——自动运动每帧 emit targetChanged 会触发大量 dataChanged，
    // 用脏标记 + 0ms 单次定时器把同一事件循环内的多次变更合并成一次批量刷新。
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(0);
    connect(m_refreshTimer, &QTimer::timeout,
            this, &TargetTableModel::flushPending);
}

int TargetTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_mgr->targetCount();
}

int TargetTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColCount;
}

QVariant TargetTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {};
    const Target& t = m_mgr->targetAt(index.row());

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case ColName:    return t.name;
        case ColRange:   return QString::number(t.rangeKm(), 'f', 2);
        case ColAzimuth: return QString::number(t.azimuthDeg(), 'f', 1);
        case ColSpeed:   return QString::number(t.speedKmh, 'f', 1);
        case ColHeading: return QString::number(t.headingDeg, 'f', 1);
        case ColAuto:    return t.autoMove;
        }
    }
    if (role == Qt::CheckStateRole && index.column() == ColAuto)
        return t.autoMove ? Qt::Checked : Qt::Unchecked;

    return {};
}

QVariant TargetTableModel::headerData(int section, Qt::Orientation o, int role) const {
    if (role != Qt::DisplayRole || o != Qt::Horizontal) return {};
    switch (section) {
    case ColName:    return "名称";
    case ColRange:   return "距离(km)";
    case ColAzimuth: return "方位(°)";
    case ColSpeed:   return "速度(km/h)";
    case ColHeading: return "航向(°)";
    case ColAuto:    return "自动";
    }
    return {};
}

Qt::ItemFlags TargetTableModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    Qt::ItemFlags f = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.column() == ColAuto)
        f |= Qt::ItemIsUserCheckable;
    else
        f |= Qt::ItemIsEditable;
    return f;
}

bool TargetTableModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid()) return false;
    const Target& t = m_mgr->targetAt(index.row());
    const int id = t.id;

    if (role == Qt::CheckStateRole && index.column() == ColAuto) {
        m_mgr->setTargetAutoMove(id, value.toInt() == Qt::Checked);
        return true;
    }
    if (role != Qt::EditRole) return false;

    bool ok = false;
    switch (index.column()) {
    case ColName: {
        QString err;
        if (!m_mgr->renameTarget(id, value.toString(), &err)) {
            QMessageBox::warning(qobject_cast<QWidget*>(parent()),
                                 "名称无效", err);
            return false;
        }
        return true;
    }
    case ColRange: {
        const double r = value.toDouble(&ok);
        if (!ok) return false;
        if (r < 0.0 || r > m_mgr->maxRangeKm()) return false;  // 拒绝越界输入
        m_mgr->setTargetPolar(id, r, t.azimuthDeg());
        return true;
    }
    case ColAzimuth: {
        const double a = value.toDouble(&ok);
        if (ok) m_mgr->setTargetPolar(id, t.rangeKm(), a);
        return ok;
    }
    case ColSpeed: {
        const double s = value.toDouble(&ok);
        if (ok) m_mgr->setTargetMotion(id, s, t.headingDeg);
        return ok;
    }
    case ColHeading: {
        const double h = value.toDouble(&ok);
        if (ok) m_mgr->setTargetMotion(id, t.speedKmh, h);
        return ok;
    }
    }
    return false;
}

void TargetTableModel::onTargetChanged(int id) {
    Q_UNUSED(id);
    // 只标记脏，不立即发 dataChanged；由定时器统一批量刷新
    m_dirty = true;
    if (!m_refreshTimer->isActive())
        m_refreshTimer->start();
}

void TargetTableModel::flushPending() {
    if (!m_dirty) return;
    m_dirty = false;
    const int n = rowCount(QModelIndex());
    if (n > 0)
        emit dataChanged(index(0, 0), index(n - 1, ColCount - 1));
}

void TargetTableModel::onTargetListChanged() {
    beginResetModel();
    endResetModel();
    // 结构已变，取消待刷新的脏标记
    m_dirty = false;
    m_refreshTimer->stop();
}