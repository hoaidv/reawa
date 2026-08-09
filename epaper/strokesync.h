#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QVector>

/**
 * Optional RM→macOS stroke sync. Inert unless RM_SYNC_HOST is set.
 * @implements [SRS-EP-01]
 */
class StrokeSync : public QObject
{
    Q_OBJECT

public:
    explicit StrokeSync(QObject *parent = nullptr);

    void connectToMac();
    void sendLine(const QByteArray &jsonLine);
    bool enabled() const { return m_enabled; }

private:
    void flushQueue();
    void scheduleFlush();

    bool m_enabled = false;
    QTcpSocket m_socket;
    QByteArray m_buffer;
    QVector<QByteArray> m_queue;
    bool m_flushScheduled = false;
    static constexpr int kMaxQueue = 256;
};
