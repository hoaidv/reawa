/**
 * @implements [SRS-EP-15] debug-log ship worker + Qt handler + stdio capture
 * @implements [SRS-EP-16] socket I/O only on the worker thread
 */

#include "debug_log_ship.h"
#include "debug_log_format.hpp"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkProxy>
#include <QSocketNotifier>
#include <QTcpSocket>
#include <QTimer>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

namespace {

DebugLogShip *g_ship = nullptr;
int g_savedStdout = -1;
int g_savedStderr = -1;

void echoLocal(int fd, const QByteArray &line)
{
    if (fd < 0)
        fd = 2;
    const char nl = '\n';
    ::write(fd, line.constData(), size_t(line.size()));
    if (line.isEmpty() || line.back() != '\n')
        ::write(fd, &nl, 1);
}

QByteArray recordJson(const epaper::debuglog::DebugLogRecord &rec, int dropped)
{
    QJsonObject o;
    o.insert(QStringLiteral("type"), QStringLiteral("debug_log"));
    o.insert(QStringLiteral("ts"), static_cast<double>(rec.ts));
    o.insert(QStringLiteral("level"), QString::fromStdString(rec.level));
    o.insert(QStringLiteral("logger"), QString::fromStdString(rec.logger));
    o.insert(QStringLiteral("msg"), QString::fromStdString(rec.msg));
    o.insert(QStringLiteral("dropped"), dropped);
    return QJsonDocument(o).toJson(QJsonDocument::Compact) + '\n';
}

} // namespace

class DebugLogWorker : public QObject
{
    Q_OBJECT

public:
    DebugLogWorker(epaper::debuglog::DebugLogQueue *queue, int stdoutFd, int stderrFd,
                   bool stdioOk, QObject *parent = nullptr)
        : QObject(parent)
        , m_queue(queue)
        , m_stdoutFd(stdoutFd)
        , m_stderrFd(stderrFd)
        , m_stdioOk(stdioOk)
    {
    }

public slots:
    void start()
    {
        // Create all QObjects on this worker thread (never parent to qApp).
        m_socket = new QTcpSocket(this);
        m_socket->setProxy(QNetworkProxy::NoProxy);
        connect(m_socket, &QTcpSocket::readyRead, this, &DebugLogWorker::onReadyRead);
        connect(m_socket, &QTcpSocket::connected, this, &DebugLogWorker::onConnected);
        connect(m_socket, &QTcpSocket::disconnected, this, &DebugLogWorker::onDisconnected);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        connect(m_socket, &QTcpSocket::errorOccurred, this, &DebugLogWorker::onSocketError);
#else
        connect(m_socket,
                static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
                this, &DebugLogWorker::onSocketError);
#endif

        auto *retry = new QTimer(this);
        retry->setInterval(2000);
        connect(retry, &QTimer::timeout, this, &DebugLogWorker::tryConnect);
        retry->start();

        auto *drain = new QTimer(this);
        drain->setInterval(50);
        connect(drain, &QTimer::timeout, this, &DebugLogWorker::drainQueue);
        drain->start();

        if (m_stdoutFd >= 0) {
            auto *n = new QSocketNotifier(m_stdoutFd, QSocketNotifier::Read, this);
            connect(n, &QSocketNotifier::activated, this, [this]() { onStdout(); });
        }
        if (m_stderrFd >= 0) {
            auto *n = new QSocketNotifier(m_stderrFd, QSocketNotifier::Read, this);
            connect(n, &QSocketNotifier::activated, this, [this]() { onStderr(); });
        }

        if (!m_stdioOk) {
            epaper::debuglog::DebugLogRecord rec;
            rec.ts = QDateTime::currentMSecsSinceEpoch();
            rec.level = "warning";
            rec.logger = "qt";
            rec.msg = "[debug] stdio capture unavailable";
            m_queue->tryPush(std::move(rec));
        }

        tryConnect();
    }

private slots:
    void tryConnect()
    {
        if (!m_socket)
            return;
        if (m_socket->state() == QAbstractSocket::ConnectedState)
            return;
        // Abort stuck Connecting / HostLookup / Closing so the 2s timer always gets a fresh attempt.
        if (m_socket->state() != QAbstractSocket::UnconnectedState)
            m_socket->abort();

        const QByteArray host = qgetenv("RM_SYNC_HOST");
        if (host.isEmpty())
            return;
        const int port = epaper::debuglog::debugLogPort(
            qgetenv("EPAPER_DEBUG_PORT").constData(),
            qgetenv("INFINI_DEBUG_PORT").constData());
        epaper::debuglog::DebugLogRecord rec;
        rec.ts = QDateTime::currentMSecsSinceEpoch();
        rec.level = "info";
        rec.logger = "qt";
        rec.msg = QStringLiteral("[debug] connecting to %1:%2")
                      .arg(QString::fromUtf8(host))
                      .arg(port)
                      .toStdString();
        m_queue->tryPush(std::move(rec));
        // Also echo locally (handler may not be shipping yet).
        echoLocal(g_savedStderr >= 0 ? g_savedStderr : 2,
                  QByteArray("[debug] connecting to ") + host + ':' + QByteArray::number(port));
        m_socket->connectToHost(QString::fromUtf8(host), quint16(port));
    }

    void onConnected()
    {
        m_inbound.clear();
        echoLocal(g_savedStderr >= 0 ? g_savedStderr : 2, QByteArray("[debug] connected"));
        drainQueue();
    }

    void onDisconnected()
    {
        m_shipping = false;
        m_inbound.clear();
        echoLocal(g_savedStderr >= 0 ? g_savedStderr : 2,
                  QByteArray("[debug] disconnected — will retry"));
    }

    void onSocketError(QAbstractSocket::SocketError)
    {
        if (!m_socket)
            return;
        echoLocal(g_savedStderr >= 0 ? g_savedStderr : 2,
                  QByteArray("[debug] socket error ") + m_socket->errorString().toUtf8());
        m_shipping = false;
        m_inbound.clear();
        m_socket->abort();
    }

    void onReadyRead()
    {
        m_inbound.append(m_socket->readAll());
        int nl = 0;
        while ((nl = m_inbound.indexOf('\n')) >= 0) {
            const QByteArray line = m_inbound.left(nl).trimmed();
            m_inbound.remove(0, nl + 1);
            if (line.isEmpty())
                continue;
            QJsonParseError err;
            const QJsonDocument doc = QJsonDocument::fromJson(line, &err);
            if (err.error != QJsonParseError::NoError || !doc.isObject())
                continue;
            const QString type = doc.object().value(QStringLiteral("type")).toString();
            const std::string t = type.toStdString();
            if (epaper::debuglog::isDocumentTypeOnDebugPort(t))
                continue;
            if (t == "debug_start")
                m_shipping = true;
            else if (t == "debug_stop")
                m_shipping = false;
            else if (t == "debug_request")
                continue;
            else
                continue;
        }
    }

    void drainQueue()
    {
        if (!m_shipping)
            return;
        if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState)
            return;
        if (m_socket->bytesToWrite() > 32768)
            return;
        const auto batch = m_queue->takeAll();
        for (const auto &item : batch)
            m_socket->write(recordJson(item.first, item.second));
    }

    void onStdout() { readPipe(m_stdoutFd, "stdout", m_stdoutBuf); }
    void onStderr() { readPipe(m_stderrFd, "stderr", m_stderrBuf); }

private:
    void readPipe(int fd, const char *level, QByteArray &acc)
    {
        if (fd < 0)
            return;
        char buf[2048];
        while (true) {
            const ssize_t n = ::read(fd, buf, sizeof(buf));
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                break;
            }
            if (n == 0)
                break;
            acc.append(buf, int(n));
            int nl = 0;
            while ((nl = acc.indexOf('\n')) >= 0) {
                QByteArray line = acc.left(nl);
                acc.remove(0, nl + 1);
                epaper::debuglog::DebugLogRecord rec;
                rec.ts = QDateTime::currentMSecsSinceEpoch();
                rec.level = level;
                rec.logger = "stdio";
                rec.msg = QString::fromUtf8(line).toStdString();
                m_queue->tryPush(std::move(rec));
            }
        }
    }

    epaper::debuglog::DebugLogQueue *m_queue = nullptr;
    QTcpSocket *m_socket = nullptr;
    QByteArray m_inbound;
    QByteArray m_stdoutBuf;
    QByteArray m_stderrBuf;
    int m_stdoutFd = -1;
    int m_stderrFd = -1;
    bool m_stdioOk = false;
    bool m_shipping = false;
};

DebugLogShip::DebugLogShip(QObject *parent)
    : QObject(parent)
{
}

DebugLogShip::~DebugLogShip()
{
    if (g_ship == this) {
        qInstallMessageHandler(nullptr);
        g_ship = nullptr;
    }
    m_thread.quit();
    m_thread.wait(2000);
#ifdef Q_OS_UNIX
    if (g_savedStdout >= 0) {
        dup2(g_savedStdout, STDOUT_FILENO);
        ::close(g_savedStdout);
        g_savedStdout = -1;
    }
    if (g_savedStderr >= 0) {
        dup2(g_savedStderr, STDERR_FILENO);
        ::close(g_savedStderr);
        g_savedStderr = -1;
    }
#endif
}

void DebugLogShip::startIfEnabled()
{
    m_enabled = epaper::debuglog::debugLogEnvOn(qgetenv("EPAPER_DEBUG_LOG").constData());
    if (!m_enabled)
        return;

    int stdoutFd = -1;
    int stderrFd = -1;
    bool stdioOk = false;
#ifdef Q_OS_UNIX
    int outp[2] = {-1, -1};
    int errp[2] = {-1, -1};
    const bool outOk = pipe(outp) == 0;
    const bool errOk = outOk && pipe(errp) == 0;
    if (outOk && errOk) {
        fcntl(outp[0], F_SETFL, O_NONBLOCK);
        fcntl(errp[0], F_SETFL, O_NONBLOCK);
        g_savedStdout = dup(STDOUT_FILENO);
        g_savedStderr = dup(STDERR_FILENO);
        if (g_savedStdout >= 0 && g_savedStderr >= 0
            && dup2(outp[1], STDOUT_FILENO) >= 0 && dup2(errp[1], STDERR_FILENO) >= 0) {
            ::close(outp[1]);
            ::close(errp[1]);
            stdoutFd = outp[0];
            stderrFd = errp[0];
            stdioOk = true;
        }
    }
    if (!stdioOk) {
        if (outp[0] >= 0)
            ::close(outp[0]);
        if (outp[1] >= 0)
            ::close(outp[1]);
        if (errp[0] >= 0)
            ::close(errp[0]);
        if (errp[1] >= 0)
            ::close(errp[1]);
    }
#endif

    g_ship = this;
    qInstallMessageHandler(&DebugLogShip::messageHandler);

    m_worker = new DebugLogWorker(&m_queue, stdoutFd, stderrFd, stdioOk);
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::started, m_worker, &DebugLogWorker::start);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread.start();
}

void DebugLogShip::tryEnqueue(const char *logger, const char *level, const QString &msg)
{
    epaper::debuglog::DebugLogRecord rec;
    rec.ts = QDateTime::currentMSecsSinceEpoch();
    rec.level = level ? level : "info";
    rec.logger = logger ? logger : "qt";
    rec.msg = msg.toStdString();
    m_queue.tryPush(std::move(rec));
}

void DebugLogShip::messageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    const char *level = "info";
    if (type == QtWarningMsg)
        level = "warning";
    else if (type == QtCriticalMsg || type == QtFatalMsg)
        level = "critical";

    if (g_ship)
        g_ship->tryEnqueue("qt", level, msg);

    echoLocal(g_savedStderr >= 0 ? g_savedStderr : 2, msg.toUtf8());

    if (type == QtFatalMsg)
        abort();
}

#include "debug_log_ship.moc"
