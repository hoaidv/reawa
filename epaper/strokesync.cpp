#include "strokesync.h"

#include <QTimer>
#include <QDebug>

StrokeSync::StrokeSync(QObject *parent)
    : QObject(parent)
{
    m_enabled = qEnvironmentVariableIsSet("RM_SYNC_HOST");
    if (!m_enabled)
        return;

    connect(&m_socket, &QTcpSocket::readyRead, this, [this]() {
        m_buffer.append(m_socket.readAll());
    });
    connect(&m_socket, &QTcpSocket::connected, this, [this]() { flushQueue(); });

    auto *retry = new QTimer(this);
    retry->setInterval(2000);
    connect(retry, &QTimer::timeout, this, [this]() {
        if (m_socket.state() == QAbstractSocket::UnconnectedState)
            connectToMac();
    });
    retry->start();
}

void StrokeSync::connectToMac()
{
    if (!m_enabled)
        return;
    if (m_socket.state() == QAbstractSocket::ConnectedState
        || m_socket.state() == QAbstractSocket::ConnectingState)
        return;

    const QByteArray host = qgetenv("RM_SYNC_HOST");
    if (host.isEmpty())
        return;
    m_socket.connectToHost(QString::fromUtf8(host), 9877);
}

void StrokeSync::sendLine(const QByteArray &jsonLine)
{
    if (!m_enabled)
        return;

    QByteArray line = jsonLine;
    if (!line.endsWith('\n'))
        line.append('\n');

    if (m_queue.size() >= kMaxQueue)
        m_queue.remove(0, m_queue.size() - kMaxQueue + 1);
    m_queue.append(line);
    scheduleFlush();
}

void StrokeSync::scheduleFlush()
{
    if (m_flushScheduled)
        return;
    m_flushScheduled = true;
    // Defer socket work off the pen hot path onto the next event-loop turn.
    QTimer::singleShot(0, this, [this]() {
        m_flushScheduled = false;
        if (m_socket.state() != QAbstractSocket::ConnectedState) {
            connectToMac();
            return;
        }
        flushQueue();
    });
}

void StrokeSync::flushQueue()
{
    if (!m_enabled)
        return;
    if (m_socket.state() != QAbstractSocket::ConnectedState)
        return;

    for (const QByteArray &line : std::as_const(m_queue))
        m_socket.write(line);
    m_queue.clear();
}
