#pragma once
#include <QObject>
#include <QVector>
#include <QtGlobal>
#include "core/udpprotocol.h"

class QUdpSocket;

// UDP 航迹接收：QUdpSocket 监听 + 解码，通过信号把解析结果交给上层。
// 只负责收与解，不做任何 UI / 数据层写操作，便于复用与测试。
class UdpTrackSource : public QObject {
    Q_OBJECT
public:
    explicit UdpTrackSource(QObject* parent = nullptr);

    // 绑定本地端口开始接收；失败返回 false 并填 errorMsg。
    bool listen(quint16 port, QString* errorMsg = nullptr);
    void close();
    bool isListening() const;
    quint16 port() const { return m_port; }

signals:
    void tracksReceived(const QVector<UdpTrack>& tracks);
    void errorOccurred(const QString& message);

private slots:
    void onReadyRead();

private:
    QUdpSocket* m_socket = nullptr;
    quint16     m_port = 0;
};