#pragma once
#include <QQuickPaintedItem>
#include <QPainter>
#include <QtQml/qqmlregistration.h>


class SketchCanvas: public QQuickPaintedItem {

    Q_OBJECT
    QML_ELEMENT 

public:

    explicit SketchCanvas(QQuickItem * parent = nullptr);
    void paint(QPainter *painter) override;
    Q_INVOKABLE void beginStroke(qreal x, qreal y);
    Q_INVOKABLE void extendStroke(qreal x, qreal y);

    Q_INVOKABLE void clear();

private:
    QVector<QVector<QPointF>> m_strokes;

};

