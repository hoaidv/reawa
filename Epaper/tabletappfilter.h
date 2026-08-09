#pragma once

#include <QObject>
#include <QEvent>

class TabletCanvasItem;

class TabletAppFilter : public QObject
{
    Q_OBJECT

public:
    explicit TabletAppFilter(QObject *parent = nullptr);

    void setCanvas(TabletCanvasItem *canvas) { m_canvas = canvas; }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    TabletCanvasItem *m_canvas = nullptr;
};
