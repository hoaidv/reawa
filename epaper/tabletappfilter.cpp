#include "tabletappfilter.h"
#include "tabletcanvasitem.h"

#include <QMouseEvent>
#include <QTabletEvent>
#include <QTouchEvent>
#include <QDebug>

TabletAppFilter::TabletAppFilter(QObject *parent)
    : QObject(parent)
{
}

bool TabletAppFilter::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);

    // @implements [SRS-EP-04] touch reachability spike — count only, do not ink
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel: {
        ++m_touchEventCount;
        if (!m_touchReachableLogged) {
            m_touchReachableLogged = true;
            qInfo().nospace()
                << "[STORY-EP-004] capacitive touch REACHABLE via QEvent::"
                << (event->type() == QEvent::TouchBegin   ? "TouchBegin"
                    : event->type() == QEvent::TouchUpdate ? "TouchUpdate"
                    : event->type() == QEvent::TouchEnd    ? "TouchEnd"
                                                           : "TouchCancel")
                << " (filter path: TabletAppFilter::eventFilter)";
        }
        // Do not consume — spike probe only; toolbar will own touch later.
        return false;
    }
    default:
        break;
    }

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
