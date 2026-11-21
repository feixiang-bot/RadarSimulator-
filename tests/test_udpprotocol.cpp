#include <QtTest>
#include <QDataStream>
#include "core/udpprotocol.h"

class TestUdpProtocol : public QObject {
    Q_OBJECT
private slots:
    void roundTripSingle();
    void roundTripMultiple();
    void encodeEmpty();
    void decodeRejectsEmpty();
    void decodeRejectsBadMagic();
    void decodeRejectsBadVersion();
};

void TestUdpProtocol::roundTripSingle() {
    QVector<UdpTrack> in;
    UdpTrack t;
    t.id         = 7;
    t.azimuthDeg = 123.5;
    t.rangeKm    = 8.25;
    t.speedKmh   = 60.0;
    t.headingDeg = 90.0;
    t.autoMove   = true;
    t.name       = "TGT-07";
    in.append(t);

    const QByteArray datagram = UdpProtocol::encodeTrackMessage(in);
    QVERIFY(!datagram.isEmpty());

    QVector<UdpTrack> out;
    QString err;
    QVERIFY2(UdpProtocol::decodeTrackMessage(datagram, &out, &err), qPrintable(err));

    QCOMPARE(out.size(), 1);
    QCOMPARE(out[0].id, 7);
    QCOMPARE(out[0].name, QString("TGT-07"));
    QCOMPARE(out[0].autoMove, true);
    QVERIFY(qAbs(out[0].azimuthDeg - 123.5) < 1e-9);
    QVERIFY(qAbs(out[0].rangeKm - 8.25) < 1e-9);
    QVERIFY(qAbs(out[0].speedKmh - 60.0) < 1e-9);
    QVERIFY(qAbs(out[0].headingDeg - 90.0) < 1e-9);
}

void TestUdpProtocol::roundTripMultiple() {
    QVector<UdpTrack> in;
    for (int i = 0; i < 3; ++i) {
        UdpTrack t;
        t.id         = 100 + i;
        t.azimuthDeg = i * 30.0;
        t.rangeKm    = 1.0 + i;
        t.autoMove   = (i % 2 == 0);
        t.name       = QString("N-%1").arg(i);
        in.append(t);
    }

    QVector<UdpTrack> out;
    QString err;
    QVERIFY2(UdpProtocol::decodeTrackMessage(UdpProtocol::encodeTrackMessage(in), &out, &err),
             qPrintable(err));

    QCOMPARE(out.size(), 3);
    for (int i = 0; i < 3; ++i) {
        QCOMPARE(out[i].id, 100 + i);
        QCOMPARE(out[i].name, QString("N-%1").arg(i));
        QVERIFY(qAbs(out[i].azimuthDeg - i * 30.0) < 1e-9);
    }
}

void TestUdpProtocol::encodeEmpty() {
    const QByteArray datagram = UdpProtocol::encodeTrackMessage({});
    QVector<UdpTrack> out;
    QString err;
    QVERIFY2(UdpProtocol::decodeTrackMessage(datagram, &out, &err), qPrintable(err));
    QVERIFY(out.isEmpty());
}

void TestUdpProtocol::decodeRejectsEmpty() {
    QVector<UdpTrack> out;
    QString err;
    QVERIFY(!UdpProtocol::decodeTrackMessage(QByteArray(), &out, &err));
    QVERIFY(!err.isEmpty());
    QVERIFY(out.isEmpty());
}

void TestUdpProtocol::decodeRejectsBadMagic() {
    QByteArray datagram = UdpProtocol::encodeTrackMessage({});
    QVERIFY(datagram.size() >= 4);
    datagram[0] = 'X';   // 破坏 magic 首字节

    QVector<UdpTrack> out;
    QString err;
    QVERIFY(!UdpProtocol::decodeTrackMessage(datagram, &out, &err));
    QVERIFY(!err.isEmpty());
    QVERIFY(out.isEmpty());
}

void TestUdpProtocol::decodeRejectsBadVersion() {
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds.setVersion(QDataStream::Qt_6_0);
    ds.writeRawData("RADT", 4);
    ds << quint8(99) << quint8(UdpProtocol::kMsgTypeTrack) << quint16(0);

    QVector<UdpTrack> out;
    QString err;
    QVERIFY(!UdpProtocol::decodeTrackMessage(buf, &out, &err));
    QVERIFY(!err.isEmpty());
    QVERIFY(out.isEmpty());
}

QTEST_GUILESS_MAIN(TestUdpProtocol)
#include "test_udpprotocol.moc"