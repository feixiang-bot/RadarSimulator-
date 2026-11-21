#include "core/udptracksender.h"
#include <QUdpSocket>

UdpTrackSender::UdpTrackSender(QObject* parent)
    : QObject(parent), m_socket(new QUdpSocket(this)) {}

void UdpTrackSender::setDestination(const QHostAddress& addr, quint16 port) {
    m_addr = addr;
    m_port = port;
}

bool UdpTrackSender::send(const QVector<UdpTrack>& tracks, QString* errorMsg) const {
    if (m_port == 0) {
        if (errorMsg) *errorMsg = QString("目标端口未设置");
        return false;
    }
    const QByteArray datagram = UdpProtocol::encodeTrackMessage(tracks);
    const qint64 n = m_socket->writeDatagram(datagram, m_addr, m_port);
    if (n != datagram.size()) {
        if (errorMsg) *errorMsg = m_socket->errorString();
        return false;
    }
    return true;
}