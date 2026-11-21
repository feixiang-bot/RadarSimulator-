#pragma once
#include <QObject>
#include <QHostAddress>
#include <QVector>
#include <QtGlobal>
#include "core/udpprotocol.h"

class QUdpSocket;

// UDP 航迹发送（回放 / 外接数据源）：编码为报文并 writeDatagram 到目标地址。
class UdpTrackSender : public QObject {
    Q_OBJECT
public:
    explicit UdpTrackSender(QObject* parent = nullptr);

    void setDestination(const QHostAddress& addr, quint16 port);
    quint16 destinationPort() const { return m_port; }

    bool send(const QVector<UdpTrack>& tracks, QString* errorMsg = nullptr) const;

private:
    QUdpSocket* m_socket = nullptr;
    QHostAddress m_addr;
    quint16 m_port = 0;
};