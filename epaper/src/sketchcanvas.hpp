#pragma once
#include <QQuickPaintedItem>
#include <QPainter>
#include <QtQml/qqmlregistration.h>
#include <QElapsedTimer>


constexpr qreal kPenWidth = 4;
constexpr qreal kPad = kPenWidth * 0.5 + 8;
constexpr qint64 kFlushMs = 8;

class SketchCanvas: public QQuickPaintedItem {

    Q_OBJECT
    QML_ELEMENT 

public:

    explicit SketchCanvas(QQuickItem * parent = nullptr);
    void paint(QPainter *painter) override;
    Q_INVOKABLE void beginStroke(qreal x, qreal y);
    Q_INVOKABLE void extendStroke(qreal x, qreal y);
    Q_INVOKABLE void endStroke();
    Q_INVOKABLE void clear();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;



private:

    void ensureImage();
    QRect dirtyFor(const QPointF &a, const QPointF &b, const QRect &bounds);
    void noteDirty(const QRect &r);

    // Coalesce ~8 ms of samples, then damage only that rect.
    // The pen-mode region selects the waveform; this path does not call swapPen.
    void flush() {
        if (m_pendingFlush.isNull())
            return;
        const QRect r = m_pendingFlush;
        m_pendingFlush = QRect();
        m_pendingFlushTimer.restart();
        update(r);
    }

    QVector<QVector<QPointF>> m_strokes;
    QImage m_image;
    QRect m_pendingFlush;
    QElapsedTimer m_pendingFlushTimer;

};

