#pragma once

#include <QObject>
#include <QEvent>
#include <QElapsedTimer>
#include <QPointF>
#include <QTimer>

#include "tabletcanvasitem.h"

class QTabletEvent;
class QTouchEvent;
class QWindow;

/**
 * App filter + leftover canvas gestures after Qt chrome hit-test.
 * @implements [SRS-EP-04] tool input routing
 * @implements [SRS-EP-09] digitizer channels on leftover pen
 * @implements [SRS-EP-21] one-finger leftover
 * @implements [SRS-EP-22] palm contacts
 * @implements [SRS-EP-24] two-finger leftover
 */
class TabletGestures : public QObject
{
    Q_OBJECT

public:
    explicit TabletGestures(QObject *parent = nullptr);

    void setCanvas(TabletCanvasItem *canvas);

    bool handleLeftoverTablet(QTabletEvent *tablet);
    bool handleLeftoverTouch(QTouchEvent *touch);

    int touchEventCount() const { return m_touchEventCount; }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void notePenNear(bool contact);
    void notePenLeave();
    void suppressCanvasTouch();
    bool canvasHandTouchOn() const;
    bool injectMappedTabletIfWindow(QObject *watched, QTabletEvent *tablet);
    TabletCanvasItem::IngestChannels channelsFrom(const QTabletEvent *tablet) const;
    bool onTouchBegin(QTouchEvent *touch);
    bool onTouchUpdate(QTouchEvent *touch);
    bool onTouchEnd(QTouchEvent *touch);
    void resetFingers();

    TabletCanvasItem *m_canvas = nullptr;
    QPointF m_tabletRaw;
    int m_touchEventCount = 0;
    bool m_touchReachableLogged = false;
    int m_fingerId = -1;
    int m_fingerId2 = -1;
    bool m_twoFinger = false;
    bool m_ignoreUntilUp = false;
    bool m_penNear = false;
    bool m_penDown = false;
    int m_lastHandLogCount = -1;
    QElapsedTimer m_handLogClock;
    QTimer *m_penIdle = nullptr;
    bool m_injectingMapped = false;
};