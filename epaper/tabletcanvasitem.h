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
#include <cstdint>
#include <string>
#include <vector>

#include "document/device_document.hpp"

class StrokeSync;

/**
 * Pen ink + device document rasterize for the sync region.
 * @implements [SRS-EP-01]
 * @implements [SRS-EP-02] vector ∩ drawingRegion paint (no bitmap push)
 * @implements [SRS-EP-07] local tree paint + stroke ingest
 * @implements [SRS-EP-09] digitizer channels on Ink samples
 */
class TabletCanvasItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(int strokeCount READ strokeCount NOTIFY strokeCountChanged)
    Q_PROPERTY(bool paintsInk READ paintsInk CONSTANT)
    Q_PROPERTY(QPointF lastPoint READ lastPoint NOTIFY debugChanged)
    Q_PROPERTY(QString debugInfo READ debugInfo NOTIFY debugChanged)
    /** Device-local tool: pen | ink_box | selection — never synced (SRS-EP-04). */
    Q_PROPERTY(QString toolMode READ toolMode WRITE setToolMode NOTIFY toolModeChanged)
    Q_PROPERTY(QRectF toolChipRect READ toolChipRect NOTIFY toolChipRectChanged)
    Q_PROPERTY(int pickableCount READ pickableCount NOTIFY pickablesChanged)

public:
    explicit TabletCanvasItem(QQuickItem *parent = nullptr);

    int strokeCount() const { return m_strokeCount; }
    bool paintsInk() const { return m_paintsInk; }
    QPointF lastPoint() const { return m_lastPoint; }
    QString debugInfo() const { return m_debugInfo; }
    QString toolMode() const { return m_toolMode; }
    void setToolMode(const QString &mode);
    QRectF toolChipRect() const { return m_toolChipRect; }
    int pickableCount() const { return m_pickables.size(); }

    /** Digitizer channels reported on this sample (SRS-EP-09). Unset = not reported. */
    struct IngestChannels {
        qreal pressure = 0;
        bool hasTilt = false;
        qreal tiltX = 0;
        qreal tiltY = 0;
        bool hasDistance = false;
        qreal distance = 0;
        bool hasTimestamp = false;
        qreal timestamp = 0;
        bool hasRotation = false;
        qreal rotation = 0;
        bool hasTangential = false;
        qreal tangential = 0;
    };

    Q_INVOKABLE void ingestPoint(QEvent::Type type, const QPointF &pos, qreal pressure);
    void ingestPoint(QEvent::Type type, const QPointF &pos, const IngestChannels &ch);
    Q_INVOKABLE void armTool(const QString &mode);
    int documentInkCount() const;
    std::string ingestDumpText() const;
    /** @implements [SRS-EP-04] finger/pen tap on ToolChip tile */
    bool tryArmToolAtCanvasPos(const QPointF &canvasPos);

    void paint(QPainter *painter) override;

signals:
    void strokeCountChanged();
    void debugChanged();
    void toolModeChanged();
    void toolChipRectChanged();
    void pickablesChanged();
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
        bool hasTilt = false;
        qreal tiltX = 0;
        qreal tiltY = 0;
        bool hasDistance = false;
        qreal distance = 0;
        bool hasTimestamp = false;
        qreal timestamp = 0;
        bool hasRotation = false;
        qreal rotation = 0;
        bool hasTangential = false;
        qreal tangential = 0;
    };

    struct WorldAabb {
        double minX = 0;
        double minY = 0;
        double maxX = 0;
        double maxY = 0;
        bool valid = false;
    };

    QPointF mapInputToCanvas(const QPointF &raw) const;
    Point makePoint(const QPointF &canvasPos, const IngestChannels &ch) const;
    void beginStroke(const QPointF &canvasPos, const IngestChannels &ch);
    void appendPoint(const QPointF &canvasPos, const IngestChannels &ch);
    void endStroke();
    void ingestCurrentStroke();
    void flushPending();
    void emitSegment(const Point &from, const Point &to);
    void ensureImage();
    void paintSegment(const Point &from, const Point &to, qreal lineWidth);
    void stampStaticBeacon();
    void stampFlushBeacon();
    void syncBegin();
    void syncPoint(const Point &pt);
    void syncEnd();
    void syncToolIntent(const QJsonObject &obj);
    void onHostMessage(const QJsonObject &obj);
    void applyViewport(const QJsonObject &obj);
    void applyDocSnapshot(const QJsonObject &obj);
    void updateToolChipRect();
    bool pointInToolChip(const QPointF &canvasPos) const;
    QString toolModeAtChipPos(const QPointF &canvasPos) const;
    QString hitPickable(const QPointF &world) const;
    void beginSelectionGesture(const QPointF &canvasPos);
    void updateSelectionGesture(const QPointF &canvasPos);
    void endSelectionGesture();
    /** @implements [SRS-EP-04] local selection bounds + move ghost (not baked into ink) */
    void paintSelectionChrome(QPainter *painter) const;
    QRectF pickablePanelRect(const QString &id, double dxWorld = 0, double dyWorld = 0) const;
    void scheduleVectorRasterize(bool sharp);
    void rasterizeVectors(bool sharp);
    QPointF worldToPanel(double wx, double wy) const;
    QPointF panelToWorld(const QPointF &panel) const;
    void drawDocNode(QPainter &p, const epaper::document::DocNode &node);
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
    epaper::document::DeviceDocument m_document;
    std::vector<std::int64_t> m_ingestNs;
    int m_ingestApplied = 0;
    int m_ingestRejected = 0;
    bool m_loggedRetiredSnapshot = false;
    QJsonArray m_pickables;
    QString m_toolMode = QStringLiteral("pen");
    QRectF m_toolChipRect;
    QString m_selectedPickableId;
    QString m_gesturePickableId;
    QPointF m_gestureStartWorld;
    QPointF m_gestureLastWorld;
    bool m_selectionGesture = false;
    /** Last panel-space chrome rect — dirty region for soft update during drag. */
    QRectF m_selectionChromeDirty;
    int m_toolIntentSeq = 0;
    QElapsedTimer m_selectionGhostClock;
    static constexpr qint64 kSelectionGhostMinIntervalMs = 50;
    bool m_rasterizePending = false;
    bool m_rasterizeSharp = false;
    /** Deferred sharp refresh queued while a stroke was in flight. */
    bool m_rasterizeDeferredSharp = false;
    int m_settleFollowUpToken = 0;
    QPointF m_lastRaw;
    QElapsedTimer m_flushClock;
    QElapsedTimer m_refreshClock;
    QRectF m_pendingDirty;
    static constexpr qint64 kFlushIntervalMs = 8;
    static constexpr qint64 kRefreshMinIntervalMs = 250;
    /** Single deferred settle pass — keep light; avoid swap storms. */
    static constexpr qint64 kSettleFollowUpMs = 180;
    static constexpr qreal kBaseWorldStroke = 2.5;
};
