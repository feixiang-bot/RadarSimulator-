#include <QtTest>
#include "core/simengine.h"
#include "core/Target.h"

class TestSimEngine : public QObject {
    Q_OBJECT
private slots:
    void stepMovesAutoTargetNorth();
    void stepHeadingEast();
    void stepSkipsStatic();
    void stepRemovesOutOfRange();
    void appendTrailTrims();
};

void TestSimEngine::stepMovesAutoTargetNorth() {
    QVector<Target> targets;
    Target t;
    t.id = 1;
    t.speedKmh = 3600.0;  // 1 km/s
    t.headingDeg = 0.0;   // 正北
    t.autoMove = true;
    targets.append(t);

    const SimEngine::StepResult r = SimEngine::step(targets, 1.0, 100.0, 100);

    QCOMPARE(r.changedIds.size(), 1);
    QCOMPARE(r.changedIds[0], 1);
    QVERIFY(r.removedIds.isEmpty());

    // 正北 → y +1km，x 不变
    QVERIFY(qAbs(targets[0].yKm - 1.0) < 1e-9);
    QVERIFY(qAbs(targets[0].xKm) < 1e-9);

    QCOMPARE(targets[0].trail.size(), 1);
    QVERIFY(qAbs(targets[0].trail.last().rangeKm - 1.0) < 1e-9);
    QVERIFY(qAbs(targets[0].trail.last().azimuthDeg - 0.0) < 1e-9);
}

void TestSimEngine::stepHeadingEast() {
    QVector<Target> targets;
    Target t;
    t.id = 2;
    t.speedKmh = 3600.0;
    t.headingDeg = 90.0;  // 正东
    t.autoMove = true;
    targets.append(t);

    SimEngine::step(targets, 1.0, 100.0, 100);

    QVERIFY(qAbs(targets[0].xKm - 1.0) < 1e-9);
    QVERIFY(qAbs(targets[0].yKm) < 1e-9);
}

void TestSimEngine::stepSkipsStatic() {
    QVector<Target> targets;

    Target a;
    a.id = 3;
    a.speedKmh = 100.0;   // 有速度但未开自动
    a.headingDeg = 45.0;
    a.autoMove = false;
    targets.append(a);

    Target b;
    b.id = 4;
    b.speedKmh = 0.0;     // 自动但速度为零
    b.headingDeg = 45.0;
    b.autoMove = true;
    targets.append(b);

    const SimEngine::StepResult r = SimEngine::step(targets, 1.0, 100.0, 100);

    QVERIFY(r.changedIds.isEmpty());
    for (const Target& x : targets) {
        QVERIFY(qAbs(x.xKm) < 1e-12);
        QVERIFY(qAbs(x.yKm) < 1e-12);
        QVERIFY(x.trail.isEmpty());
    }
}

void TestSimEngine::stepRemovesOutOfRange() {
    QVector<Target> targets;
    Target t;
    t.id = 5;
    t.yKm = 20.0;          // range 20 > maxRange 10
    targets.append(t);

    const SimEngine::StepResult r = SimEngine::step(targets, 0.016, 10.0, 100);

    QVERIFY(targets.isEmpty());
    QCOMPARE(r.removedIds.size(), 1);
    QCOMPARE(r.removedIds[0], 5);
}

void TestSimEngine::appendTrailTrims() {
    Target t;
    t.yKm = 1.0;
    for (int i = 0; i < 10; ++i) {
        SimEngine::appendTrailPoint(t, 5);
        t.yKm += 0.1;
    }
    // 追加 10 点后裁剪到 5 点：保留最近 5 个 → [1.5 .. 1.9]
    QCOMPARE(t.trail.size(), 5);
    QVERIFY(qAbs(t.trail.first().rangeKm - 1.5) < 1e-9);
    QVERIFY(qAbs(t.trail.last().rangeKm - 1.9) < 1e-9);
}

QTEST_GUILESS_MAIN(TestSimEngine)
#include "test_simengine.moc"