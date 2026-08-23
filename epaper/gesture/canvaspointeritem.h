#pragma once

#include <QQuickItem>
#include <QTouchEvent>

class TabletGestures;

/**
 * Full-bleed leftover pointer target under ToolChip / Follow / DBG.
 * @implements [SRS-EP-04]
 * @implements [SRS-EP-21]
 */
class CanvasPointerItem : public QQuickItem
{
    Q_OBJECT

public:
    explicit CanvasPointerItem(QQuickItem *parent = nullptr);

    TabletGestures *gestures() const { return m_gestures; }
    void setGestures(TabletGestures *g);

protected:
    void touchEvent(QTouchEvent *event) override;

private:
    TabletGestures *m_gestures = nullptr;
};