#include "strokesync.h"

#include <QTimer>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkProxy>

StrokeSync::StrokeSync(QObject *parent)
    : QObject(parent)
{
    m_enabled = qEnvironmentVariableIsSet("RM_SYNC_HOST");
    if (!m_enabled)
        return;

    m_socket.setProxy(QNetworkProxy::NoProxy);
    connect(&m_socket, &QTcpSocket::readyRead, this, &StrokeSync::onReadyRead);
    connect(&m_socket, &QTcpSocket::connected, this, [this]() {
        qInfo() << "[sync] connected to" << m_socket.peerAddress().toString() << m_socket.peerPort();
        emit socketConnected();
        flushQueue();
    });
    connect(&m_socket, &QTcpSocket::disconnected, this, [this]() {
        qInfo() << "[sync] disconnected — will retry";
        m_inbound.clear();
        emit socketDisconnected();
    });
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(&m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
#else
    connect(&m_socket,
            static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
            this, [this](QAbstractSocket::SocketError) {
#endif
        // Abort so we never stick in ConnectingState / ClosingState across retries.
        qWarning() << "[sync] socket error" << m_socket.errorString();
        m_socket.abort();
        m_inbound.clear();
        emit socketDisconnected();
    });

    auto *retry = new QTimer(this);
    retry->setInterval(2000);
    connect(retry, &QTimer::timeout, this, [this]() {
        if (m_socket.state() != QAbstractSocket::ConnectedState)
            connectToMac();
    });
    retry->start();
}

void StrokeSync::connectToMac()
{
    if (!m_enabled)
        return;
    if (m_socket.state() == QAbstractSocket::ConnectedState)
        return;

    // Reset stuck Connecting / HostLookup / Closing before a fresh attempt.
    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        m_socket.abort();

    const QByteArray host = qgetenv("RM_SYNC_HOST");
    if (host.isEmpty())
        return;
    // Mac USB IP (e.g. 10.11.99.12) — never the tablet's own 10.11.99.1.
    qInfo() << "[sync] connecting to" << QString::fromUtf8(host) << "9877";
    m_socket.connectToHost(QString::fromUtf8(host), 9877);
}

bool StrokeSync::isConnected() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
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

void StrokeSync::onReadyRead()
{
    m_inbound.append(m_socket.readAll());
    int nl = 0;
    while ((nl = m_inbound.indexOf('\n')) >= 0) {
        const QByteArray line = m_inbound.left(nl).trimmed();
        m_inbound.remove(0, nl + 1);
        if (!line.isEmpty())
            handleInboundLine(line);
    }
}

void StrokeSync::handleInboundLine(const QByteArray &line)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(line, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[sync] bad host line" << line.left(80);
        return;
    }
    emit hostMessage(doc.object());
}
