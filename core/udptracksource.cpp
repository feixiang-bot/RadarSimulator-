#include "core/udptracksource.h"
#include <QUdpSocket>
#include <QHostAddress>

UdpTrackSource::UdpTrackSource(QObject* parent)
    : QObject(parent), m_socket(new QUdpSocket(this)) {
    connect(m_socket, &QUdpSocket::readyRead, this, &UdpTrackSource::onReadyRead);
}

bool UdpTrackSource::listen(quint16 port, QString* errorMsg) {
    if (m_socket->bind(QHostAddress::Any, port,
                       QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        m_port = port;
        return true;
    }
    if (errorMsg) *errorMsg = m_socket->errorString();
    return false;
}

void UdpTrackSource::close() {
    m_socket->close();
    m_port = 0;
}

bool UdpTrackSource::isListening() const {
    return m_socket->state() == QAbstractSocket::BoundState;
}

void UdpTrackSource::onReadyRead() {
    while (m_socket->hasPendingDatagrams()) {
        const qint64 size = m_socket->pendingDatagramSize();
        if (size <= 0) continue;

        QByteArray datagram(int(size), Qt::Uninitialized);
        QHostAddress sender;
        quint16 senderPort = 0;
        const qint64 n = m_socket->readDatagram(datagram.data(), datagram.size(),
                                                &sender, &senderPort);
        if (n < 0) continue;   // 读取失败，跳过

        QVector<UdpTrack> tracks;
        QString err;
        if (UdpProtocol::decodeTrackMessage(datagram, &tracks, &err)) {
            emit tracksReceived(tracks);
        } else {
            emit errorOccurred(
                QString("忽略来自 %1:%2 的无效报文：%3")
                    .arg(sender.toString()).arg(senderPort).arg(err));
        }
    }
}