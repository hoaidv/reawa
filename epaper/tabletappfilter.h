#pragma once

#include <QObject>
#include <QEvent>

class TabletCanvasItem;

/**
 * App-wide event filter: pen → canvas; touch → ToolChip (STORY-EP-006).
 * @implements [SRS-EP-04] tool input routing
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
    TabletCanvasItem *m_canvas = nullptr;
    int m_touchEventCount = 0;
    bool m_touchReachableLogged = false;
};
