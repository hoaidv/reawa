#pragma once
/**
 * Sidecar TCP :9878 debug-log ship. Worker thread owns the socket.
 * @implements [SRS-EP-15] debug-log ship
 * @implements [SRS-EP-16] 0 I/O on paint / ingestPoint
 */

#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>

#include "debug_log_queue.hpp"

class DebugLogWorker;

class DebugLogShip : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList lines READ lines NOTIFY linesChanged)
    Q_PROPERTY(QString text READ text NOTIFY linesChanged)

public:
    static DebugLogShip *instance();

    explicit DebugLogShip(QObject *parent = nullptr);
    ~DebugLogShip() override;

    void startIfEnabled();
    bool enabled() const { return m_enabled; }

    void armReconnect();

    QStringList lines() const { return m_lines; }
    QString text() const { return m_lines.join(QLatin1Char('\n')); }

    /** Qt message handler — enqueue or drop; never writes the socket. */
    static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);

    void tryEnqueue(const char *logger, const char *level, const QString &msg);

public slots:
    void appendUiLine(const QString &line);

signals:
    void linesChanged();

private:
    static DebugLogShip *s_instance;
    bool m_enabled = false;
    QThread m_thread;
    DebugLogWorker *m_worker = nullptr;
    epaper::debuglog::DebugLogQueue m_queue;
    QStringList m_lines;
    static constexpr int kUiCap = 256;
};
