#pragma once

#include <QObject>
#include <QEvent>
#include <QPointF>
#include <QTimer>

#include "pen_sample.hpp"

class QTabletEvent;
class QTouchEvent;
class QWindow;

/**
 * Raw input filter. It publishes the three facts Qt's pointer handlers cannot
 * report, and decides nothing:
 *   1. a pen sample, panel-mapped, with the channels a QML HandlerPoint drops;
 *   2. whether the pen is near or down;
 *   3. how many contacts are live (QPointerEvent::pointCount has no QML peer).
 * Hit-testing, grabs and gesture recognition stay with Qt's handlers; the policy
 * that reads these facts lives in Main.qml. Deliberately knows nothing about the
 * canvas — a filter that calls cancelHandTouch() has made a product decision two
 * layers below where that decision belongs.
 * @implements [SRS-EP-04] Qt pointer routing
 * @implements [SRS-EP-09] digitizer channels on pen ingest
 * @implements [SRS-EP-21] pen-near cancels hand touch
 */
class TabletGestures : public QObject
{
    Q_OBJECT
    /** Live contacts on the glass, 0 while the pen suppresses touch. */
    Q_PROPERTY(int contactCount READ contactCount NOTIFY contactCountChanged)
    /** Pen in proximity or in contact — hand touch must yield. */
    Q_PROPERTY(bool penNear READ penNear NOTIFY penNearChanged)

public:
    explicit TabletGestures(QObject *parent = nullptr);

    int contactCount() const { return m_contacts; }
    bool penNear() const { return m_penNear; }

signals:
    /** Emitted before the mapped tablet event is delivered, so the canvas has the
     *  full channel set by the time a handler calls back. */
    void penSample(const QPointF &raw, const epaper::input::PenSample &channels);
    void contactCountChanged();
    void penNearChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setPenNear(bool isNear);
    void setContacts(int live);
    void notePenLeave();
    void noteContacts(QTouchEvent *touch);
    bool remapPen(QObject *watched, QTabletEvent *tablet);
    bool injectMapped(QObject *watched, QWindow *w, QTabletEvent *tablet, const QPointF &mapped);
    epaper::input::PenSample channelsFrom(const QTabletEvent *tablet) const;

    bool m_penNear = false;
    bool m_penDown = false;
    bool m_injectingMapped = false;
    int m_contacts = 0;
    QTimer *m_penIdle = nullptr;
};
