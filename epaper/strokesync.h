#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QVector>
#include <QJsonObject>

/**
 * App: StrokeSync TCP to Infini :9877. USB stay-up is UsbLink (infra).
 * @implements [SRS-EP-01]
 * @implements [SRS-EP-02] viewport + region refresh receive
 * @implements [SRS-EP-08] one-way sync TCP session
 */
class StrokeSync : public QObject
{
    Q_OBJECT

public:
    explicit StrokeSync(QObject *parent = nullptr);

    void connectToMac();
    void sendLine(const QByteArray &jsonLine);
    bool enabled() const { return m_enabled; }
    bool isConnected() const;

signals:
    void hostMessage(const QJsonObject &obj);
    void socketConnected();
    void socketDisconnected();

private:
    void flushQueue();
    void scheduleFlush();
    void onReadyRead();
    void handleInboundLine(const QByteArray &line);

    bool m_enabled = false;
    QTcpSocket m_socket;
    QByteArray m_inbound;
    QVector<QByteArray> m_queue;
    bool m_flushScheduled = false;
    static constexpr int kMaxQueue = 256;
};
