#include "strokesync.h"

#include <QTimer>

StrokeSync::StrokeSync(QObject *parent)
    : QObject(parent)
{
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
    if (m_socket.state() == QAbstractSocket::ConnectedState
        || m_socket.state() == QAbstractSocket::ConnectingState)
        return;

    const QByteArray host = qgetenv("RM_SYNC_HOST");
    const QString macHost = host.isEmpty() ? QStringLiteral("10.11.99.2") : QString::fromUtf8(host);
    m_socket.connectToHost(macHost, 9877);
}

void StrokeSync::sendLine(const QByteArray &jsonLine)
{
    QByteArray line = jsonLine;
    if (!line.endsWith('\n'))
        line.append('\n');

    if (m_socket.state() != QAbstractSocket::ConnectedState) {
        m_queue.append(line);
        connectToMac();
        return;
    }

    m_socket.write(line);
}

void StrokeSync::flushQueue()
{
    if (m_socket.state() != QAbstractSocket::ConnectedState)
        return;

    for (const QByteArray &line : std::as_const(m_queue))
        m_socket.write(line);
    m_queue.clear();
}
