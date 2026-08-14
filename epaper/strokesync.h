#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QVector>
#include <QJsonObject>

/**
 * Bidirectional RM↔macOS JSON-lines sync. Inert unless RM_SYNC_HOST is set.
 * Outbound: stroke_*; Inbound: viewport, region_refresh.
 * @implements [SRS-EP-01]
 * @implements [SRS-EP-02] viewport + region refresh receive
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
    /** Host → device message (viewport / region_refresh). */
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
    QByteArray m_buffer;
    QByteArray m_inbound;
    QVector<QByteArray> m_queue;
    bool m_flushScheduled = false;
    static constexpr int kMaxQueue = 256;
};
