#include <QtTest>
#include "core/Target.h"

class TestTarget : public QObject {
    Q_OBJECT
private slots:
    void rangeKm();
    void azimuthCardinals();
    void azimuthWrap();
};

void TestTarget::rangeKm() {
    Target t;
    t.xKm = 3.0;
    t.yKm = 4.0;
    QVERIFY(qAbs(t.rangeKm() - 5.0) < 1e-9);
}

void TestTarget::azimuthCardinals() {
    // 数据层直角坐标约定：x 正东，y 正北；方位角正北 0°，顺时针。
    Target n;  // 正北 → 0°
    n.xKm = 0.0; n.yKm = 1.0;
    QVERIFY(qAbs(n.azimuthDeg() - 0.0) < 1e-9);

    Target e;  // 正东 → 90°
    e.xKm = 1.0; e.yKm = 0.0;
    QVERIFY(qAbs(e.azimuthDeg() - 90.0) < 1e-9);

    Target s;  // 正南 → 180°
    s.xKm = 0.0; s.yKm = -1.0;
    QVERIFY(qAbs(s.azimuthDeg() - 180.0) < 1e-9);

    Target w;  // 正西 → 270°
    w.xKm = -1.0; w.yKm = 0.0;
    QVERIFY(qAbs(w.azimuthDeg() - 270.0) < 1e-9);

    Target ne; // 东北 45°
    ne.xKm = 1.0; ne.yKm = 1.0;
    QVERIFY(qAbs(ne.azimuthDeg() - 45.0) < 1e-9);
}

void TestTarget::azimuthWrap() {
    // 西南象限：atan2 原始值为负，应归一化到 [0,360) → 225°
    Target sw;
    sw.xKm = -1.0; sw.yKm = -1.0;
    QVERIFY(qAbs(sw.azimuthDeg() - 225.0) < 1e-9);
}

QTEST_GUILESS_MAIN(TestTarget)
#include "test_target.moc"