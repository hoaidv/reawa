#include "strokesync.h"
#include "net_retry.hpp"
#include "usb_link.hpp"
#include "usb_path.hpp"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkProxy>
#include <QTimer>

#if defined(Q_OS_LINUX)
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

namespace {

void applyTcpKeepaliveProbes(QTcpSocket &sock)
{
    sock.setSocketOption(QAbstractSocket::KeepAliveOption, 1);
#if defined(Q_OS_LINUX)
    const qintptr fd = sock.socketDescriptor();
    if (fd < 0)
        return;
    int idle = 5;
    int intvl = 2;
    int cnt = 4;
    const int sfd = static_cast<int>(fd);
    ::setsockopt(sfd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    ::setsockopt(sfd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
    ::setsockopt(sfd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));
#endif
}

} // namespace

StrokeSync::StrokeSync(QObject *parent)
    : QObject(parent)
{
    m_enabled = qEnvironmentVariableIsSet("RM_SYNC_HOST");
    if (!m_enabled)
        return;

    m_socket.setProxy(QNetworkProxy::NoProxy);
    m_socket.setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    connect(&m_socket, &QTcpSocket::readyRead, this, &StrokeSync::onReadyRead);
    connect(&m_socket, &QTcpSocket::connected, this, [this]() {
        qInfo() << "[sync] connected to" << m_socket.peerAddress().toString() << m_socket.peerPort();
        m_wasConnected = true;
        m_retriesLeft = epaper::kTcpRetryLimit;
        applyTcpKeepaliveProbes(m_socket);
        emit socketConnected();
        flushQueue();
    });
    connect(&m_socket, &QTcpSocket::disconnected, this, [this]() {
        qInfo() << "[sync] disconnected";
        if (m_wasConnected)
            m_retriesLeft = epaper::kTcpRetryLimit;
        m_wasConnected = false;
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
        qWarning() << "[sync] socket error" << m_socket.errorString();
        m_socket.abort();
        m_inbound.clear();
        emit socketDisconnected();
    });

    auto *retry = new QTimer(this);
    retry->setInterval(epaper::kAppTcpRetryMs);
    connect(retry, &QTimer::timeout, this, [this]() {
        if (!epaper::usbEthernetLive()) {
            if (m_socket.state() != QAbstractSocket::UnconnectedState)
                m_socket.abort();
            return;
        }
        if (m_socket.state() == QAbstractSocket::ConnectedState)
            return;
        if (m_socket.state() == QAbstractSocket::ConnectingState
            || m_socket.state() == QAbstractSocket::HostLookupState)
            return;
        if (m_retriesLeft <= 0)
            return;
        --m_retriesLeft;
        qInfo() << "[sync] retry left" << m_retriesLeft;
        connectToMac();
    });
    retry->start();

    m_retriesLeft = epaper::kTcpRetryLimit;
    if (auto *usb = epaper::UsbLink::instance()) {
        connect(usb, &epaper::UsbLink::requestAppReconnect, this, &StrokeSync::armReconnect);
        connect(this, &StrokeSync::socketConnected, usb, &epaper::UsbLink::refreshHud);
        connect(this, &StrokeSync::socketDisconnected, usb, &epaper::UsbLink::refreshHud);
    }
    QTimer::singleShot(0, this, [this]() {
        if (!epaper::usbEthernetLive() || m_socket.state() == QAbstractSocket::ConnectedState)
            return;
        if (m_retriesLeft <= 0)
            return;
        --m_retriesLeft;
        connectToMac();
    });
}

void StrokeSync::armReconnect()
{
    m_retriesLeft = epaper::kTcpRetryLimit;
    if (!epaper::usbEthernetLive()) {
        if (m_socket.state() != QAbstractSocket::UnconnectedState)
            m_socket.abort();
        return;
    }
    if (m_socket.state() == QAbstractSocket::ConnectedState)
        return;
    --m_retriesLeft;
    connectToMac();
}

void StrokeSync::connectToMac()
{
    if (!m_enabled)
        return;
    if (!epaper::usbEthernetLive()) {
        if (m_socket.state() != QAbstractSocket::UnconnectedState)
            m_socket.abort();
        return;
    }
    if (m_socket.state() == QAbstractSocket::ConnectedState)
        return;
    if (m_socket.state() == QAbstractSocket::ConnectingState
        || m_socket.state() == QAbstractSocket::HostLookupState)
        return;

    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        m_socket.abort();

    const QByteArray host = qgetenv("RM_SYNC_HOST");
    if (host.isEmpty())
        return;
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
        if (!epaper::usbEthernetLive()) {
            if (m_socket.state() != QAbstractSocket::UnconnectedState)
                m_socket.abort();
            return;
        }
        if (m_socket.state() != QAbstractSocket::ConnectedState) {
            if (m_retriesLeft > 0)
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
