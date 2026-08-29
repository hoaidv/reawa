#include "qtinputfilter.h"

#include <QHoverEvent>
#include <QCoreApplication>
#include <QEvent>
#include <QEventPoint>
#include <QGuiApplication>
#include <QList>
#include <QPointF>
#include <QPointingDevice>
#include <QRectF>
#include <QScreen>
#include <QByteArray>
#include <QDebug>
#include <QString>
#include <QSocketNotifier>
#include <QTabletEvent>
#include <QTouchEvent>
#include <QVector>
#include <QWindow>

#if defined(Q_OS_LINUX)
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

using epaper::input::PenSample;

QtInputFilter::QtInputFilter(QObject *parent)
    : QObject(parent)
{
    m_penIdle = new QTimer(this);
    m_penIdle->setSingleShot(true);
    m_penIdle->setInterval(600);
    connect(m_penIdle, &QTimer::timeout, this, [this]() {
        // Evdev near-tracking is authoritative when the tool is still in range.
        // The idle timer only covers stacks that omit LeaveProximity.
        if (m_stylus.phase() != epaper::input::StylusPhase::Away)
            return;
        if (!m_penDown)
            setPenNear(false);
    });
    attachStylusProximity();
}

QtInputFilter::~QtInputFilter()
{
#if defined(Q_OS_LINUX)
    if (m_stylusFd >= 0)
        ::close(m_stylusFd);
#endif
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
    // Qt evdevtablet only delivers TabletMove while BTN_TOUCH is (or just was)
    // down. This branch is the lift-into-near frame; hover tracking is evdev.
    if (!m_penDown)
        emit penHover(mapped.x(), mapped.y());
    return injectMapped(watched, w, tablet, mapped);
}

void QtInputFilter::noteWatchedWindow(QObject *watched)
{
    if (auto *w = qobject_cast<QWindow *>(watched))
        m_lastWindow = w;
}

QWindow *QtInputFilter::hoverWindow() const
{
    if (m_lastWindow)
        return m_lastWindow.data();
    const auto windows = QGuiApplication::topLevelWindows();
    return windows.isEmpty() ? nullptr : windows.first();
}

void QtInputFilter::emitMappedHover(double windowX, double windowY)
{
    auto *w = hoverWindow();
    if (!w)
        return;
    const QPointF mapped = epaper::input::mapPanel(QPointF(windowX, windowY), w->width(), w->height());
    if (mapped == m_lastHover)
        return;
    m_lastHover = mapped;
    emit penHover(mapped.x(), mapped.y());
}

void QtInputFilter::applyStylusSyn()
{
    const auto phase = m_stylus.phase();
    if (phase == epaper::input::StylusPhase::Away) {
        m_lastHover = QPointF();
        notePenLeave();
        return;
    }
    setPenNear(true);
    m_penIdle->stop();
    if (!m_stylus.shouldEmitHover(m_penDown))
        return;
    auto *w = hoverWindow();
    QRectF winRect(0, 0, w ? w->width() : 0, w ? w->height() : 0);
    if (QScreen *screen = QGuiApplication::primaryScreen())
        winRect = screen->geometry();
    double gx = 0;
    double gy = 0;
    m_stylus.windowPos(winRect.width(), winRect.height(), &gx, &gy);
    QPointF local(gx, gy);
    if (w)
        local = QPointF(gx - w->position().x(), gy - w->position().y());
    emitMappedHover(local.x(), local.y());
}

void QtInputFilter::attachStylusProximity()
{
#if defined(Q_OS_LINUX)
    for (int i = 0; i < 32 && m_stylusFd < 0; ++i) {
        const QByteArray path = QByteArrayLiteral("/dev/input/event") + QByteArray::number(i);
        const int fd = ::open(path.constData(), O_RDONLY | O_NONBLOCK);
        if (fd < 0)
            continue;
        unsigned char keyBits[(KEY_MAX + 7) / 8] = {};
        if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits) < 0) {
            ::close(fd);
            continue;
        }
        const auto has = [&](int bit) {
            return keyBits[bit / 8] & static_cast<unsigned char>(1 << (bit % 8));
        };
        if (!has(BTN_TOOL_PEN) && !has(BTN_TOOL_RUBBER)) {
            ::close(fd);
            continue;
        }
        input_absinfo absX{};
        input_absinfo absY{};
        if (ioctl(fd, EVIOCGABS(ABS_X), &absX) < 0 || ioctl(fd, EVIOCGABS(ABS_Y), &absY) < 0) {
            ::close(fd);
            continue;
        }
        m_stylus.setAbsRange(absX.minimum, absX.maximum, absY.minimum, absY.maximum);
        m_stylusFd = fd;
        m_stylusNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
        connect(m_stylusNotifier, &QSocketNotifier::activated, this,
                [this](QSocketDescriptor, QSocketNotifier::Type) { onStylusReadable(); });
        qInfo().noquote() << QStringLiteral("[input] stylus proximity %1").arg(QString::fromLatin1(path));
        return;
    }
#else
    (void)0;
#endif
}

void QtInputFilter::onStylusReadable()
{
#if defined(Q_OS_LINUX)
    input_event buffer[32];
    for (;;) {
        const ssize_t n = ::read(m_stylusFd, buffer, sizeof(buffer));
        if (n <= 0)
            break;
        const int count = static_cast<int>(n / static_cast<ssize_t>(sizeof(input_event)));
        for (int i = 0; i < count; ++i) {
            const input_event &ev = buffer[i];
            m_stylus.feed(ev.type, ev.code, ev.value);
            if (ev.type == EV_SYN && ev.code == SYN_REPORT)
                applyStylusSyn();
        }
        if (n < static_cast<ssize_t>(sizeof(buffer)))
            break;
    }
#endif
}

bool QtInputFilter::eventFilter(QObject *watched, QEvent *event)
{
    noteWatchedWindow(watched);
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
    case QEvent::HoverMove: {
        if (!m_penNear || m_penDown)
            return false;
        auto *w = qobject_cast<QWindow *>(watched);
        auto *hover = static_cast<QHoverEvent *>(event);
        if (!w || !hover)
            return false;
        emitMappedHover(hover->position().x(), hover->position().y());
        return false;
    }
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
