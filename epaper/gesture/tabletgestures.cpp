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
    m_penIdle->stop();
}

void TabletGestures::suppressCanvasTouch()
{
    if (!m_canvas)
        return;
    m_canvas->cancelHandTouch();
    m_canvas->onContactsCleared();
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

/**
 * Contact count, observed without consuming the event. Qt handlers cannot answer
 * this: a passive PointHandler is not told which contact it holds, and a grabbing
 * handler only learns of contacts it wins. The filter sees every point.
 * @implements [SRS-EP-24] second contact outranks a one-finger manip
 */
void TabletGestures::noteContacts(QTouchEvent *touch)
{
    int live = 0;
    for (const QEventPoint &p : touch->points()) {
        if (p.state() != QEventPoint::State::Released)
            ++live;
    }

    static const bool trace = qEnvironmentVariableIntValue("EPAPER_TOUCH_TRACE") > 0;
    if (trace || live != m_contacts) {
        QString where;
        for (const QEventPoint &p : touch->points()) {
            where += QStringLiteral(" (%1,%2)")
                         .arg(int(p.position().x()))
                         .arg(int(p.position().y()));
        }
        qInfo().noquote() << QStringLiteral("[touch] %1 n=%2->%3%4")
                                 .arg(touch->type() == QEvent::TouchBegin ? QStringLiteral("begin")
                                          : touch->type() == QEvent::TouchUpdate
                                              ? QStringLiteral("move")
                                              : touch->type() == QEvent::TouchEnd
                                                  ? QStringLiteral("end")
                                                  : QStringLiteral("cancel"))
                                 .arg(m_contacts)
                                 .arg(live)
                                 .arg(where);
    }
    if (live == m_contacts)
        return;
    m_contacts = live;
    if (!m_canvas)
        return;
    if (live >= 2)
        m_canvas->onSecondContact();
    else if (live == 0)
        m_canvas->onContactsCleared();
}

bool TabletGestures::remapPen(QObject *watched, QTabletEvent *tablet)
{
    auto *w = qobject_cast<QWindow *>(watched);
    if (!m_canvas || !w || m_injectingMapped)
        return false;

    const QPointF raw = tablet->position();
    const QPointF mapped = m_canvas->mapInputToCanvas(raw);
    // Tilt/rotation/tangential are not on a QML HandlerPoint; park them so the
    // canvas can pick the full channel set up when Qt delivers the point.
    m_canvas->stashTabletSample(raw, channelsFrom(tablet));
    return injectMapped(watched, w, tablet, mapped);
}

bool TabletGestures::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        // @implements [SRS-EP-21] pen near wins over hand touch
        if (m_penNear || m_penDown) {
            m_contacts = 0;
            suppressCanvasTouch();
            return true;
        }
        noteContacts(static_cast<QTouchEvent *>(event));
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
        if (event->type() == QEvent::TabletPress) {
            notePenNear(true);
        } else if (event->type() == QEvent::TabletRelease) {
            m_penDown = false;
            m_penNear = true;
            m_penIdle->start();
        } else {
            // No suppressCanvasTouch() on move: it aborted a live pen manip.
            m_penNear = true;
            m_penDown = tablet->pressure() > 0.01;
            m_penIdle->start();
        }
        if (remapPen(watched, tablet))
            return true;
        return false;
    }
    // Synthesized mouse from tablet/touch would double every gesture.
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        return true;
    default:
        return false;
    }
}
