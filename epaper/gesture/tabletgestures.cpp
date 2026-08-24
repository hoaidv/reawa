#include "tabletgestures.h"

#include <QCoreApplication>
#include <QEvent>
#include <QEventPoint>
#include <QList>
#include <QPointF>
#include <QPointingDevice>
#include <QTabletEvent>
#include <QTouchEvent>
#include <QVector>
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
    m_penOnCanvas = false;
    m_penIdle->stop();
}

void TabletGestures::suppressCanvasTouch()
{
    if (m_twoFinger && m_canvas) {
        m_canvas->endTwoFingerTouch();
        m_twoFinger = false;
    }
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

bool TabletGestures::injectMapped(QObject *watched, QWindow *w, QTabletEvent *tablet,
                                 const QPointF &mapped)
{
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

bool TabletGestures::ingestPen(QObject *watched, QTabletEvent *tablet)
{
    auto *w = qobject_cast<QWindow *>(watched);
    if (!m_canvas || !w || m_injectingMapped)
        return false;

    const QPointF raw = tablet->position();
    const QPointF mapped = m_canvas->mapInputToCanvas(raw);
    const auto ch = channelsFrom(tablet);
    const QEvent::Type t = tablet->type();

    // Press/release to chrome TapHandlers only. Moves stay in C++ — tablet rate
    // through QML PointHandler + sendEvent was the pen lag.
    if (t == QEvent::TabletPress) {
        m_penOnCanvas = !m_canvas->isScreenChromeAt(mapped);
        if (m_penOnCanvas)
            m_canvas->ingestMappedTablet(t, mapped, raw, ch);
        else
            injectMapped(watched, w, tablet, mapped);
        return true;
    }
    if (t == QEvent::TabletRelease) {
        if (m_penOnCanvas)
            m_canvas->ingestMappedTablet(t, mapped, raw, ch);
        else
            injectMapped(watched, w, tablet, mapped);
        m_penOnCanvas = false;
        return true;
    }
    if (m_penOnCanvas)
        m_canvas->ingestMappedTablet(t, mapped, raw, ch);
    return true;
}

bool TabletGestures::handleTwoFinger(QTouchEvent *touch)
{
    if (!m_canvas)
        return false;

    QVector<const QEventPoint *> pressed;
    for (const QEventPoint &tp : touch->points()) {
        if (tp.state() != QEventPoint::State::Released)
            pressed.append(&tp);
    }
    auto canvasOf = [this](const QEventPoint &tp) {
        return m_canvas->mapFromGlobal(tp.globalPosition());
    };

    const QEvent::Type t = touch->type();
    if (t == QEvent::TouchEnd || t == QEvent::TouchCancel) {
        if (!m_twoFinger)
            return false;
        m_canvas->endTwoFingerTouch();
        m_twoFinger = false;
        // Let PointHandler see the end so its exclusive grab drops.
        return false;
    }

    if (m_twoFinger) {
        if (pressed.size() < 2) {
            m_canvas->endTwoFingerTouch();
            m_twoFinger = false;
            return true;
        }
        m_canvas->updateTwoFingerTouch(canvasOf(*pressed[0]), canvasOf(*pressed[1]));
        return true;
    }

    if (pressed.size() == 2 && m_canvas->canPromoteToTwoFinger() && m_canvas->handTouchArmed()) {
        m_twoFinger = m_canvas->beginTwoFingerTouch(canvasOf(*pressed[0]), canvasOf(*pressed[1]));
        return m_twoFinger;
    }
    return false;
}

bool TabletGestures::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel: {
        if (m_penNear || m_penDown) {
            suppressCanvasTouch();
            return true;
        }
        return handleTwoFinger(static_cast<QTouchEvent *>(event));
    }
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
            m_penNear = true;
            m_penDown = tablet->pressure() > 0.01;
        }
        if (ingestPen(watched, tablet))
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
