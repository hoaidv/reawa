#include "usb_link.hpp"
#include "net_retry.hpp"
#include "usb_path.hpp"
#include "usbgadget.hpp"

#include <QMetaObject>
#include <QStringList>
#include <QTimer>

#include <cstdio>
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
        connect(tick, &QTimer::timeout, this, &UsbLinkWorker::refresh);
        tick->start();
        refresh();
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
        refresh();
        emit recoverApps();
    }

    void refresh()
    {
        using usbgadget::assignUsb0Address;
        using usbgadget::classify;
        using usbgadget::probe;

        auto snap = probe();
        const std::string udc = firstUdcState();
        const bool udcConfigured = (udc == "configured");
        const bool udcSuspended = (udc == "suspended");
        bool unplugged = udc.empty()
            ? !(snap.ifacePresent && snap.hasTabletAddr)
            : (udc == "not attached");
        bool pluggedNow = !unplugged && !udcSuspended;
        if (pluggedNow && classify(snap) == usbgadget::LinkClass::GadgetDown) {
            assignUsb0Address();
            snap = probe();
            unplugged = udc.empty()
                ? !(snap.ifacePresent && snap.hasTabletAddr)
                : (udc == "not attached");
            pluggedNow = !unplugged && !udcSuspended;
        }

        bool ssh = false;
        bool stroke = false;
        bool log = false;
        parseEstablishedPorts("/proc/net/tcp", &ssh, &stroke, &log);
        parseEstablishedPorts("/proc/net/tcp6", &ssh, &stroke, &log);

        const bool pathLive = udcConfigured && snap.hasTabletAddr && snap.flagsUp;
        epaper::setUsbEthernetLive(pathLive);

        const bool tcpStroke = stroke;
        const bool tcpLog = log;
        const bool tcpSsh = ssh;
        const bool allowTcpHud = !udcSuspended && pathLive;
        stroke = allowTcpHud && tcpStroke;
        log = allowTcpHud && tcpLog;
        ssh = allowTcpHud && tcpSsh;

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

        QString debug;
        if (udcSuspended)
            debug = QStringLiteral("host USB suspend (lid/sleep) — TCP hold");
        if (!m_recoverNote.isEmpty()) {
            if (!debug.isEmpty())
                debug += QStringLiteral(" | ");
            debug += m_recoverNote;
        }

        if (!pluggedNow && m_wasPlugged)
            emit recoverApps();
        if (pluggedNow && !m_wasPlugged) {
            m_recoverNote = QStringLiteral("plugged: usb0 addr + Infini retry");
            emit recoverApps();
        } else if (pathLive && !stroke && pluggedNow) {
            emit recoverApps();
        }
        m_wasPlugged = pluggedNow;

        emit hud(status, debug, linkState);
    }

private:
    QString m_recoverNote;
    bool m_wasPlugged = false;
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

void UsbLink::refreshHud()
{
    if (!m_worker)
        return;
    QMetaObject::invokeMethod(m_worker, "refresh", Qt::QueuedConnection);
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
