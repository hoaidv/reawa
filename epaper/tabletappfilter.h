#pragma once

#include <QObject>
#include <QEvent>
#include <QTimer>

class TabletCanvasItem;

/**
 * App-wide event filter: pen → canvas; touch → ToolChip + one/two-finger canvas.
 * @implements [SRS-EP-04] tool input routing
 * @implements [SRS-EP-21] one-finger canvas hit routing
 * @implements [SRS-EP-24] two-finger pan pinch routing
 */
class TabletAppFilter : public QObject
{
    Q_OBJECT

public:
    explicit TabletAppFilter(QObject *parent = nullptr);

    void setCanvas(TabletCanvasItem *canvas) { m_canvas = canvas; }

    /** STORY-EP-004: number of QTouch* events observed since launch. */
    int touchEventCount() const { return m_touchEventCount; }
    bool touchReachableLogged() const { return m_touchReachableLogged; }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void notePenNear(bool contact);
    void notePenLeave();
    void suppressCanvasTouch();
    bool canvasHandTouchOn() const;

    TabletCanvasItem *m_canvas = nullptr;
    int m_touchEventCount = 0;
    bool m_touchReachableLogged = false;
    int m_fingerId = -1;
    int m_fingerId2 = -1;
    bool m_twoFinger = false;
    bool m_ignoreUntilUp = false;
    bool m_qmlOwnsTouch = false;
    bool m_penNear = false;
    bool m_penDown = false;
    QTimer *m_penIdle = nullptr;
};
