/**
 * Infra: USB gadget probe + HUD. Never UDC unbind. Manual recover = usb0 addr + TCP retry.
 * @fix [STORY-EP-036]
 */
#pragma once

#include <QObject>
#include <QString>
#include <QThread>

namespace epaper {

class UsbLinkWorker;

class UsbLink : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY hudChanged)
    Q_PROPERTY(QString debugLine READ debugLine NOTIFY hudChanged)
    /** unplugged | plugged | connected */
    Q_PROPERTY(QString linkState READ linkState NOTIFY hudChanged)

public:
    static UsbLink *instance();

    explicit UsbLink(QObject *parent = nullptr);
    ~UsbLink() override;

    void start();

    QString status() const { return m_status; }
    QString debugLine() const { return m_debug; }
    QString linkState() const { return m_linkState; }

    /** Button: USB bring-up if needed, then Infini TCP apps get 3 retries. */
    Q_INVOKABLE void recoverInfini();

signals:
    void hudChanged();
    void requestAppReconnect();

private slots:
    void onHud(const QString &status, const QString &debug, const QString &linkState);
    void onRecoverApps();

private:
    static UsbLink *s_instance;
    QThread m_thread;
    UsbLinkWorker *m_worker = nullptr;
    QString m_status = QStringLiteral("Unplugged");
    QString m_debug;
    QString m_linkState = QStringLiteral("unplugged");
    bool m_started = false;
};

} // namespace epaper
