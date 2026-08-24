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
#include <QStringList>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

#include "document/device_document.hpp"
#include "document/hand_touch.hpp"
#include "document/manipulate.hpp"
#include "document/one_way_sync.hpp"
#include "document/viewport_follow.hpp"
#include "gesture/manipdrag.h"
#include "toolchip_layout.hpp"

class StrokeSync;
class ToolCanvasItem;

/**
 * Pen ink + device document rasterize for the sync region.
 * @implements [SRS-EP-01]
 * @implements [SRS-EP-02] vector ∩ drawingRegion paint (no bitmap push)
 * @implements [SRS-EP-07] local tree paint + stroke ingest
 * @implements [SRS-EP-08] one-way sync handshake and publish
 * @implements [SRS-EP-11] live SmartGroup manipulation
 * @implements [SRS-EP-21] one-finger pick move palm pan
 * @implements [SRS-EP-23] finger exclusive-tool switch
 * @implements [SRS-EP-24] two-finger pan pinch viewport
 * @implements [SRS-EP-25] one-finger hand-touch quality
 * @implements [SRS-EP-26] two-finger map-apply quality
 * @implements [SRS-EP-49] viewport-follow Infini session enum
 * @implements [SRS-EP-50] FollowToggle sibling of ToolChip
 * @implements [SRS-EP-51] follow exclusivity and map-apply quality
 */
class TabletCanvasItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(int strokeCount READ strokeCount NOTIFY strokeCountChanged)
    Q_PROPERTY(bool paintsInk READ paintsInk CONSTANT)
    Q_PROPERTY(QPointF lastPoint READ lastPoint NOTIFY debugChanged)
    Q_PROPERTY(QString debugInfo READ debugInfo NOTIFY debugChanged)
    /** Device-local exclusive tool: sel_rect | sel_freeform | pen — never synced (SRS-EP-04). */
    Q_PROPERTY(QString toolMode READ toolMode WRITE setToolMode NOTIFY toolModeChanged)
    Q_PROPERTY(bool recogInkBoxArmed READ recogInkBoxArmed NOTIFY recogChanged)
    Q_PROPERTY(bool recogConnectorArmed READ recogConnectorArmed NOTIFY recogChanged)
    Q_PROPERTY(bool recogTogglesDimmed READ recogTogglesDimmed NOTIFY toolModeChanged)
    Q_PROPERTY(QString lastStrokeLatch READ lastStrokeLatch NOTIFY lastStrokeLatchChanged)
    Q_PROPERTY(QRectF toolChipRect READ toolChipRect NOTIFY toolChipRectChanged)
    Q_PROPERTY(QRectF followToggleRect READ followToggleRect NOTIFY trailingChromeChanged)
    Q_PROPERTY(QRectF usbLinkRect READ usbLinkRect NOTIFY trailingChromeChanged)
    Q_PROPERTY(QRectF debugToggleRect READ debugToggleRect NOTIFY trailingChromeChanged)
    Q_PROPERTY(QRectF handTouchToggleRect READ handTouchToggleRect NOTIFY trailingChromeChanged)
    Q_PROPERTY(QRectF debugLogRect READ debugLogRect NOTIFY trailingChromeChanged)
    Q_PROPERTY(bool debugLogVisible READ debugLogVisible NOTIFY debugLogVisibleChanged)
    Q_PROPERTY(bool handTouchArmed READ handTouchArmed NOTIFY handTouchArmedChanged)
    Q_PROPERTY(QString followDirection READ followDirection NOTIFY followChanged)
    Q_PROPERTY(bool followPressed READ followPressed NOTIFY followChanged)
    Q_PROPERTY(bool followUnavailable READ followUnavailable NOTIFY followChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
    Q_PROPERTY(QRectF encloseCtaRect READ encloseCtaRect NOTIFY selectionChromeChanged)
    Q_PROPERTY(bool encloseVisible READ encloseVisible NOTIFY selectionChromeChanged)
    Q_PROPERTY(QString encloseRefuseReason READ encloseRefuseReason NOTIFY selectionChromeChanged)
    Q_PROPERTY(QString manipulationUnavailable READ manipulationUnavailable NOTIFY selectionChromeChanged)
    Q_PROPERTY(QRectF manipulationUnavailableRect READ manipulationUnavailableRect NOTIFY selectionChromeChanged)
    Q_PROPERTY(QRectF selectionBoundsRect READ selectionBoundsRect NOTIFY selectionChromeChanged)
    Q_PROPERTY(int handleCount READ handleCount NOTIFY selectionChromeChanged)
    Q_PROPERTY(qreal handleSize READ handleSize NOTIFY selectionChromeChanged)
    Q_PROPERTY(bool modeChipVisible READ modeChipVisible NOTIFY selectionChromeChanged)
    Q_PROPERTY(QString modeChipLabel READ modeChipLabel NOTIFY selectionChromeChanged)
    Q_PROPERTY(QRectF modeChipRect READ modeChipRectProp NOTIFY selectionChromeChanged)

public:
    explicit TabletCanvasItem(QQuickItem *parent = nullptr);

    int strokeCount() const { return m_strokeCount; }
    bool paintsInk() const { return m_paintsInk; }
    QPointF lastPoint() const { return m_lastPoint; }
    QString debugInfo() const { return m_debugInfo; }
    QString toolMode() const { return m_toolMode; }
    void setToolMode(const QString &mode);
    bool recogInkBoxArmed() const { return m_chip.recogInkBox; }
    bool recogConnectorArmed() const { return m_chip.recogConnector; }
    bool recogTogglesDimmed() const { return m_chip.recogDimmed(); }
    QString lastStrokeLatch() const { return m_lastStrokeLatch; }
    Q_INVOKABLE void toggleRecogInkBox();
    Q_INVOKABLE void toggleRecogConnector();
    QRectF toolChipRect() const { return m_toolChipRect; }
    QRectF followToggleRect() const { return m_followToggleRect; }
    QRectF usbLinkRect() const { return m_usbLinkRect; }
    QRectF debugToggleRect() const { return m_debugToggleRect; }
    QRectF handTouchToggleRect() const { return m_handTouchToggleRect; }
    QRectF debugLogRect() const { return m_debugLogRect; }
    bool debugLogVisible() const { return m_debugLogVisible; }
    bool handTouchArmed() const { return m_handTouchArmed; }
    Q_INVOKABLE void toggleDebugLog();
    Q_INVOKABLE void toggleHandTouch();
    bool followPressed() const { return m_follow.ariaPressed(); }
    bool followUnavailable() const { return m_follow.ariaDisabled(); }
    bool canUndo() const { return m_document.undoDepth() > 0; }
    bool canRedo() const { return m_document.redoDepth() > 0; }
    Q_INVOKABLE void requestUndo();
    Q_INVOKABLE void requestRedo();
    QRectF encloseCtaRect() const { return m_encloseCtaRect; }
    bool encloseVisible() const { return m_encloseVisible; }
    QString encloseRefuseReason() const { return m_encloseRefuseReason; }
    QString manipulationUnavailable() const { return m_manipUnavailable; }
    QRectF manipulationUnavailableRect() const { return m_manipUnavailableRect; }
    QRectF selectionBoundsRect() const { return m_selectionBoundsRect; }
    int handleCount() const { return m_handleCount; }
    qreal handleSize() const { return m_handleSize; }
    bool modeChipVisible() const { return m_modeChipVisible; }
    QString modeChipLabel() const { return m_modeChipLabel; }
    QRectF modeChipRectProp() const { return m_modeChipRect; }
    Q_INVOKABLE void encloseSelection();
    void bindToolCanvas(class ToolCanvasItem *overlay);
    /** @implements [SRS-EP-12] ToolCanvasLayer stroke chrome (no document blit) */
    void paintToolChrome(QPainter *painter);

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
    /**
     * Capacitive one-finger path (STORY-EP-038). Knob / box / empty palm vs pan.
     * @implements [SRS-EP-21] one-finger canvas hit
     */
    bool beginFingerTouch(const QPointF &canvasPos);
    void updateFingerTouch(const QPointF &canvasPos, int fingerCount);
    void endFingerTouch(const QPointF &canvasPos);
    /**
     * Two-finger local pan/pinch (STORY-EP-039). A one-finger manip already in
     * flight is reverted, not blocking: two contacts always mean navigate.
     * @implements [SRS-EP-24] two-finger canvas pan pinch
     */
    void abortFingerManip();
    bool beginTwoFingerTouch(const QPointF &a, const QPointF &b);
    void updateTwoFingerTouch(const QPointF &a, const QPointF &b);
    void endTwoFingerTouch();
    void cancelHandTouch();
    QString followDirection() const { return m_followDirection; }
    void setFollowDirection(const QString &dir);
    Q_INVOKABLE void tapFollowToggle();
    int viewportUpCount() const { return m_viewportUpCount; }

    void paint(QPainter *painter) override;

    QPointF mapInputToCanvas(const QPointF &raw) const;
    void ingestMappedTablet(QEvent::Type type, const QPointF &canvasPos,
        const QPointF &rawPos, const IngestChannels &ch);
    /** Panel (canvas item) → document world. */
    Q_INVOKABLE QPointF panelToWorld(const QPointF &panel) const;
    /** @implements [SRS-EP-11] handle drag in world space */
    void beginHandleDrag(int handleIndex, const QPointF &world);
    void updateHandleDrag(const QPointF &world);
    void endHandleDrag();
    Q_INVOKABLE void tapModeChip();
    /**
     * Qt DragHandler / PinchHandler canvas entry. Qt owns the hit-test and the
     * grab; these only carry an already-arbitrated point into the document.
     * @implements [SRS-EP-04] Qt pointer routing
     */
    Q_INVOKABLE void onPointerStart(qreal x, qreal y, qreal pressure, bool pen);
    Q_INVOKABLE void onPointerMove(qreal x, qreal y, qreal pressure, bool pen);
    Q_INVOKABLE void onPointerEnd(qreal x, qreal y, bool pen);
    Q_INVOKABLE void onPointerCancel();
    /** A finger pressed and released without travelling — select or deselect. */
    Q_INVOKABLE void onFingerTap(qreal x, qreal y);
    Q_INVOKABLE void onSecondContact();
    Q_INVOKABLE void onContactsCleared();
    Q_INVOKABLE void onPinchStart(qreal x, qreal y, qreal scale);
    Q_INVOKABLE void onPinchUpdate(qreal x, qreal y, qreal scale);
    Q_INVOKABLE void onPinchEnd();
    void stashTabletSample(const QPointF &raw, const IngestChannels &ch);

signals:
    void strokeCountChanged();
    void debugChanged();
    void toolModeChanged();
    void recogChanged();
    void lastStrokeLatchChanged();
    void toolChipRectChanged();
    void trailingChromeChanged();
    void debugLogVisibleChanged();
    void handTouchArmedChanged();
    void followChanged();
    void historyChanged();
    void pickablesChanged();
    void selectionChromeChanged();
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

    qreal ingestPanelHeight() const;
    Point makePoint(const QPointF &canvasPos, const IngestChannels &ch) const;
    void applyContactPress(const QPointF &canvasPos, const IngestChannels &ch);
    QPointF pinchArmPoint(qreal x, qreal y, qreal scale, bool positive) const;
    int handleIndexAtPanel(const QPointF &panel, double hitDu) const;
    bool tryBeginHandleAtPanel(const QPointF &panel, double hitDu);
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
    void flushOneWayWire();
    void sendManipPreviewToInfini();
    void onHostMessage(const QJsonObject &obj);
    void applyViewport(const QJsonObject &obj);
    void applyDocSnapshot(const QJsonObject &obj);
    void updateToolChipRect();
    void emitViewportFollow();
    void flushFollowOutbound();
    void applyFollowCamera();
    void cacheInfiniViewport(const QJsonObject &obj);
    bool isSelectionTool() const;
    void beginMarqueeOrLasso(const QPointF &canvasPos);
    void finishMarqueeOrLasso();
    void refreshSelectionChrome();
    QString hitLocalSmartGroup(const QPointF &world) const;
    QString hitPickable(const QPointF &world) const;
    void beginSelectionGesture(const QPointF &canvasPos);
    void startLiveManip(const epaper::document::DocNode *subject,
                        epaper::document::ResizeHandle handle, const QPointF &world);
    void applyDragWorld(const QPointF &world);
    bool fingerHitsBox(const QPointF &canvasPos) const;
    void ensureLocalDrawingRegion();
    void applyLocalFingerPan(const QPointF &canvasPos);
    void applyLocalTwoFinger(const QPointF &a, const QPointF &b);
    epaper::handtouch::TwoFingerContacts uvPair(const QPointF &a, const QPointF &b) const;
    void maybePublishLocalViewport(bool settle);
    epaper::handtouch::FollowDirection followEnum() const;
    void updateSelectionGesture(const QPointF &canvasPos);
    void endSelectionGesture();
    void redrawLiveManipRegion();
    void commitLiveManip();
    void damageToolChrome(const QRectF &next);
    void damageToolChromeSegment(const QRectF &seg);
    void syncToolCanvasPresence();
    void paintLiveManipOnToolCanvas(QPainter *painter);
    QRectF pickablePanelRect(const QString &id, double dxWorld = 0, double dyWorld = 0) const;
    void applyHistoryRestore(bool isUndo);
    void pruneSelectionAfterHistory();
    void notifyHistory();
    void scheduleVectorRasterize(bool sharp);
    void rasterizeVectors(bool sharp);
    void beginRecogWidthBlink(const std::vector<std::string> &inkIds);
    /** Returns true when the highlighted boundary set changed. */
    bool setMembershipHighlight(const std::vector<std::string> &boundaryInkIds);
    void clearMembershipHighlight();
    void collectSmartGroupInkIds(const epaper::document::DocNode &sg, bool boundaryOnly,
                                 std::vector<std::string> *out) const;
    QPointF worldToPanel(double wx, double wy) const;
    void drawDocNode(QPainter &p, const epaper::document::DocNode &node,
                     const epaper::document::DocNode *smartParent = nullptr);
    void drawTree(QPainter &p, const std::vector<epaper::document::DocNode> &nodes,
                  const epaper::document::DocNode *smartParent);
    void drawWarpedConnector(QPainter &p, const epaper::document::DocNode &conn);
    qreal connectorPanelStrokeWidth(const epaper::document::DocNode &conn) const;
    QRectF warpedConnectorPanelRect(const epaper::document::DocNode &conn) const;
    QRectF boundConnectorsPanelUnion(const std::string &sgId) const;
    void captureOriginConnectorPunches(const std::string &sgId);
    double panelScale() const;
    QRectF worldBoundsToPanel(const epaper::document::SmartBounds &wb) const;
    bool lodOkPanel(const epaper::document::SmartBounds &wb) const;
    bool viewportZoomedOut() const;
    void showManipUnavailable(const epaper::document::SmartBounds &wb);
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
    /** @fix [STORY-EP-033] wait for a non-origin sample after a stale Press. */
    bool m_awaitingPlausiblePress = false;
    /** True after stroke_begin went to Infini — origin starts must not preview. */
    bool m_strokePreviewSent = false;
    /** Reawa-style gut pose; legacy "portrait"/"landscape" normalized on ingest. */
    QString m_orientation = QStringLiteral("gutToLeft");
    qreal m_activeWorldStrokeWidth = 2.5;
    int m_viewportSeq = 0;
    WorldAabb m_drawingRegion;
    QString m_followDirection = QStringLiteral("none");
    epaper::follow::FollowSession m_follow;
    QRectF m_followToggleRect;
    QRectF m_usbLinkRect;
    QRectF m_debugToggleRect;
    QRectF m_handTouchToggleRect;
    QRectF m_debugLogRect;
    bool m_debugLogVisible = false;
    bool m_handTouchArmed = true;
    int m_viewportUpCount = 0;
    enum class FingerGesture { None, Chip, Move, Resize, EmptyPending, EmptyPan, TwoFinger } m_fingerGesture =
        FingerGesture::None;
    bool m_fingerLockedUntilLift = false;
    QPointF m_fingerDownPanel;
    QPointF m_fingerDownWorld;
    WorldAabb m_fingerPanOrigin;
    QElapsedTimer m_fingerPanClock;
    epaper::handtouch::TwoFingerContacts m_twoOriginContacts{};
    QPointF m_twoA;
    QPointF m_twoB;
    epaper::document::DeviceDocument m_document;
    epaper::document::OneWaySyncSession m_oneWay;
    std::vector<std::int64_t> m_ingestNs;
    int m_ingestApplied = 0;
    int m_ingestRejected = 0;
    bool m_loggedRetiredSnapshot = false;
    QJsonArray m_pickables;
    epaper::toolchip::ChipModel m_chip;
    QString m_toolMode = QStringLiteral("pen");
    /** Exclusive tool latched at pen-down (SRS-EP-04 / SRS-EP-10). */
    QString m_strokeArmedTool;
    QString m_lastStrokeLatch;
    bool m_needEncloseRasterize = false;
    /** @implements [SRS-EP-12] UI-EP-06 enclose pulse + last-join highlight (CHL-0020) */
    std::unordered_set<std::string> m_blinkInkIds;
    std::unordered_set<std::string> m_highlightInkIds;
    qreal m_blinkWidthMul = 1.0;
    int m_blinkToken = 0;
    QRectF m_toolChipRect;
    QString m_selectedPickableId;
    QStringList m_selectedIds;
    QVector<QPointF> m_lassoPanel;
    QPointF m_marqueeStartPanel;
    QPointF m_marqueeEndPanel;
    enum class SelGesture { None, Move, Resize, Marquee, Lasso } m_selGesture = SelGesture::None;
    ToolCanvasItem *m_toolCanvas = nullptr;
    QRectF m_toolChromePrev;
    QRectF m_selectionBoundsRect;
    int m_handleCount = 0;
    qreal m_handleSize = 16.0;
    bool m_modeChipVisible = false;
    QString m_modeChipLabel;
    QRectF m_modeChipRect;
    QRectF m_originPanelRect;
    QRectF m_originConnPunch;
    /** Rest-pose connector polylines in panel space — CanvasLayer origin hole (stroke, not AABB). */
    struct OriginConnStroke {
        QVector<QPointF> panel;
        qreal width = 4;
    };
    QVector<OriginConnStroke> m_originConnStrokes;
    QRectF m_encloseCtaRect;
    bool m_encloseVisible = false;
    QString m_encloseRefuseReason;
    QString m_manipUnavailable;
    QRectF m_manipUnavailableRect;
    epaper::gesture::ManipDrag m_drag;
    bool m_selectionGesture = false;
    /** Last panel-space chrome rect — dirty region for soft update during drag. */
    QRectF m_selectionChromeDirty;
    QRectF m_liveDirtyPrev;
    int m_toolIntentSeq = 0;
    QElapsedTimer m_selectionGhostClock;
    static constexpr qint64 kSelectionGhostMinIntervalMs = 200;
    bool m_rasterizePending = false;
    bool m_rasterizeSharp = false;
    /** Deferred sharp refresh queued while a stroke was in flight. */
    bool m_rasterizeDeferredSharp = false;
    int m_settleFollowUpToken = 0;
    QPointF m_lastRaw;
    /** Last QTabletEvent sample — PointHandler only exposes pressure. */
    IngestChannels m_stashTablet;
    QPointF m_stashRaw;
    bool m_stashValid = false;
    bool m_pinchIgnore = false;
    qreal m_pinchArm = 80.0;
    qreal m_pinchScale0 = 1.0;
    QElapsedTimer m_flushClock;
    QElapsedTimer m_refreshClock;
    QRectF m_pendingDirty;
    static constexpr qint64 kFlushIntervalMs = 8;
    static constexpr qint64 kRefreshMinIntervalMs = 250;
    /** Single deferred settle pass — keep light; avoid swap storms. */
    static constexpr qint64 kSettleFollowUpMs = 180;
    static constexpr qreal kBaseWorldStroke = 2.5;
};
