#include "native_window.hpp"

#include "stylus_handler.hpp"

#include <QTabletEvent>

namespace {

// Digitizer (landscape) to panel (portrait). Same map as epaper_old mapPanel.
QPointF mapPanel(const QPointF &raw, qreal panelW, qreal panelH)
{
    const qreal w = qMax<qreal>(1.0, panelW);
    const qreal h = qMax<qreal>(1.0, panelH);
    return QPointF(raw.y() * (w / h), h - raw.x() * (h / w));
}

} // namespace

NativeWindow::NativeWindow(QWindow *parent)
    : QQuickWindow(parent)
{
}

bool NativeWindow::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease:
        break;
    default:
        return QQuickWindow::event(event);
    }

    auto *tablet = static_cast<QTabletEvent *>(event);
    const QPointF p = mapPanel(tablet->position(), width(), height());
    const qreal pressure = tablet->pressure();
    const qreal tiltX = tablet->xTilt();
    const qreal tiltY = tablet->yTilt();

    const auto handlers = findChildren<StylusHandler *>();
    for (StylusHandler *handler : handlers) {
        switch (event->type()) {
        case QEvent::TabletPress:
            handler->press(p.x(), p.y(), pressure, tiltX, tiltY);
            break;
        case QEvent::TabletMove:
            handler->move(p.x(), p.y(), pressure, tiltX, tiltY);
            break;
        case QEvent::TabletRelease:
            handler->release(p.x(), p.y(), pressure, tiltX, tiltY);
            break;
        default:
            break;
        }
    }

    // Swallow the event so PointHandler does not draw the pen a second time.
    tablet->accept();
    return true;
}
