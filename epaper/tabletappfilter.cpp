#include "tabletappfilter.h"
#include "tabletcanvasitem.h"

#include <QMouseEvent>
#include <QTabletEvent>
#include <QTouchEvent>
#include <QEventPoint>
#include <QDebug>

TabletAppFilter::TabletAppFilter(QObject *parent)
    : QObject(parent)
{
}

bool TabletAppFilter::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);

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
        // @implements [SRS-EP-04] finger tap on ToolChip → armTool (STORY-EP-006)
        if (m_canvas && event->type() == QEvent::TouchEnd) {
            auto *touch = static_cast<QTouchEvent *>(event);
            for (const QEventPoint &tp : touch->points()) {
                if (tp.state() != QEventPoint::State::Released)
                    continue;
                const QPointF canvasPos = m_canvas->mapFromGlobal(tp.globalPosition());
                if (m_canvas->tryArmToolAtCanvasPos(canvasPos))
                    return true;
            }
        }
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
