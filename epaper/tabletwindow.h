#pragma once

#include <QQuickWindow>
#include <QTabletEvent>

class TabletCanvasItem;

/**
 * Fullscreen Quick window that owns the ink canvas pointer for the app filter.
 * @implements [SRS-EP-01]
 */
class TabletWindow : public QQuickWindow
{
    Q_OBJECT
    Q_PROPERTY(TabletCanvasItem *canvas READ canvas WRITE setCanvas NOTIFY canvasChanged)

public:
    explicit TabletWindow(QWindow *parent = nullptr);

    TabletCanvasItem *canvas() const { return m_canvas; }

public slots:
    void setCanvas(TabletCanvasItem *canvas);

signals:
    void canvasChanged(TabletCanvasItem *canvas);

private:
    TabletCanvasItem *m_canvas = nullptr;
};
