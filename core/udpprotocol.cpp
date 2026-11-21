#include "core/udpprotocol.h"
#include <QDataStream>
#include <QIODevice>
#include <cstring>

namespace {
const char kMagicBytes[4] = {'R', 'A', 'D', 'T'};
}

namespace UdpProtocol {

QByteArray encodeTrackMessage(const QVector<UdpTrack>& tracks) {
    QByteArray buf;
    QDataStream ds(&buf, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds.setVersion(QDataStream::Qt_6_0);

    ds.writeRawData(kMagicBytes, 4);
    ds << quint8(kVersion) << quint8(kMsgTypeTrack) << quint16(tracks.size());

    for (const UdpTrack& t : tracks) {
        ds << quint32(t.id)
           << double(t.azimuthDeg)
           << double(t.rangeKm)
           << double(t.speedKmh)
           << double(t.headingDeg)
           << quint8(t.autoMove ? 1 : 0)
           << t.name;
    }
    return buf;
}

bool decodeTrackMessage(const QByteArray& datagram, QVector<UdpTrack>* out,
                        QString* errorMsg) {
    if (!out) return false;
    out->clear();

    QDataStream ds(datagram);
    ds.setByteOrder(QDataStream::BigEndian);
    ds.setVersion(QDataStream::Qt_6_0);

    auto fail = [&](const QString& msg) {
        if (errorMsg) *errorMsg = msg;
        out->clear();
        return false;
    };

    char magic[4] = {0, 0, 0, 0};
    if (ds.readRawData(magic, 4) != 4)
        return fail("报文过短：缺少 magic 头");
    if (std::memcmp(magic, kMagicBytes, 4) != 0)
        return fail("magic 不匹配：不是 RADT 航迹报文");

    quint8 version = 0;
    ds >> version;
    if (ds.status() != QDataStream::Ok)
        return fail("报文不完整：缺少 version 字段");
    if (version != kVersion)
        return fail(QString("不支持的协议版本：%1").arg(version));

    quint8 msgType = 0;
    ds >> msgType;
    if (ds.status() != QDataStream::Ok)
        return fail("报文不完整：缺少 msgType 字段");
    if (msgType != kMsgTypeTrack)
        return fail(QString("未知报文类型：%1").arg(msgType));

    quint16 count = 0;
    ds >> count;
    if (ds.status() != QDataStream::Ok)
        return fail("报文不完整：缺少记录数字段");
    if (count > kMaxRecordsPerDatagram)
        return fail(QString("记录数超限：%1").arg(count));

    out->reserve(int(count));
    for (quint16 i = 0; i < count; ++i) {
        quint32 idu = 0;
        double az = 0.0, r = 0.0, s = 0.0, h = 0.0;
        quint8 flags = 0;
        QString name;
        ds >> idu >> az >> r >> s >> h >> flags >> name;
        if (ds.status() != QDataStream::Ok)
            return fail("报文不完整：记录数据被截断");

        UdpTrack t;
        t.id         = int(idu);
        t.azimuthDeg = az;
        t.rangeKm    = r;
        t.speedKmh   = s;
        t.headingDeg = h;
        t.autoMove   = (flags & 1) != 0;
        t.name       = name;
        out->append(t);
    }
    return true;
}

} // namespace UdpProtocol