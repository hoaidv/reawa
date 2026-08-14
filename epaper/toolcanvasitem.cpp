#include "toolcanvasitem.h"
#include "tabletcanvasitem.h"
#include "epaperbridge.h"

#include <QPainter>
#include <QDebug>

ToolCanvasItem::ToolCanvasItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(false);
    setRenderTarget(QQuickPaintedItem::Image);
    setOpaquePainting(false);
    setFillColor(Qt::transparent);
    setAcceptedMouseButtons(Qt::NoButton);
    setAcceptTouchEvents(false);
    setAcceptHoverEvents(false);
    setVisible(false);
}

void ToolCanvasItem::setCanvas(TabletCanvasItem *c)
{
    if (m_canvas == c)
        return;
    m_canvas = c;
    if (m_canvas)
        m_canvas->bindToolCanvas(this);
    emit canvasChanged();
    update();
}

void ToolCanvasItem::componentComplete()
{
    QQuickPaintedItem::componentComplete();
    if (!EpaperBridge::instance()->attachMonoModeRegion(this)) {
        qInfo() << "[tool-canvas] Mono attach failed — tight bbox fallback (ADR-0019)";
    }
}

void ToolCanvasItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        EpaperBridge::instance()->attachMonoModeRegion(this);
}

void ToolCanvasItem::paint(QPainter *painter)
{
    if (!m_canvas)
        return;
    m_canvas->paintToolChrome(painter);
}
