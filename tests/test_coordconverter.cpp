#include <QtTest>
#include "core/coordconverter.h"

class TestCoordConverter : public QObject {
    Q_OBJECT
private slots:
    void polarToScreenCardinals();
    void polarToScreenRangeMapping();
    void screenToPolarCardinals();
    void polarScreenRoundTrip();
};

void TestCoordConverter::polarToScreenCardinals() {
    CoordConverter c;
    c.setGeometry(QPointF(200, 150), 100.0, 50.0);

    // 圆心（0 km）→ 屏幕中心
    QPointF p = c.polarToScreen(0.0, 0.0);
    QVERIFY(qAbs(p.x() - 200.0) < 1e-9);
    QVERIFY(qAbs(p.y() - 150.0) < 1e-9);

    // 0°（正北）→ 正上方
    p = c.polarToScreen(50.0, 0.0);
    QVERIFY(qAbs(p.x() - 200.0) < 1e-9);
    QVERIFY(qAbs(p.y() - 50.0) < 1e-9);

    // 90°（正东，顺时针）→ 正右方
    p = c.polarToScreen(50.0, 90.0);
    QVERIFY(qAbs(p.x() - 300.0) < 1e-9);
    QVERIFY(qAbs(p.y() - 150.0) < 1e-9);

    // 180°（正南）→ 正下方
    p = c.polarToScreen(50.0, 180.0);
    QVERIFY(qAbs(p.x() - 200.0) < 1e-9);
    QVERIFY(qAbs(p.y() - 250.0) < 1e-9);

    // 270°（正西）→ 正左方
    p = c.polarToScreen(50.0, 270.0);
    QVERIFY(qAbs(p.x() - 100.0) < 1e-9);
    QVERIFY(qAbs(p.y() - 150.0) < 1e-9);
}

void TestCoordConverter::polarToScreenRangeMapping() {
    CoordConverter c;
    c.setGeometry(QPointF(200, 150), 100.0, 50.0);

    // 一半量程（25 km）→ 一半半径（50 px）
    const QPointF p = c.polarToScreen(25.0, 90.0);
    QVERIFY(qAbs(p.x() - 250.0) < 1e-9);
    QVERIFY(qAbs(p.y() - 150.0) < 1e-9);
}

void TestCoordConverter::screenToPolarCardinals() {
    CoordConverter c;
    c.setGeometry(QPointF(200, 150), 100.0, 50.0);

    double r = 0.0, a = 0.0;

    c.screenToPolar(QPointF(200, 150), r, a);  // 圆心：距离为 0，方位无定义
    QVERIFY(qAbs(r - 0.0) < 1e-9);
    QVERIFY(a >= 0.0 && a < 360.0);

    c.screenToPolar(QPointF(200, 50), r, a);   // 正北
    QVERIFY(qAbs(r - 50.0) < 1e-6);
    QVERIFY(qAbs(a - 0.0) < 1e-6);

    c.screenToPolar(QPointF(300, 150), r, a);  // 正东
    QVERIFY(qAbs(r - 50.0) < 1e-6);
    QVERIFY(qAbs(a - 90.0) < 1e-6);

    c.screenToPolar(QPointF(100, 150), r, a);  // 正西
    QVERIFY(qAbs(r - 50.0) < 1e-6);
    QVERIFY(qAbs(a - 270.0) < 1e-6);
}

void TestCoordConverter::polarScreenRoundTrip() {
    CoordConverter c;
    c.setGeometry(QPointF(320, 240), 200.0, 20.0);

    for (double ang = 0.0; ang < 360.0; ang += 15.0) {
        const QPointF s = c.polarToScreen(12.0, ang);
        double r = 0.0, a = 0.0;
        c.screenToPolar(s, r, a);

        double da = qAbs(a - ang);
        if (da > 180.0) da = 360.0 - da;  // 环形角度差
        QVERIFY2(qAbs(r - 12.0) < 1e-6 && da < 1e-6,
                 qPrintable(QString("roundtrip 失败 @ %1° -> (r=%2, a=%3)")
                                .arg(ang).arg(r).arg(a)));
    }
}

QTEST_GUILESS_MAIN(TestCoordConverter)
#include "test_coordconverter.moc"