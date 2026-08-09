#pragma once

#include <QQuickPaintedItem>
#include <QVector>
#include <QPointF>
#include <QEvent>
#include <QAtomicInt>
#include <QElapsedTimer>
#include <QImage>
#include <QRectF>

class StrokeSync;

/**
 * Pen input plus the ink layer itself: strokes are rasterised into a persistent
 * QImage and blitted from paint(). Set RM_INK_MODE=pool to fall back to the QML
 * Rectangle pool (slower, but proven visible on this backend).
 * @implements [SRS-EP-01]
 */
class TabletCanvasItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(int strokeCount READ strokeCount NOTIFY strokeCountChanged)
    Q_PROPERTY(bool paintsInk READ paintsInk CONSTANT)
    Q_PROPERTY(QPointF lastPoint READ lastPoint NOTIFY debugChanged)
    Q_PROPERTY(QString debugInfo READ debugInfo NOTIFY debugChanged)

public:
    explicit TabletCanvasItem(QQuickItem *parent = nullptr);

    int strokeCount() const { return m_strokeCount; }
    bool paintsInk() const { return m_paintsInk; }
    QPointF lastPoint() const { return m_lastPoint; }
    QString debugInfo() const { return m_debugInfo; }

    Q_INVOKABLE void ingestPoint(QEvent::Type type, const QPointF &pos, qreal pressure);

    void paint(QPainter *painter) override;

signals:
    void strokeCountChanged();
    void debugChanged();
    void segmentDrawn(qreal x1, qreal y1, qreal x2, qreal y2, qreal lineWidth);

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void componentComplete() override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;

private:
    struct Point {
        QPointF pos;
        qreal pressure;
    };

    QPointF mapInputToCanvas(const QPointF &raw) const;
    void beginStroke(const QPointF &canvasPos, qreal pressure);
    void appendPoint(const QPointF &canvasPos, qreal pressure);
    void endStroke();
    void flushPending();
    void emitSegment(const Point &from, const Point &to);
    void ensureImage();
    void paintSegment(const Point &from, const Point &to, qreal lineWidth);
    void stampStaticBeacon();
    void stampFlushBeacon();
    void syncBegin();
    void syncPoint(const Point &pt);
    void syncEnd();

    StrokeSync *m_sync = nullptr;
    QImage m_image;
    /** Bumped on the scene-graph thread; read only for the on-screen counter. */
    QAtomicInt m_paintCount{0};
    QVector<Point> m_current;
    int m_strokeCount = 0;
    int m_strokeSeq = 0;
    int m_flushCount = 0;
    int m_updateNodeLogs = 0;
    QString m_activeStrokeId;
    QPointF m_lastPoint;
    QString m_debugInfo;

    Point m_lastEmitted{};
    bool m_paintsInk = true;
    bool m_beacons = true;
    bool m_hasEmitted = false;
    bool m_strokeActive = false;
    QElapsedTimer m_flushClock;
    QRectF m_pendingDirty;
    static constexpr qint64 kFlushIntervalMs = 8;
};
