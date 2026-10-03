#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

// Declared inside NativeWindow the way PointHandler is. The window maps the
// tablet sample and calls press/move/release; this object does not see the
// QTabletEvent itself.
class StylusHandler : public QObject {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit StylusHandler(QObject *parent = nullptr);

    void press(qreal x, qreal y, qreal pressure, qreal tiltX, qreal tiltY);
    void move(qreal x, qreal y, qreal pressure, qreal tiltX, qreal tiltY);
    void release(qreal x, qreal y, qreal pressure, qreal tiltX, qreal tiltY);

signals:
    void stylusPress(qreal x, qreal y, qreal pressure, qreal tiltX, qreal tiltY);
    void stylusMove(qreal x, qreal y, qreal pressure, qreal tiltX, qreal tiltY);
    void stylusRelease(qreal x, qreal y, qreal pressure, qreal tiltX, qreal tiltY);
};
