#include "usb_link.hpp"
#include "net_retry.hpp"
#include "usb_path.hpp"
#include "usbgadget.hpp"

#include <QDebug>
#include <QMetaObject>
#include <QStringList>
#include <QTimer>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>

namespace epaper {
namespace {

bool parseEstablishedPorts(const char *path, bool *ssh22, bool *stroke9877, bool *log9878)
{
#ifdef __linux__
    FILE *f = std::fopen(path, "r");
    if (!f)
        return false;
    char line[512];
    if (!std::fgets(line, sizeof(line), f)) {
        std::fclose(f);
        return false;
    }
    while (std::fgets(line, sizeof(line), f)) {
        unsigned locPort = 0;
        unsigned remPort = 0;
        unsigned state = 0;
        if (std::sscanf(line, "%*d: %*x:%x %*x:%x %x", &locPort, &remPort, &state) < 3)
            continue;
        if (state != 0x01)
            continue;
        if (locPort == 22)
            *ssh22 = true;
        if (remPort == 9877 || locPort == 9877)
            *stroke9877 = true;
        if (remPort == 9878 || locPort == 9878)
            *log9878 = true;
    }
    std::fclose(f);
    return true;
#else
    (void)path;
    (void)ssh22;
    (void)stroke9877;
    (void)log9878;
    return false;
#endif
}

std::string firstUdcState()
{
#ifdef __linux__
    DIR *d = opendir("/sys/class/udc");
    if (!d)
        return {};
    std::string out;
    while (struct dirent *e = readdir(d)) {
        if (e->d_name[0] == '.')
            continue;
        char path[256];
        std::snprintf(path, sizeof(path), "/sys/class/udc/%s/state", e->d_name);
        FILE *f = std::fopen(path, "r");
        if (!f)
            continue;
        char buf[64] = {};
        if (std::fgets(buf, sizeof(buf), f)) {
            out = buf;
            while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
                out.pop_back();
        }
        std::fclose(f);
        break;
    }
    closedir(d);
    return out;
#else
    return {};
#endif
}

} // namespace

class UsbLinkWorker : public QObject
{
    Q_OBJECT

public slots:
    void start()
    {
        auto *tick = new QTimer(this);
        tick->setInterval(kUsbLinkCheckMs);
        connect(tick, &QTimer::timeout, this, &UsbLinkWorker::onTick);
        tick->start();
        onTick();
    }

signals:
    void hud(const QString &status, const QString &debug, const QString &linkState);
    void recoverApps();

public slots:
    void recover()
    {
        using usbgadget::assignUsb0Address;
        m_recoverNote = QStringLiteral("recover: usb0 addr + Infini 3 tries (no UDC write)");
        assignUsb0Address();
        onTick();
        emit recoverApps();
    }

private slots:
    void onTick()
    {
        using usbgadget::classify;
        using usbgadget::probe;

        const auto snap = probe();
        const auto cls = classify(snap);
        const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();

        bool ssh = false;
        bool stroke = false;
        bool log = false;
        parseEstablishedPorts("/proc/net/tcp", &ssh, &stroke, &log);
        parseEstablishedPorts("/proc/net/tcp6", &ssh, &stroke, &log);

        const std::string udc = firstUdcState();
        const bool udcConfigured = (udc == "configured");
        const bool udcSuspended = (udc == "suspended");
        const bool pathLive = udcConfigured && snap.hasTabletAddr && snap.flagsUp;
        epaper::setUsbEthernetLive(pathLive);

        const bool tcpStroke = stroke;
        const bool tcpLog = log;
        const bool tcpSsh = ssh;
        // Do not advertise STROKE/LOG while UDC is suspended (stale ESTABLISHED).
        const bool allowTcpHud = !udcSuspended && pathLive;
        stroke = allowTcpHud && tcpStroke;
        log = allowTcpHud && tcpLog;
        ssh = allowTcpHud && tcpSsh;

        if (stroke) {
            if (m_strokeUpMs == 0)
                m_strokeUpMs = nowMs;
            m_strokeDownMs = 0;
        } else {
            if (m_strokeDownMs == 0)
                m_strokeDownMs = nowMs;
            m_strokeUpMs = 0;
        }

        const bool unplugged = udc.empty()
            ? !(snap.ifacePresent && snap.hasTabletAddr)
            : (udc == "not attached");

        QString status;
        if (udcSuspended) {
            status = QStringLiteral("Suspended");
        } else if (unplugged) {
            status = QStringLiteral("Unplugged");
        } else if (!ssh && !stroke && !log) {
            status = QStringLiteral("Plugged");
        } else {
            QStringList parts;
            if (ssh)
                parts << QStringLiteral("SSH (:22)");
            if (stroke)
                parts << QStringLiteral("STROKE (:9877)");
            if (log)
                parts << QStringLiteral("LOG (:9878)");
            status = parts.join(QStringLiteral(" | "));
        }

        QString linkState = QStringLiteral("plugged");
        if (unplugged)
            linkState = QStringLiteral("unplugged");
        else if (pathLive && stroke)
            linkState = QStringLiteral("connected");

        QString debug = QStringLiteral("udc=%1 vbus/plug=%2 usb0=%3 up=%4 addr=%5 carrier=%6 class=%7")
                            .arg(QString::fromStdString(udc.empty() ? "?" : udc))
                            .arg(udcConfigured ? 1 : 0)
                            .arg(snap.ifacePresent ? 1 : 0)
                            .arg(snap.flagsUp ? 1 : 0)
                            .arg(snap.hasTabletAddr ? 1 : 0)
                            .arg(snap.carrier ? 1 : 0)
                            .arg(QLatin1String(usbgadget::classLabel(cls)));
        if (stroke && m_strokeUpMs)
            debug += QStringLiteral(" | Infini up %1s").arg((nowMs - m_strokeUpMs) / 1000);
        else if (!stroke && !unplugged && m_strokeDownMs)
            debug += QStringLiteral(" | Infini down %1s").arg((nowMs - m_strokeDownMs) / 1000);
        if (udcSuspended)
            debug += QStringLiteral(" | host USB suspend (lid/sleep) — TCP hold");
        if (udcConfigured && !snap.carrier)
            debug += QStringLiteral(" | carrier=0 (g_ether; ignore if ping works)");
        if ((tcpStroke || tcpLog || tcpSsh) && !pathLive)
            debug += QStringLiteral(" | ignore stale TCP ESTABLISHED");
        if (unplugged)
            debug += QStringLiteral(" | cable/host not attached");
        else if (!udcSuspended && !snap.hasTabletAddr)
            debug += QStringLiteral(" | no 10.11.99.1");
        else if (pathLive && !stroke)
            debug += QStringLiteral(" | :9877 not established");
        debug += QStringLiteral(" | no UDC write; Infini 3 tries / button");
        if (!m_recoverNote.isEmpty())
            debug += QStringLiteral(" | ") + m_recoverNote;

        emit hud(status, debug, linkState);
    }

private:
    qint64 m_strokeUpMs = 0;
    qint64 m_strokeDownMs = 0;
    QString m_recoverNote;
};

UsbLink *UsbLink::s_instance = nullptr;

UsbLink *UsbLink::instance()
{
    return s_instance;
}

UsbLink::UsbLink(QObject *parent)
    : QObject(parent)
{
    s_instance = this;
}

UsbLink::~UsbLink()
{
    m_thread.quit();
    m_thread.wait(2000);
}

void UsbLink::start()
{
    if (m_started)
        return;
    m_started = true;
    m_worker = new UsbLinkWorker;
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::started, m_worker, &UsbLinkWorker::start);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &UsbLinkWorker::hud, this, &UsbLink::onHud, Qt::QueuedConnection);
    connect(m_worker, &UsbLinkWorker::recoverApps, this, &UsbLink::onRecoverApps,
            Qt::QueuedConnection);
    m_thread.start();
}

void UsbLink::recoverInfini()
{
    if (!m_worker)
        return;
    QMetaObject::invokeMethod(m_worker, "recover", Qt::QueuedConnection);
}

void UsbLink::onRecoverApps()
{
    emit requestAppReconnect();
}

void UsbLink::onHud(const QString &status, const QString &debug, const QString &linkState)
{
    if (status == m_status && debug == m_debug && linkState == m_linkState)
        return;
    m_status = status;
    m_debug = debug;
    m_linkState = linkState;
    emit hudChanged();
}

} // namespace epaper

#include "usb_link.moc"
