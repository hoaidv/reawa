#include "tabletwindow.h"
#include "tabletcanvasitem.h"

#include <QMouseEvent>
#include <QDebug>

TabletWindow::TabletWindow(QWindow *parent)
    : QQuickWindow(parent)
{
}

void TabletWindow::setCanvas(TabletCanvasItem *canvas)
{
    if (m_canvas == canvas)
        return;
    m_canvas = canvas;
    emit canvasChanged(m_canvas);
}

void TabletWindow::tabletEvent(QTabletEvent *event)
{
    ++m_debugEvents;
    emit debugEventsChanged();

    const QPointF pos = event->position();
    const qreal pressure = event->pressure();

    switch (event->type()) {
    case QEvent::TabletPress:
        forwardPoint(QEvent::TabletPress, pos, pressure);
        break;
    case QEvent::TabletMove:
        forwardPoint(QEvent::TabletMove, pos, pressure);
        break;
    case QEvent::TabletRelease:
        forwardPoint(QEvent::TabletRelease, pos, pressure);
        break;
    default:
        break;
    }

    event->accept();
}

bool TabletWindow::event(QEvent *event)
{
    // Some RM builds synthesize pen as mouse events.
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease: {
        auto *mouse = static_cast<QMouseEvent *>(event);
        if (mouse->source() == Qt::MouseEventNotSynthesized
            || mouse->source() == Qt::MouseEventSynthesizedBySystem) {
            ++m_debugEvents;
            emit debugEventsChanged();
            const qreal pressure = (event->type() == QEvent::MouseButtonRelease) ? 0.0 : 0.75;
            forwardPoint(event->type(), mouse->position(), pressure);
            return true;
        }
        break;
    }
    default:
        break;
    }

    return QQuickWindow::event(event);
}

void TabletWindow::forwardPoint(QEvent::Type type, const QPointF &pos, qreal pressure)
{
    if (!m_canvas)
        return;
    // Fullscreen canvas: tablet positions are already window-local.
    m_canvas->ingestPoint(type, pos, pressure);
}
