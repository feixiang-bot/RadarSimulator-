#pragma once
#include <QByteArray>
#include <QString>
#include <QVector>

// UDP 航迹报文协议：纯逻辑编解码，不依赖 socket / 事件循环，可单测。
// 报文（QDataStream · BigEndian · Qt_6_0 版本）：
//   [magic "RADT" 4B][version u8][msgType u8][count u16]
//   + count × record{ id u32 | azimuthDeg f64 | rangeKm f64 | speedKmh f64
//                     | headingDeg f64 | flags u8(bit0=autoMove) | name QString }
namespace UdpProtocol {

struct UdpTrack {
    int    id         = 0;
    double azimuthDeg = 0.0;
    double rangeKm    = 0.0;
    double speedKmh   = 0.0;
    double headingDeg = 0.0;
    bool   autoMove   = false;
    QString name;
};

constexpr quint8 kVersion = 1;
constexpr quint8 kMsgTypeTrack = 1;
constexpr int kMaxRecordsPerDatagram = 1024;

QByteArray encodeTrackMessage(const QVector<UdpTrack>& tracks);
bool decodeTrackMessage(const QByteArray& datagram, QVector<UdpTrack>* out,
                        QString* errorMsg = nullptr);

} // namespace UdpProtocol

// 全局别名：让 UdpTrackSource / UdpTrackSender / 上层代码直接用 UdpTrack
using UdpTrack = UdpProtocol::UdpTrack;