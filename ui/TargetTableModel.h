//TargetTableModel.h
#pragma once
#include <QAbstractTableModel>

class TargetManager;
class QTimer;

class TargetTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit TargetTableModel(TargetManager* mgr, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation o, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

private slots:
    void onTargetChanged(int id);
    void onTargetListChanged();

private:
    void flushPending();   // 批量刷新脏数据

    TargetManager* m_mgr = nullptr;
    QTimer* m_refreshTimer = nullptr;   // 表格刷新节流定时器
    bool m_dirty = false;
};