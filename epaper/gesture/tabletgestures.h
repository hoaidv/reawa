#pragma once

#include <QObject>
#include <QEvent>
#include <QTimer>

#include "tabletcanvasitem.h"

class QTabletEvent;
class QTouchEvent;
class QWindow;

/**
 * Thin app filter: remap tablet and ingest pen in C++ (tablet rate is too high
 * for QML PointHandler). One-finger capacitive is PointHandler; two-finger is
 * here so the first contact is not exclusive-grabbed away from a pinch.
 * @implements [SRS-EP-04] Qt pointer routing
 * @implements [SRS-EP-09] digitizer channels on pen ingest
 * @implements [SRS-EP-21] pen-near cancels one-finger
 * @implements [SRS-EP-24] two-finger pan pinch
 */
class TabletGestures : public QObject
{
    Q_OBJECT

public:
    explicit TabletGestures(QObject *parent = nullptr);

    void setCanvas(TabletCanvasItem *canvas);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void notePenNear(bool contact);
    void notePenLeave();
    void suppressCanvasTouch();
    bool ingestPen(QObject *watched, QTabletEvent *tablet);
    bool injectMapped(QObject *watched, QWindow *w, QTabletEvent *tablet, const QPointF &mapped);
    bool handleTwoFinger(QTouchEvent *touch);
    TabletCanvasItem::IngestChannels channelsFrom(const QTabletEvent *tablet) const;

    TabletCanvasItem *m_canvas = nullptr;
    bool m_penNear = false;
    bool m_penDown = false;
    bool m_penOnCanvas = false;
    bool m_twoFinger = false;
    bool m_injectingMapped = false;
    QTimer *m_penIdle = nullptr;
};
