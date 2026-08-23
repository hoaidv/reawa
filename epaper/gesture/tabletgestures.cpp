#include "tabletgestures.h"

#include "document/hand_touch.hpp"
#include "canvaspointeritem.h"

#include <QQuickItem>
#include <QQuickWindow>
#include <QCoreApplication>
#include <QDebug>
#include <QEventPoint>
#include <QList>
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
    if (!m_canvas)
        return;
    const auto kids = m_canvas->childItems();
    for (QQuickItem *k : kids) {
        if (auto *p = qobject_cast<CanvasPointerItem *>(k))
            p->setGestures(this);
    }
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
    if (m_fingerId >= 0 || m_twoFinger) {
        m_ignoreUntilUp = true;
        m_fingerId = -1;
        m_fingerId2 = -1;
        m_twoFinger = false;
    }
}

bool TabletGestures::canvasHandTouchOn() const
{
    const bool toggleOn = m_canvas && m_canvas->handTouchArmed();
    return epaper::handtouch::handTouchEnabled(m_penNear, m_penDown, toggleOn);
}

void TabletGestures::resetFingers()
{
    m_fingerId = -1;
    m_fingerId2 = -1;
    m_twoFinger = false;
    m_ignoreUntilUp = false;
    m_lastHandLogCount = -1;
}

static const QEventPoint *findPointId(const QList<QEventPoint> &pts, int id)
{
    if (id < 0)
        return nullptr;
    for (const QEventPoint &tp : pts) {
        if (tp.id() == id)
            return &tp;
    }
    return nullptr;
}

static QString handTouchLogLine(const char *phase, const QVector<const QEventPoint *> &pts,
                                TabletCanvasItem *canvas)
{
    QString s = QStringLiteral("[hand] %1 n=%2").arg(QLatin1String(phase)).arg(pts.size());
    for (const QEventPoint *p : pts) {
        QPointF c = p->position();
        if (canvas)
            c = canvas->mapFromGlobal(p->globalPosition());
        s += QStringLiteral(" #%1(%2,%3)")
                 .arg(p->id())
                 .arg(qRound(c.x()))
                 .arg(qRound(c.y()));
    }
    return s;
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

static QQuickItem *leafAtScene(QQuickItem *root, const QPointF &scene)
{
    QQuickItem *cur = root;
    for (int i = 0; i < 32 && cur; ++i) {
        const QPointF local = cur->mapFromScene(scene);
        QQuickItem *child = cur->childAt(local.x(), local.y());
        if (!child)
            return cur;
        cur = child;
    }
    return cur;
}

static bool itemOrAncestorHasTapHandler(QQuickItem *item)
{
    while (item) {
        for (QQuickItem *c : item->childItems()) {
            const char *n = c->metaObject()->className();
            if (n && QLatin1String(n).contains(QLatin1String("TapHandler")))
                return true;
        }
        item = item->parentItem();
    }
    return false;
}

bool TabletGestures::injectMappedTabletIfWindow(QObject *watched, QTabletEvent *tablet)
{
    auto *w = qobject_cast<QWindow *>(watched);
    if (!m_canvas || !w || m_injectingMapped)
        return false;
    m_tabletRaw = tablet->position();
    const QPointF mapped = m_canvas->mapInputToCanvas(m_tabletRaw);
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

    // Qt 6.8 does not deliver QTabletEvent to QQuickItem leftover targets (TapHandler
    // still runs). Ingest here when the pick is not chrome. @implements [SRS-EP-04]
    auto *qw = qobject_cast<QQuickWindow *>(watched);
    QQuickItem *leaf = (qw && qw->contentItem()) ? leafAtScene(qw->contentItem(), mapped)
                                                 : nullptr;
    if (!itemOrAncestorHasTapHandler(leaf))
        handleLeftoverTablet(&mappedEv);
    return true;
}

bool TabletGestures::handleLeftoverTablet(QTabletEvent *tablet)
{
    if (!m_canvas)
        return false;
    m_canvas->ingestMappedTablet(tablet->type(), tablet->position(), m_tabletRaw,
                                 channelsFrom(tablet));
    return true;
}

bool TabletGestures::onTouchBegin(QTouchEvent *touch)
{
    const auto pts = touch->points();
    QVector<const QEventPoint *> pressed;
    for (const QEventPoint &tp : pts) {
        if (tp.state() != QEventPoint::State::Released)
            pressed.append(&tp);
    }
    auto canvasOf = [this](const QEventPoint &tp) {
        return m_canvas->mapFromGlobal(tp.globalPosition());
    };
    qInfo().noquote() << handTouchLogLine("begin", pressed, m_canvas);
    m_lastHandLogCount = pressed.size();
    m_handLogClock.restart();

    if (m_ignoreUntilUp)
        return true;
    if (epaper::handtouch::palmByContactCount(pressed.size())) {
        suppressCanvasTouch();
        return true;
    }
    if (!canvasHandTouchOn())
        return false;
    if (pressed.size() == 2) {
        m_fingerId = pressed[0]->id();
        m_fingerId2 = pressed[1]->id();
        m_twoFinger = m_canvas->beginTwoFingerTouch(canvasOf(*pressed[0]), canvasOf(*pressed[1]));
        if (!m_twoFinger) {
            m_fingerId2 = -1;
            m_canvas->beginFingerTouch(canvasOf(*pressed[0]));
        }
        return true;
    }
    if (pressed.size() != 1)
        return true;
    m_fingerId = pressed[0]->id();
    m_fingerId2 = -1;
    m_twoFinger = false;
    m_canvas->beginFingerTouch(canvasOf(*pressed[0]));
    return true;
}

bool TabletGestures::onTouchUpdate(QTouchEvent *touch)
{
    const auto pts = touch->points();
    QVector<const QEventPoint *> pressed;
    for (const QEventPoint &tp : pts) {
        if (tp.state() != QEventPoint::State::Released)
            pressed.append(&tp);
    }
    auto canvasOf = [this](const QEventPoint &tp) {
        return m_canvas->mapFromGlobal(tp.globalPosition());
    };
    if (epaper::handtouch::palmByContactCount(pressed.size())) {
        suppressCanvasTouch();
        return true;
    }
    if (m_ignoreUntilUp)
        return true;
    if (!canvasHandTouchOn() && !m_twoFinger && m_fingerId < 0)
        return false;
    if (m_twoFinger) {
        if (pressed.size() < 2) {
            m_canvas->endTwoFingerTouch();
            m_twoFinger = false;
            m_ignoreUntilUp = true;
            m_fingerId2 = -1;
            return true;
        }
        const QEventPoint *a = findPointId(pts, m_fingerId);
        const QEventPoint *b = findPointId(pts, m_fingerId2);
        if (!a && pressed.size() > 0)
            a = pressed[0];
        if (!b && pressed.size() > 1)
            b = pressed[1];
        if (a && b)
            m_canvas->updateTwoFingerTouch(canvasOf(*a), canvasOf(*b));
        return true;
    }
    if (pressed.size() == 2 && canvasHandTouchOn() && m_canvas->canPromoteToTwoFinger()) {
        m_fingerId = pressed[0]->id();
        m_fingerId2 = pressed[1]->id();
        m_twoFinger = m_canvas->beginTwoFingerTouch(canvasOf(*pressed[0]), canvasOf(*pressed[1]));
        return true;
    }
    const QEventPoint *primary = findPointId(pts, m_fingerId);
    if (!primary && !pressed.isEmpty())
        primary = pressed[0];
    if (m_fingerId < 0 || !primary)
        return true;
    m_canvas->updateFingerTouch(canvasOf(*primary), pressed.size());
    return true;
}

bool TabletGestures::onTouchEnd(QTouchEvent *touch)
{
    const auto pts = touch->points();
    auto canvasOf = [this](const QEventPoint &tp) {
        return m_canvas->mapFromGlobal(tp.globalPosition());
    };
    if (m_twoFinger) {
        m_canvas->endTwoFingerTouch();
    } else if (!m_ignoreUntilUp && m_fingerId >= 0) {
        const QEventPoint *released = findPointId(pts, m_fingerId);
        if (!released) {
            for (const QEventPoint &tp : pts) {
                if (tp.state() == QEventPoint::State::Released) {
                    released = &tp;
                    break;
                }
            }
        }
        if (released)
            m_canvas->endFingerTouch(canvasOf(*released));
    }
    resetFingers();
    return true;
}

bool TabletGestures::handleLeftoverTouch(QTouchEvent *touch)
{
    if (!m_canvas)
        return false;
    ++m_touchEventCount;
    switch (touch->type()) {
    case QEvent::TouchBegin:
        return onTouchBegin(touch);
    case QEvent::TouchUpdate:
        return onTouchUpdate(touch);
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        return onTouchEnd(touch);
    default:
        return false;
    }
}

bool TabletGestures::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel: {
        if (!m_touchReachableLogged) {
            m_touchReachableLogged = true;
            qInfo() << "[STORY-EP-004] capacitive touch REACHABLE via TabletGestures";
        }
        auto *touch = static_cast<QTouchEvent *>(event);
        int n = 0;
        for (const QEventPoint &tp : touch->points()) {
            if (tp.state() != QEventPoint::State::Released)
                ++n;
        }
        if (epaper::handtouch::palmByContactCount(n)) {
            suppressCanvasTouch();
            return true;
        }
        return false;
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
            notePenNear(tablet->pressure() > 0.01);
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