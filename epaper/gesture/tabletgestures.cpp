#include "tabletgestures.h"

#include <QCoreApplication>
#include <QEvent>
#include <QPointF>
#include <QPointingDevice>
#include <QTabletEvent>
#include <QWindow>

TabletGestures::TabletGestures(QObject *parent)
    : QObject(parent)
{
    m_penIdle = new QTimer(this);
    m_penIdle->setSingleShot(true);
    m_penIdle->setInterval(600);
    connect(m_penIdle, &QTimer::timeout, this, [this]() {
        if (!m_penDown)
            m_penNear = false;
    });
}

void TabletGestures::setCanvas(TabletCanvasItem *canvas)
{
    m_canvas = canvas;
}

void TabletGestures::notePenNear(bool contact)
{
    m_penNear = true;
    if (contact)
        m_penDown = true;
    suppressCanvasTouch();
    m_penIdle->start();
}

void TabletGestures::notePenLeave()
{
    m_penNear = false;
    m_penDown = false;
    m_penIdle->stop();
}

void TabletGestures::suppressCanvasTouch()
{
    if (m_canvas)
        m_canvas->cancelHandTouch();
}

TabletCanvasItem::IngestChannels TabletGestures::channelsFrom(const QTabletEvent *tablet) const
{
    TabletCanvasItem::IngestChannels ch;
    ch.pressure = tablet->pressure();
    const QPointingDevice *dev = tablet->pointingDevice();
    const auto caps = dev ? dev->capabilities() : QPointingDevice::Capabilities{};
    using Cap = QPointingDevice::Capability;
    if (caps.testFlag(Cap::XTilt) || caps.testFlag(Cap::YTilt)) {
        ch.hasTilt = true;
        ch.tiltX = tablet->xTilt();
        ch.tiltY = tablet->yTilt();
    }
    if (caps.testFlag(Cap::ZPosition)) {
        ch.hasDistance = true;
        ch.distance = tablet->z();
    }
    ch.hasTimestamp = true;
    ch.timestamp = qreal(tablet->timestamp());
    if (caps.testFlag(Cap::Rotation)) {
        ch.hasRotation = true;
        ch.rotation = tablet->rotation();
    }
    if (caps.testFlag(Cap::TangentialPressure)) {
        ch.hasTangential = true;
        ch.tangential = tablet->tangentialPressure();
    }
    return ch;
}

bool TabletGestures::injectMappedTabletIfWindow(QObject *watched, QTabletEvent *tablet)
{
    auto *w = qobject_cast<QWindow *>(watched);
    if (!m_canvas || !w || m_injectingMapped)
        return false;
    const QPointF raw = tablet->position();
    // Stash before sendEvent so PointHandler onPointerStart sees tilt/rotation.
    m_canvas->stashTabletSample(raw, channelsFrom(tablet));
    const QPointF mapped = m_canvas->mapInputToCanvas(raw);
    QTabletEvent mappedEv(tablet->type(), tablet->pointingDevice(), mapped,
                          w->mapToGlobal(mapped), tablet->pressure(),
                          float(tablet->xTilt()), float(tablet->yTilt()),
                          float(tablet->tangentialPressure()), tablet->rotation(),
                          float(tablet->z()), tablet->modifiers(), tablet->button(),
                          tablet->buttons());
    mappedEv.setTimestamp(tablet->timestamp());
    m_injectingMapped = true;
    QCoreApplication::sendEvent(watched, &mappedEv);
    m_injectingMapped = false;
    return true;
}

bool TabletGestures::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        // Pen near: eat capacitive so PointHandler does not pan while inking.
        // ≥3-contact eat removed — palm is 20 mm travel ([CHL-0027]).
        if (m_penNear || m_penDown) {
            suppressCanvasTouch();
            return true;
        }
        return false;
    default:
        break;
    }

    if (!m_canvas)
        return false;

    switch (event->type()) {
    case QEvent::TabletEnterProximity:
        m_penNear = true;
        suppressCanvasTouch();
        return false;
    case QEvent::TabletLeaveProximity:
        notePenLeave();
        return false;
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease: {
        auto *tablet = static_cast<QTabletEvent *>(event);
        if (event->type() == QEvent::TabletPress)
            notePenNear(true);
        else if (event->type() == QEvent::TabletRelease) {
            m_penDown = false;
            m_penNear = true;
            m_penIdle->start();
        } else {
            // Keep proximity, but do not cancelHandTouch — that would abort
            // an in-flight pen move/resize that shares ManipDrag with fingers.
            m_penNear = true;
            m_penDown = tablet->pressure() > 0.01;
            m_penIdle->start();
        }
        if (injectMappedTabletIfWindow(watched, tablet))
            return true;
        return false;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        return true;
    default:
        return false;
    }
}
