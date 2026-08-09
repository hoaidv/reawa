#include "tabletappfilter.h"
#include "tabletcanvasitem.h"

#include <QMouseEvent>
#include <QTabletEvent>

TabletAppFilter::TabletAppFilter(QObject *parent)
    : QObject(parent)
{
}

bool TabletAppFilter::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    if (!m_canvas)
        return false;

    switch (event->type()) {
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease: {
        auto *tablet = static_cast<QTabletEvent *>(event);
        m_canvas->ingestPoint(event->type(), tablet->position(), tablet->pressure());
        return true;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease: {
        auto *mouse = static_cast<QMouseEvent *>(event);
        const qreal pressure = event->type() == QEvent::MouseButtonRelease ? 0.0 : 0.75;
        m_canvas->ingestPoint(event->type(), mouse->position(), pressure);
        return true;
    }
    default:
        return false;
    }
}
