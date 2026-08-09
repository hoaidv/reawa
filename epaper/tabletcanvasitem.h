#pragma once

#include <QQuickPaintedItem>
#include <QPixmap>
#include <QVector>
#include <QPointF>
#include <QEvent>

class StrokeSync;

class TabletCanvasItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(int strokeCount READ strokeCount NOTIFY strokeCountChanged)
    Q_PROPERTY(QPointF lastPoint READ lastPoint NOTIFY debugChanged)
    Q_PROPERTY(QString debugInfo READ debugInfo NOTIFY debugChanged)

public:
    static constexpr int kPenXMax = 20967;
    static constexpr int kPenYMax = 15725;

    explicit TabletCanvasItem(QQuickItem *parent = nullptr);

    int strokeCount() const { return m_strokeCount; }
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

private:
    struct Point {
        QPointF pos;
        qreal pressure;
    };

    QPointF mapInputToCanvas(const QPointF &raw) const;
    void ensurePixmap();
    void drawSegment(const Point &from, const Point &to);
    void setDebug(const QPointF &raw, const QPointF &mapped);
    void beginStroke(const QPointF &canvasPos, qreal pressure);
    void appendPoint(const QPointF &canvasPos, qreal pressure);
    void endStroke();
    void syncBegin();
    void syncPoint(const Point &pt);
    void syncEnd();
    void forceRefresh();

    QPixmap m_pixmap;
    StrokeSync *m_sync = nullptr;
    QVector<QVector<Point>> m_strokes;
    QVector<Point> m_current;
    int m_strokeCount = 0;
    int m_strokeSeq = 0;
    QString m_activeStrokeId;
    QPointF m_lastPoint;
    QString m_debugInfo;
    QPointF m_lastEmitted;
    bool m_hasEmitted = false;
};
