#include "tabletwindow.h"
#include "tabletcanvasitem.h"

TabletWindow::TabletWindow(QWindow *parent)
    : QQuickWindow(parent)
{
}

void TabletWindow::setCanvas(TabletCanvasItem *canvas)
{
    if (m_canvas == canvas)
        return;
    m_canvas = canvas;
    emit canvasChanged(m_canvas);
}
