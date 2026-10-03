#pragma once

#include <QEvent>
#include <QQuickWindow>
#include <QtQml/qqmlregistration.h>

// Root window. Stylus press, move, and release are taken here, mapped into
// panel coordinates, and delivered to StylusHandler children. Every other
// event is passed to QQuickWindow. Proximity is not among the tablet events.
class NativeWindow : public QQuickWindow {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit NativeWindow(QWindow *parent = nullptr);

protected:
    bool event(QEvent *event) override;
};
