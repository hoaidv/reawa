#pragma once

#include <QQuickPaintedItem>

class TabletCanvasItem;

/**
 * ToolCanvasLayer — marquee / lasso / settled AABB. Never blits the document.
 * @implements [SRS-EP-12] SelectionOverlay stroke chrome
 * @implements [ADR-0019] ToolCanvasLayer: Pen while lasso/marquee, Mono after pen-up
 */
class ToolCanvasItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(TabletCanvasItem *canvas READ canvas WRITE setCanvas NOTIFY canvasChanged)

public:
    explicit ToolCanvasItem(QQuickItem *parent = nullptr);

    TabletCanvasItem *canvas() const { return m_canvas; }
    void setCanvas(TabletCanvasItem *c);
    void setStrokeWaveform(bool penInFlight);
    void paint(QPainter *painter) override;

signals:
    void canvasChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    TabletCanvasItem *m_canvas = nullptr;
};
