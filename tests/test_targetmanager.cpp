#include <QtTest>
#include "core/targetmanager.h"

class TestTargetManager : public QObject {
    Q_OBJECT
private slots:
    void addTargetIncrementsId();
    void defaultNaming();
    void addTargetAtRejectsOutOfRange();
    void removeTarget();
    void setPolarAndCartesian();
    void setMaxRangeCleans();
    void updateMovesAndCleans();
    void findById();
    void trailAppendAndTrim();
    void trailOnAutoMove();
    void trailClearedOnRangeChangeAndRemove();
    void renameToUniqueSucceeds();
    void renameToDuplicateFails();
    void renameToEmptyFails();
    void renameUnchangedSucceeds();
};

void TestTargetManager::addTargetIncrementsId() {
    TargetManager mgr;
    Target t;
    int id1 = mgr.addTarget(t);
    int id2 = mgr.addTarget(t);
    QCOMPARE(id1, 1);
    QCOMPARE(id2, 2);
    QCOMPARE(mgr.targetCount(), 2);
    QVERIFY(mgr.findById(1) != nullptr);
    QVERIFY(mgr.findById(2) != nullptr);
}

void TestTargetManager::defaultNaming() {
    TargetManager mgr;
    int id1 = mgr.addTargetAt(5.0, 0.0);
    int id2 = mgr.addTargetAt(5.0, 45.0);
    QCOMPARE(id1, 1);
    QCOMPARE(id2, 2);
    QCOMPARE(mgr.targetAt(0).name, QString("TGT-01"));
    QCOMPARE(mgr.targetAt(1).name, QString("TGT-02"));
}

void TestTargetManager::addTargetAtRejectsOutOfRange() {
    TargetManager mgr;  // 默认量程 10.0
    QCOMPARE(mgr.addTargetAt(11.0, 0.0), -1);
    QCOMPARE(mgr.addTargetAt(-0.5, 0.0), -1);
    QCOMPARE(mgr.targetCount(), 0);
    // 正好在量程圆周上是有效的
    QCOMPARE(mgr.addTargetAt(10.0, 0.0), 1);
}

void TestTargetManager::removeTarget() {
    TargetManager mgr;
    int id = mgr.addTargetAt(5.0, 0.0);
    QCOMPARE(mgr.targetCount(), 1);
    mgr.removeTarget(id);
    QCOMPARE(mgr.targetCount(), 0);
    QVERIFY(mgr.findById(id) == nullptr);
    // 删除不存在的 id 不应崩溃
    mgr.removeTarget(999);
    QCOMPARE(mgr.targetCount(), 0);
}

void TestTargetManager::setPolarAndCartesian() {
    TargetManager mgr;
    int id = mgr.addTargetAt(5.0, 0.0);

    // 极坐标入口：东向 10 km
    mgr.setTargetPolar(id, 10.0, 90.0);
    const Target* t = mgr.findById(id);
    QVERIFY(t != nullptr);
    QVERIFY(qAbs(t->xKm - 10.0) < 1e-9);
    QVERIFY(qAbs(t->yKm - 0.0) < 1e-9);

    // 直角坐标入口：正北 10 km
    mgr.setTargetCartesian(id, 0.0, 10.0);
    QVERIFY(qAbs(t->rangeKm() - 10.0) < 1e-9);
    QVERIFY(qAbs(t->azimuthDeg() - 0.0) < 1e-9);
}

void TestTargetManager::setMaxRangeCleans() {
    TargetManager mgr;
    int id = mgr.addTargetAt(5.0, 0.0);
    QCOMPARE(mgr.targetCount(), 1);

    mgr.setMaxRangeKm(3.0);  // 5 km > 3 km，应立即被清理
    QCOMPARE(mgr.maxRangeKm(), 3.0);
    QCOMPARE(mgr.targetCount(), 0);
    QVERIFY(mgr.findById(id) == nullptr);
}

void TestTargetManager::updateMovesAndCleans() {
    // 运动推进：36 km/h，航向正北，推进 1 小时 → 北移 36 km
    {
        TargetManager mgr;
        mgr.setMaxRangeKm(100.0);
        int id = mgr.addTargetAt(0.0, 0.0);
        mgr.setTargetMotion(id, 36.0, 0.0);
        mgr.setTargetAutoMove(id, true);
        mgr.update(3600.0);

        const Target* t = mgr.findById(id);
        QVERIFY(t != nullptr);
        QVERIFY(qAbs(t->xKm - 0.0) < 1e-6);
        QVERIFY(qAbs(t->yKm - 36.0) < 1e-6);
        QVERIFY(qAbs(t->rangeKm() - 36.0) < 1e-6);
    }

    // 出界清理：默认量程 10 km，推进后 5+36=41 km > 10 → 被移除
    {
        TargetManager mgr;
        int id = mgr.addTargetAt(5.0, 0.0);
        mgr.setTargetMotion(id, 36.0, 0.0);
        mgr.setTargetAutoMove(id, true);
        mgr.update(3600.0);
        QCOMPARE(mgr.targetCount(), 0);
    }
}

void TestTargetManager::findById() {
    TargetManager mgr;
    QVERIFY(mgr.findById(1) == nullptr);
}

void TestTargetManager::trailAppendAndTrim() {
    TargetManager mgr;
    int id = mgr.addTargetAt(2.0, 0.0);
    QCOMPARE(int(mgr.findById(id)->trail.size()), 1);  // 初始 1 个航迹点

    mgr.setTrailMaxPoints(3);
    for (int k = 0; k < 10; ++k)
        mgr.setTargetCartesian(id, double(k), 0.0);

    const Target* t = mgr.findById(id);
    QVERIFY(t != nullptr);
    QCOMPARE(int(t->trail.size()), 3);  // 裁剪到上限 3 点
    // 最后一点 == 当前位置
    QVERIFY(qAbs(t->trail.last().rangeKm - t->rangeKm()) < 1e-9);
    QVERIFY(qAbs(t->trail.last().azimuthDeg - t->azimuthDeg()) < 1e-9);
}

void TestTargetManager::trailOnAutoMove() {
    TargetManager mgr;
    mgr.setMaxRangeKm(100.0);
    int id = mgr.addTargetAt(0.0, 0.0);
    QCOMPARE(int(mgr.findById(id)->trail.size()), 1);

    mgr.setTargetMotion(id, 36.0, 0.0);
    mgr.setTargetAutoMove(id, true);
    mgr.update(1.0);  // 推进 1 秒，应追加 1 个航迹点

    QVERIFY(mgr.findById(id)->trail.size() >= 2);
}

void TestTargetManager::trailClearedOnRangeChangeAndRemove() {
    TargetManager mgr;
    int id1 = mgr.addTargetAt(2.0, 0.0);
    mgr.setTargetCartesian(id1, 4.0, 0.0);  // 追加 1 点
    QVERIFY(mgr.findById(id1)->trail.size() >= 2);

    mgr.setMaxRangeKm(5.0);  // 4 km 仍在量程内，不删除目标，但航迹清空
    QCOMPARE(int(mgr.findById(id1)->trail.size()), 0);

    // 删除目标：航迹随目标一起销毁
    int id2 = mgr.addTargetAt(3.0, 90.0);
    mgr.setTargetCartesian(id2, 3.5, 0.0);
    QVERIFY(mgr.findById(id2)->trail.size() >= 2);
    mgr.removeTarget(id2);
    QVERIFY(mgr.findById(id2) == nullptr);
}

void TestTargetManager::renameToUniqueSucceeds() {
    TargetManager mgr;
    int id = mgr.addTargetAt(2.0, 0.0);   // 名 TGT-01
    QString err;
    QVERIFY(mgr.renameTarget(id, "Ghost-9", &err));
    QCOMPARE(mgr.findById(id)->name, QString("Ghost-9"));
    QVERIFY(err.isEmpty());
}

void TestTargetManager::renameToDuplicateFails() {
    TargetManager mgr;
    mgr.addTargetAt(2.0, 0.0);             // TGT-01
    int id2 = mgr.addTargetAt(3.0, 90.0);  // TGT-02
    QString err;
    QVERIFY(!mgr.renameTarget(id2, "TGT-01", &err));
    QVERIFY(!err.isEmpty());
    QCOMPARE(mgr.findById(id2)->name, QString("TGT-02"));  // 未被修改
}

void TestTargetManager::renameToEmptyFails() {
    TargetManager mgr;
    int id = mgr.addTargetAt(2.0, 0.0);
    QString err;
    QVERIFY(!mgr.renameTarget(id, "   ", &err));
    QVERIFY(!err.isEmpty());
    QCOMPARE(mgr.findById(id)->name, QString("TGT-01"));    // 未被修改
}

void TestTargetManager::renameUnchangedSucceeds() {
    TargetManager mgr;
    int id = mgr.addTargetAt(2.0, 0.0);   // TGT-01
    QString err;
    QVERIFY(mgr.renameTarget(id, "TGT-01", &err));          // 改成自己，不算重复
    QCOMPARE(mgr.findById(id)->name, QString("TGT-01"));
}

QTEST_GUILESS_MAIN(TestTargetManager)
#include "test_targetmanager.moc"