#include "tabletappfilter.h"
#include "tabletcanvasitem.h"
#include "document/hand_touch.hpp"

#include <QTabletEvent>
#include <QTouchEvent>
#include <QEventPoint>
#include <QPointingDevice>
#include <QDebug>
#include <QList>
#include <QVector>
#include <QTimer>
#include <QElapsedTimer>

TabletAppFilter::TabletAppFilter(QObject *parent)
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

void TabletAppFilter::notePenNear(bool contact)
{
    m_penNear = true;
    if (contact)
        m_penDown = true;
    suppressCanvasTouch();
    m_penIdle->start();
}

void TabletAppFilter::notePenLeave()
{
    m_penNear = false;
    m_penDown = false;
    m_penIdle->stop();
}

void TabletAppFilter::suppressCanvasTouch()
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

bool TabletAppFilter::canvasHandTouchOn() const
{
    const bool toggleOn = m_canvas && m_canvas->handTouchArmed();
    return epaper::handtouch::handTouchEnabled(m_penNear, m_penDown, toggleOn);
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
        // @implements [SRS-EP-21] one-finger box / empty palm / local pan
        // @implements [SRS-EP-22] hand-touch toggle and contact-count palm
        // @implements [SRS-EP-24] two-finger pan pinch; block during box-move
        if (m_canvas) {
            auto *touch = static_cast<QTouchEvent *>(event);
            const auto pts = touch->points();
            QVector<const QEventPoint *> pressed;
            pressed.reserve(pts.size());
            for (const QEventPoint &tp : pts) {
                if (tp.state() == QEventPoint::State::Released)
                    continue;
                pressed.append(&tp);
            }
            auto canvasOf = [this](const QEventPoint &tp) {
                return m_canvas->mapFromGlobal(tp.globalPosition());
            };
            auto resetFingers = [this]() {
                m_fingerId = -1;
                m_fingerId2 = -1;
                m_twoFinger = false;
                m_ignoreUntilUp = false;
                m_qmlOwnsTouch = false;
                m_lastHandLogCount = -1;
            };
            auto logPhase = [&](const char *phase, const QVector<const QEventPoint *> &set,
                                bool force) {
                const int n = set.size();
                const bool countChanged = n != m_lastHandLogCount;
                const bool palmBurst = epaper::handtouch::palmByContactCount(n)
                    && (!m_handLogClock.isValid() || m_handLogClock.elapsed() >= 80);
                if (!force && !countChanged && !palmBurst)
                    return;
                qInfo().noquote() << handTouchLogLine(phase, set, m_canvas);
                m_lastHandLogCount = n;
                m_handLogClock.restart();
            };

            if (event->type() == QEvent::TouchBegin) {
                logPhase("begin", pressed, true);
                if (m_ignoreUntilUp)
                    return true;
                if (!pressed.isEmpty()) {
                    const QPointF pos0 = canvasOf(*pressed[0]);
                    if (m_canvas->debugLogVisible() && m_canvas->debugLogRect().contains(pos0)
                        && !m_canvas->isChromeHit(pos0)) {
                        m_qmlOwnsTouch = true;
                        return false;
                    }
                    const bool chrome = m_canvas->isChromeHit(pos0);
                    if (!canvasHandTouchOn() && !chrome)
                        return true;
                    if (epaper::handtouch::palmByContactCount(pressed.size()) && !chrome) {
                        suppressCanvasTouch();
                        return true;
                    }
                    if (chrome) {
                        m_fingerId = pressed[0]->id();
                        m_fingerId2 = -1;
                        m_twoFinger = false;
                        m_canvas->beginFingerTouch(pos0);
                        return true;
                    }
                }
                if (pressed.size() == 2) {
                    m_fingerId = pressed[0]->id();
                    m_fingerId2 = pressed[1]->id();
                    m_twoFinger = m_canvas->beginTwoFingerTouch(canvasOf(*pressed[0]),
                                                               canvasOf(*pressed[1]));
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
            if (event->type() == QEvent::TouchUpdate) {
                logPhase("update", pressed, false);
                if (m_qmlOwnsTouch)
                    return false;
                if (m_ignoreUntilUp)
                    return true;
                if (epaper::handtouch::palmByContactCount(pressed.size())) {
                    suppressCanvasTouch();
                    return true;
                }
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
                    if (!a)
                        a = pressed[0];
                    if (!b)
                        b = pressed[1];
                    if (a && b)
                        m_canvas->updateTwoFingerTouch(canvasOf(*a), canvasOf(*b));
                    return true;
                }
                if (pressed.size() == 2 && canvasHandTouchOn()
                    && m_canvas->canPromoteToTwoFinger()) {
                    m_fingerId = pressed[0]->id();
                    m_fingerId2 = pressed[1]->id();
                    m_twoFinger = m_canvas->beginTwoFingerTouch(canvasOf(*pressed[0]),
                                                               canvasOf(*pressed[1]));
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
            if (event->type() == QEvent::TouchEnd || event->type() == QEvent::TouchCancel) {
                QVector<const QEventPoint *> ending;
                ending.reserve(pts.size());
                for (const QEventPoint &tp : pts)
                    ending.append(&tp);
                logPhase("end", ending, true);
                if (m_qmlOwnsTouch) {
                    resetFingers();
                    return false;
                }
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
        } else
            notePenNear(tablet->pressure() > 0.01);
        TabletCanvasItem::IngestChannels ch;
        // @implements [SRS-EP-09] retain digitizer-reported channels on the node
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
        m_canvas->ingestPoint(event->type(), tablet->position(), ch);
        return true;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        // @fix [STORY-EP-033] tablet-only ingest. Mouse is a different coordinate
        // space; mapping it through mapInputToCanvas paints from panel bottom-left.
        // Qt often synthesizes MouseButtonPress before TabletPress. Chrome hit-test
        // is C++ on the tablet path (applyContactPress).
        return true;
    default:
        return false;
    }
}
