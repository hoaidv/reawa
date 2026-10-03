#pragma once
#include <QQuickPaintedItem>
#include <QPainter>
#include <QtQml/qqmlregistration.h>


constexpr qreal kPenWidth = 4;
constexpr qreal kPad = kPenWidth * 0.5 + 8;

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

    void ensureImage();
    QRect dirtyFor(const QPointF &a, const QPointF &b, const QRect &bounds);

    QVector<QVector<QPointF>> m_strokes;
    QImage m_image;

};

