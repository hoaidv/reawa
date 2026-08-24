#pragma once

#include <QObject>
#include <QEvent>
#include <QTimer>

#include "tabletcanvasitem.h"

class QTabletEvent;
class QTouchEvent;
class QWindow;

/**
 * Thin app filter. It only fixes what Qt cannot know: the RM digitizer needs a
 * panel remap, the extra pen channels need parking, tablet→mouse synthesis must
 * not double a gesture, and a near pen outranks hand touch. Hit-testing, grabs
 * and gesture recognition all stay with Qt's handlers.
 * @implements [SRS-EP-04] Qt pointer routing
 * @implements [SRS-EP-09] digitizer channels on pen ingest
 * @implements [SRS-EP-21] pen-near cancels hand touch
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
    void noteContacts(QTouchEvent *touch);
    bool remapPen(QObject *watched, QTabletEvent *tablet);
    bool injectMapped(QObject *watched, QWindow *w, QTabletEvent *tablet, const QPointF &mapped);
    TabletCanvasItem::IngestChannels channelsFrom(const QTabletEvent *tablet) const;

    TabletCanvasItem *m_canvas = nullptr;
    bool m_penNear = false;
    bool m_penDown = false;
    bool m_injectingMapped = false;
    int m_contacts = 0;
    QTimer *m_penIdle = nullptr;
};
