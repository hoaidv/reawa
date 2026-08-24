#include "qtinputfilter.h"

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

using epaper::input::PenSample;

QtInputFilter::QtInputFilter(QObject *parent)
    : QObject(parent)
{
    m_penIdle = new QTimer(this);
    m_penIdle->setSingleShot(true);
    m_penIdle->setInterval(600);
    connect(m_penIdle, &QTimer::timeout, this, [this]() {
        if (!m_penDown)
            setPenNear(false);
    });
}

void QtInputFilter::setPenNear(bool isNear)
{
    if (m_penNear == isNear)
        return;
    m_penNear = isNear;
    emit penNearChanged();
}

void QtInputFilter::setContacts(int live)
{
    if (m_contacts == live)
        return;
    m_contacts = live;
    emit contactCountChanged();
}

void QtInputFilter::notePenLeave()
{
    m_penDown = false;
    m_penIdle->stop();
    setPenNear(false);
}

PenSample QtInputFilter::channelsFrom(const QTabletEvent *tablet) const
{
    PenSample ch;
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

bool QtInputFilter::injectMapped(QObject *watched, QWindow *w, QTabletEvent *tablet,
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
 * this: MultiPointHandler exposes only the min/max it requires, never the live
 * count, and a passive PointHandler is not told which contact it holds. The
 * filter sees every point; what a second contact *means* is decided in QML.
 * @implements [SRS-EP-24] second contact outranks a one-finger manip
 */
void QtInputFilter::noteContacts(QTouchEvent *touch)
{
    int live = 0;
    for (const QEventPoint &p : touch->points()) {
        if (p.state() != QEventPoint::State::Released)
            ++live;
    }

    static const bool trace = qEnvironmentVariableIntValue("EPAPER_TOUCH_TRACE") > 0;
    if (trace) {
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
    setContacts(live);
}

/**
 * Unwind live grabs the Qt way when the pen takes over. Swallowing touch
 * mid-gesture starves whichever handler holds the grab: it never sees the
 * release, stays active, and keeps an exclusive grab Qt only reaps later. One
 * point-less TouchCancel makes QPointingDevicePrivate::sendTouchCancelEvent add
 * every grabbed point, deliver to each grabber, and clear exclusive *and* passive
 * grabs — after which "the next touch event can only be a TouchBegin".
 * @implements [SRS-EP-21] pen near wins over hand touch
 */
void QtInputFilter::cancelLiveTouch(QObject *watched, const QTouchEvent *touch)
{
    // Nothing on the glass means nothing can be grabbed. This also limits us to one
    // cancel per takeover: the caller zeroes the count right after.
    if (m_contacts <= 0)
        return;
    auto *w = qobject_cast<QWindow *>(watched);
    if (!w)
        return;
    QTouchEvent cancel(QEvent::TouchCancel, touch->pointingDevice(), touch->modifiers());
    m_injectingCancel = true;
    QCoreApplication::sendEvent(w, &cancel);
    m_injectingCancel = false;
}

bool QtInputFilter::remapPen(QObject *watched, QTabletEvent *tablet)
{
    auto *w = qobject_cast<QWindow *>(watched);
    if (!w || m_injectingMapped)
        return false;

    const QPointF raw = tablet->position();
    const QPointF mapped = epaper::input::mapPanel(raw, w->width(), w->height());
    // Tilt/rotation/tangential are not on a QML HandlerPoint. Publish them before
    // injecting, so the canvas holds the full set when a handler calls back during
    // the synchronous delivery below.
    emit penSample(raw, channelsFrom(tablet));
    return injectMapped(watched, w, tablet, mapped);
}

bool QtInputFilter::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        // Our own cancel, on its way to the handlers that need to hear it.
        if (m_injectingCancel)
            return false;
        // @implements [SRS-EP-21] pen near wins over hand touch
        if (m_penNear || m_penDown) {
            cancelLiveTouch(watched, static_cast<QTouchEvent *>(event));
            setContacts(0);
            return true;
        }
        noteContacts(static_cast<QTouchEvent *>(event));
        return false;
    case QEvent::TabletEnterProximity:
        setPenNear(true);
        return false;
    case QEvent::TabletLeaveProximity:
        notePenLeave();
        return false;
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease: {
        auto *tablet = static_cast<QTabletEvent *>(event);
        if (event->type() == QEvent::TabletPress)
            m_penDown = true;
        else if (event->type() == QEvent::TabletRelease)
            m_penDown = false;
        else
            m_penDown = tablet->pressure() > 0.01;
        setPenNear(true);
        m_penIdle->start();
        return remapPen(watched, tablet);
    }
    // Mouse synthesis is off in main(): swallowing a synth press here would leave
    // it accepted, handing the grab to whatever item sits under the finger.
    default:
        return false;
    }
}
