/**
 * Infra: USB gadget stay-up. Independent of StrokeSync / debug-log TCP.
 * Probe + restore run on a worker thread only.
 * @fix [STORY-EP-036]
 */
#pragma once

#include <QObject>
#include <QThread>

namespace epaper {

class UsbLinkWorker;

class UsbLink : public QObject
{
    Q_OBJECT

public:
    explicit UsbLink(QObject *parent = nullptr);
    ~UsbLink() override;

    void start();

private slots:
    void onReport(int cls, bool iface, bool up, bool addr, bool carrier, bool didRestore,
                  bool restoreOk);

private:
    QThread m_thread;
    UsbLinkWorker *m_worker = nullptr;
    int m_lastCls = -1;
    bool m_started = false;
};

} // namespace epaper
