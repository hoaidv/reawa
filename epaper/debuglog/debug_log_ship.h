#pragma once
/**
 * Sidecar TCP :9878 debug-log ship. Worker thread owns the socket.
 * @implements [SRS-EP-15] debug-log ship
 * @implements [SRS-EP-16] 0 I/O on paint / ingestPoint
 */

#include <QObject>
#include <QThread>

#include "debug_log_queue.hpp"

class DebugLogWorker;

class DebugLogShip : public QObject
{
    Q_OBJECT

public:
    explicit DebugLogShip(QObject *parent = nullptr);
    ~DebugLogShip() override;

    void startIfEnabled();
    bool enabled() const { return m_enabled; }

    void armReconnect();

    /** Qt message handler — enqueue or drop; never writes the socket. */
    static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);

    void tryEnqueue(const char *logger, const char *level, const QString &msg);

private:
    bool m_enabled = false;
    QThread m_thread;
    DebugLogWorker *m_worker = nullptr;
    epaper::debuglog::DebugLogQueue m_queue;
};
