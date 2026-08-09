#pragma once

#include <QQuickWindow>
#include <QTabletEvent>

class TabletCanvasItem;

class TabletWindow : public QQuickWindow
{
    Q_OBJECT
    Q_PROPERTY(TabletCanvasItem *canvas READ canvas WRITE setCanvas NOTIFY canvasChanged)
    Q_PROPERTY(int debugEvents READ debugEvents NOTIFY debugEventsChanged)

public:
    explicit TabletWindow(QWindow *parent = nullptr);

    TabletCanvasItem *canvas() const { return m_canvas; }
    int debugEvents() const { return m_debugEvents; }

public slots:
    void setCanvas(TabletCanvasItem *canvas);

signals:
    void canvasChanged(TabletCanvasItem *canvas);
    void debugEventsChanged();

protected:
    void tabletEvent(QTabletEvent *event) override;
    bool event(QEvent *event) override;

private:
    void forwardPoint(QEvent::Type type, const QPointF &pos, qreal pressure);

    TabletCanvasItem *m_canvas = nullptr;
    int m_debugEvents = 0;
};
