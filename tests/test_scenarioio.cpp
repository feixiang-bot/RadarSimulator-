#include <QtTest>
#include <QTemporaryDir>
#include "core/targetmanager.h"
#include "core/ScenarioIO.h"

class TestScenarioIO : public QObject {
    Q_OBJECT
private slots:
    void saveLoadRoundTrip();
    void loadClearsExisting();
    void loadMissingFileFails();
};

void TestScenarioIO::saveLoadRoundTrip() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("scenario.json");

    TargetManager mgr;
    mgr.setMaxRangeKm(20.0);
    int id1 = mgr.addTargetAt(8.0, 45.0, "Alpha");
    mgr.setTargetMotion(id1, 22.0, 90.0);
    mgr.setTargetAutoMove(id1, true);
    int id2 = mgr.addTargetAt(12.0, 200.0);  // 默认命名 TGT-02
    Q_UNUSED(id2);

    QString err;
    QVERIFY2(ScenarioIO::save(path, mgr, &err), qPrintable(err));

    TargetManager mgr2;
    QVERIFY2(ScenarioIO::load(path, mgr2, &err), qPrintable(err));

    QCOMPARE(mgr2.maxRangeKm(), 20.0);
    QCOMPARE(mgr2.targetCount(), 2);

    const Target& a = mgr2.targetAt(0);
    QCOMPARE(a.name, QString("Alpha"));
    QVERIFY(qAbs(a.rangeKm() - 8.0) < 1e-6);
    QVERIFY(qAbs(a.azimuthDeg() - 45.0) < 1e-6);
    QCOMPARE(a.speedKmh, 22.0);
    QCOMPARE(a.headingDeg, 90.0);
    QCOMPARE(a.autoMove, true);

    const Target& b = mgr2.targetAt(1);
    QCOMPARE(b.name, QString("TGT-02"));
    QVERIFY(qAbs(b.rangeKm() - 12.0) < 1e-6);
    QVERIFY(qAbs(b.azimuthDeg() - 200.0) < 1e-6);
}

void TestScenarioIO::loadClearsExisting() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("scenario.json");

    TargetManager src;
    src.addTargetAt(5.0, 0.0, "A");
    QString err;
    QVERIFY2(ScenarioIO::save(path, src, &err), qPrintable(err));

    // 目标管理器里已有旧目标，加载时应被清空
    TargetManager dst;
    dst.addTargetAt(5.0, 0.0, "old1");
    dst.addTargetAt(6.0, 0.0, "old2");
    QCOMPARE(dst.targetCount(), 2);

    QVERIFY2(ScenarioIO::load(path, dst, &err), qPrintable(err));
    QCOMPARE(dst.targetCount(), 1);
    QCOMPARE(dst.targetAt(0).name, QString("A"));
}

void TestScenarioIO::loadMissingFileFails() {
    QString err;
    TargetManager mgr;
    QVERIFY(!ScenarioIO::load("no_such_file_xyz.json", mgr, &err));
    QVERIFY(!err.isEmpty());
}

QTEST_GUILESS_MAIN(TestScenarioIO)
#include "test_scenarioio.moc"