#include "canvaspointeritem.h"
#include "tabletgestures.h"

#include <QTouchEvent>

CanvasPointerItem::CanvasPointerItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    setAcceptHoverEvents(false);
}

void CanvasPointerItem::setGestures(TabletGestures *g)
{
    if (m_gestures == g)
        return;
    m_gestures = g;
}

void CanvasPointerItem::touchEvent(QTouchEvent *event)
{
    if (m_gestures && m_gestures->handleLeftoverTouch(event))
        event->accept();
    else
        event->ignore();
}
