#include "usb_link.hpp"
#include "net_retry.hpp"
#include "usbgadget.hpp"

#include <QDebug>
#include <QTimer>

#include <chrono>

namespace epaper {

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
    void report(int cls, bool iface, bool up, bool addr, bool carrier, bool didRestore,
                bool restoreOk);

private slots:
    void onTick()
    {
        using usbgadget::LinkClass;
        using usbgadget::classify;
        using usbgadget::probe;
        using usbgadget::restoreWithoutUnplug;
        using usbgadget::shouldRestoreGadget;

        const auto snap = probe();
        const LinkClass cls = classify(snap);
        bool didRestore = false;
        bool restoreOk = false;
        const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();
        if (shouldRestoreGadget(cls) && (m_lastRestoreMs == 0 || nowMs - m_lastRestoreMs >= 20000)) {
            m_lastRestoreMs = nowMs;
            didRestore = true;
            restoreOk = restoreWithoutUnplug();
        }
        emit report(int(cls), snap.ifacePresent, snap.flagsUp, snap.hasTabletAddr, snap.carrier,
                    didRestore, restoreOk);
    }

private:
    qint64 m_lastRestoreMs = 0;
};

UsbLink::UsbLink(QObject *parent)
    : QObject(parent)
{
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
    connect(m_worker, &UsbLinkWorker::report, this, &UsbLink::onReport, Qt::QueuedConnection);
    m_thread.start();
}

void UsbLink::onReport(int cls, bool iface, bool up, bool addr, bool carrier, bool didRestore,
                       bool restoreOk)
{
    using usbgadget::classLabel;
    using usbgadget::LinkClass;
    if (cls != m_lastCls) {
        qInfo() << "[usb-link]" << classLabel(LinkClass(cls)) << "usb0=" << iface << "up=" << up
                << "addr=" << addr << "carrier=" << carrier;
        m_lastCls = cls;
    }
    if (!didRestore)
        return;
    if (restoreOk)
        qInfo() << "[usb-link] restore finished — local usb0 addressed; "
                   "if Mac ping still fails, unplug is still required";
    else
        qWarning() << "[usb-link] restore failed — not claiming unplug is required";
}

} // namespace epaper

#include "usb_link.moc"
