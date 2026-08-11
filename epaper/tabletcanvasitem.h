#pragma once

#include <QQuickPaintedItem>
#include <QVector>
#include <QPointF>
#include <QEvent>
#include <QAtomicInt>
#include <QElapsedTimer>
#include <QImage>
#include <QRectF>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

class StrokeSync;

/**
 * Pen ink + ADR-0009 viewport / vector document rasterize for the sync region.
 * @implements [SRS-EP-01]
 * @implements [SRS-EP-02] vector ∩ drawingRegion paint (no bitmap push)
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
        QPointF raw;
    };

    struct WorldAabb {
        double minX = 0;
        double minY = 0;
        double maxX = 0;
        double maxY = 0;
        bool valid = false;
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
    void onHostMessage(const QJsonObject &obj);
    void applyViewport(const QJsonObject &obj);
    void applyDocSnapshot(const QJsonObject &obj);
    void scheduleVectorRasterize(bool sharp);
    void rasterizeVectors(bool sharp);
    QPointF worldToPanel(double wx, double wy) const;
    QPointF panelToWorld(const QPointF &panel) const;
    void appendLocalStrokeAsWorldPath();
    void drawVectorNode(QPainter &p, const QJsonObject &node);
    double panelScale() const;
    qreal worldStrokeWidth(qreal pressure) const;
    void panelToFrameUv(double localX, double localY, double *u, double *v) const;
    void frameUvToPanel(double u, double v, double *x, double *y) const;
    bool orientationLandscape() const;
    bool orientationInvertX() const;
    bool orientationInvertY() const;

    StrokeSync *m_sync = nullptr;
    QImage m_image;
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
    /** Reawa-style gut pose; legacy "portrait"/"landscape" normalized on ingest. */
    QString m_orientation = QStringLiteral("gutToLeft");
    qreal m_activeWorldStrokeWidth = 2.5;
    int m_viewportSeq = 0;
    WorldAabb m_drawingRegion;
    QJsonArray m_vectorNodes;
    bool m_rasterizePending = false;
    bool m_rasterizeSharp = false;
    QPointF m_lastRaw;
    QElapsedTimer m_flushClock;
    QElapsedTimer m_refreshClock;
    QRectF m_pendingDirty;
    static constexpr qint64 kFlushIntervalMs = 8;
    static constexpr qint64 kRefreshMinIntervalMs = 250;
    static constexpr qreal kBaseWorldStroke = 2.5;
};
