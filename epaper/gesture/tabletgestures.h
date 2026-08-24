#pragma once

#include <QObject>
#include <QEvent>
#include <QTimer>

#include "tabletcanvasitem.h"

class QTabletEvent;
class QWindow;

/**
 * Thin app filter: remap tablet, stash digitizer channels, eat mouse synthesis,
 * cancel hand-touch while the pen is near. Canvas input is Qt PointHandler /
 * PinchHandler — this filter does not leftover-ingest.
 * @implements [SRS-EP-04] Qt pointer routing
 * @implements [SRS-EP-09] digitizer channels stashed for PointHandler
 * @implements [SRS-EP-21] pen-near cancels one-finger
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
    bool injectMappedTabletIfWindow(QObject *watched, QTabletEvent *tablet);
    TabletCanvasItem::IngestChannels channelsFrom(const QTabletEvent *tablet) const;

    TabletCanvasItem *m_canvas = nullptr;
    bool m_penNear = false;
    bool m_penDown = false;
    QTimer *m_penIdle = nullptr;
    bool m_injectingMapped = false;
};
