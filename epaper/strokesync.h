#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QVector>

class StrokeSync : public QObject
{
    Q_OBJECT

public:
    explicit StrokeSync(QObject *parent = nullptr);

    void connectToMac();
    void sendLine(const QByteArray &jsonLine);

private:
    void flushQueue();

    QTcpSocket m_socket;
    QByteArray m_buffer;
    QVector<QByteArray> m_queue;
};
