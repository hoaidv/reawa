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

#include "canvas_frame.hpp"
#include "document/device_document.hpp"
#include "document/hand_touch.hpp"
#include "document/manipulate.hpp"
#include "document/one_way_sync.hpp"
#include "document/viewport_follow.hpp"
#include "gesture/manipdrag.h"
#include "input/pen_sample.hpp"
#include "primary_toolbar.hpp"

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
    Q_PROPERTY(bool paintsInk READ paintsInk CONSTANT)
    Q_PROPERTY(QString debugInfo READ debugInfo NOTIFY debugChanged)
    Q_PROPERTY(bool debugLogVisible READ debugLogVisible NOTIFY debugLogVisibleChanged)

/**
 * =================================================================================================
 * Types
 * =================================================================================================
 */

public:
    /** Digitizer channels reported on this sample (SRS-EP-09). Unset = not reported. */
    using IngestChannels = epaper::input::PenSample;

    /**
     * Coordinate spaces.
     *
     * Panel is Qt's own space (canvas item pixels), so it stays a plain QPointF and
     * drops straight into QPainter, QRectF and QLineF; the alias is documentation
     * only and enforces nothing on its own. World / frame-uv live in CanvasFrame
     * (double PODs). Raw is distinct here. Neither World nor Raw converts to
     * QPointF implicitly, so a bare QPointF is panel by construction.
     */
    using PanelPt = QPointF;
    using WorldPt = epaper::canvasframe::WorldPt;
    using FrameUv = epaper::canvasframe::FrameUv;
    using WorldAabb = epaper::canvasframe::WorldAabb;

    /** Digitizer position, before mapPanel() rotates it into panel space. */
    struct RawPt {
        qreal x = 0;
        qreal y = 0;
    };

    /**
     * Hatch for modules that still carry world points as QPointF (gesture/manipdrag).
     * Greppable on purpose — each use is a place the space stops being checked.
     */
    static QPointF worldQ(WorldPt w) { return QPointF(w.x, w.y); }

private:
    struct Point {
        PanelPt pos;
        qreal pressure;
        RawPt raw;
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

/**
 * =================================================================================================
 * Construction and Qt item lifecycle
 * =================================================================================================
 */

public:
    explicit TabletCanvasItem(QQuickItem *parent = nullptr);

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void componentComplete() override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;

private:
    int m_updateNodeLogs = 0;

/**
 * =================================================================================================
 * Canvas frame — orientation, camera region, transforms
 *
 * Owns m_frame. Mutators return FrameIntent; applyFrameIntent() is the only place
 * that turns those into Qt effects.
 * =================================================================================================
 */

public:
    /** Digitizer → panel. Distinct types, so the mapped point cannot be remapped. */
    PanelPt mapInputToCanvas(RawPt raw) const;

    /** Panel (canvas item) → document world. */
    WorldPt panelToWorld(const PanelPt &panel) const;

protected:
private:
    qreal ingestPanelHeight() const;
    void syncFramePanelSize() const;
    void applyFrameIntent(epaper::canvasframe::FrameIntent intent);

    bool orientationLandscape() const { return m_frame.landscape(); }
    bool orientationInvertX() const { return m_frame.invertX(); }
    bool orientationInvertY() const { return m_frame.invertY(); }
    FrameUv panelToFrameUv(const PanelPt &panel) const;
    PanelPt frameUvToPanel(FrameUv uv) const;
    PanelPt worldToPanel(double wx, double wy) const;
    PanelPt worldToPanel(WorldPt w) const { return worldToPanel(w.x, w.y); }
    double panelScale() const;
    QRectF worldBoundsToPanel(const epaper::document::SmartBounds &wb) const;
    bool viewportZoomedOut() const;
    bool lodOkPanel(const epaper::document::SmartBounds &wb) const;
    void ensureLocalDrawingRegion();

    epaper::canvasframe::CanvasFrame m_frame;

/**
 * =================================================================================================
 * Panel raster surface
 * 
 * Cycle: ensureImage → emitSegment/paintSegment → flushPending
 * =================================================================================================
 */


public:
    bool paintsInk() const { return m_paintsInk; }
    void paint(QPainter *painter) override;

signals:
    void segmentDrawn(qreal x1, qreal y1, qreal x2, qreal y2, qreal lineWidth);
    
private:

    void ensureImage();
    void paintSegment(const Point &from, const Point &to, qreal lineWidth);
    void emitSegment(const Point &from, const Point &to);
    void flushPending();
    void stampStaticBeacon();
    void stampFlushBeacon();


    QImage m_image;
    bool m_paintsInk = true;
    bool m_beacons = true;
    QRectF m_pendingDirty;
    QElapsedTimer m_flushClock;
    int m_flushCount = 0;
    QAtomicInt m_paintCount{0};
    static constexpr qint64 kFlushIntervalMs = 8;

/**
 * =================================================================================================
 * Device document and undo history
 * 
 * Cycle: requestUndo/requestRedo → applyHistoryRestore → pruneSelectionAfterHistory → notifyHistory
 * =================================================================================================
 */

 
public:
    bool canUndo() const { return m_document.undoDepth() > 0; }
    bool canRedo() const { return m_document.redoDepth() > 0; }
    Q_INVOKABLE void requestUndo();
    Q_INVOKABLE void requestRedo();

signals:
    void historyChanged();

private:
    void applyHistoryRestore(bool isUndo);
    void pruneSelectionAfterHistory();
    void notifyHistory();

    epaper::document::DeviceDocument m_document;
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)

/**
 * =================================================================================================
 * Pen stroke capture and ingest
 * 
 * Cycles: 
 *      sample entry (ingestPoint ×2, ingestMappedTablet); 
 *      stroke life (beginStroke → appendPoint → endStroke → ingestCurrentStroke)
 * =================================================================================================
 */
public:
    int strokeCount() const { return m_strokeCount; }
    QPointF lastPoint() const { return m_lastPoint; }

    /**
     * @p pos is raw, not panel — it is QPointF because Q_INVOKABLE marshals through
     * moc, which cannot carry RawPt. Converted on entry.
     */
    Q_INVOKABLE void ingestPoint(QEvent::Type type, const QPointF &pos, qreal pressure);
    void ingestPoint(QEvent::Type type, const QPointF &pos, const IngestChannels &ch);

    void ingestMappedTablet(QEvent::Type type, const PanelPt &canvasPos, RawPt rawPos,
        const IngestChannels &ch);
        
    std::string ingestDumpText() const;

signals:
    void strokeCountChanged();

private:
    Point makePoint(const PanelPt &canvasPos, const IngestChannels &ch) const;
    void applyContactPress(const PanelPt &canvasPos, const IngestChannels &ch);
    void beginStroke(const PanelPt &canvasPos, const IngestChannels &ch);
    void appendPoint(const PanelPt &canvasPos, const IngestChannels &ch);
    void endStroke();
    void ingestCurrentStroke();
    qreal worldStrokeWidth(qreal pressure) const;

    
    QVector<Point> m_current;
    Point m_lastEmitted{};
    bool m_hasEmitted = false;
    bool m_strokeActive = false;
    QString m_activeStrokeId;
    int m_strokeSeq = 0;
    int m_strokeCount = 0;
    PanelPt m_lastPoint;
    RawPt m_lastRaw;
    qreal m_activeWorldStrokeWidth = 2.5;
    /** @fix [STORY-EP-033] wait for a non-origin sample after a stale Press. */
    bool m_awaitingPlausiblePress = false;
    /** True after stroke_begin went to Infini — origin starts must not preview. */
    bool m_strokePreviewSent = false;
    bool m_needEncloseRasterize = false;
    std::vector<std::int64_t> m_ingestNs;
    int m_ingestApplied = 0;
    int m_ingestRejected = 0;
    static constexpr qreal kBaseWorldStroke = 2.5;
    

    Q_PROPERTY(int strokeCount READ strokeCount NOTIFY strokeCountChanged)
    Q_PROPERTY(QPointF lastPoint READ lastPoint NOTIFY debugChanged)

/**
 * =================================================================================================
 * Rasterize scheduling and document tree paint
 * 
 * Cycle: scheduleVectorRasterize → rasterizeVectors → drawTree → drawDocNode
 * =================================================================================================
 */

private:

    void scheduleVectorRasterize(bool sharp);
    void rasterizeVectors(bool sharp);
    void drawTree(QPainter &p, const std::vector<epaper::document::DocNode> &nodes,
        const epaper::document::DocNode *smartParent);
    void drawDocNode(QPainter &p, const epaper::document::DocNode &node,
        const epaper::document::DocNode *smartParent = nullptr);

    bool m_rasterizePending = false;
    bool m_rasterizeSharp = false;
    /** Deferred sharp refresh queued while a stroke was in flight. */
    bool m_rasterizeDeferredSharp = false;
    int m_settleFollowUpToken = 0;

    QElapsedTimer m_refreshClock;
    static constexpr qint64 kRefreshMinIntervalMs = 250;
    /** Single deferred settle pass — keep light; avoid swap storms. */
    static constexpr qint64 kSettleFollowUpMs = 180;
    
/**
 * =================================================================================================
 * Connector ink rendering
 * =================================================================================================
 */

private: 

    qreal connectorPanelStrokeWidth(const epaper::document::DocNode &conn) const;
    void drawWarpedConnector(QPainter &p, const epaper::document::DocNode &conn);
    QRectF warpedConnectorPanelRect(const epaper::document::DocNode &conn) const;
    QRectF boundConnectorsPanelUnion(const std::string &sgId) const;

/**
 * =================================================================================================
 * Recognizer feedback
 *
 * Cycles: 
 *      beginRecogWidthBlink (self-expiring via m_blinkToken guard in its timer lambda); 
 *      setMembershipHighlight ↔ clearMembershipHighlight
 * =================================================================================================
 */

private:

    void beginRecogWidthBlink(const std::vector<std::string> &inkIds);
    /** Returns true when the highlighted boundary set changed. */
    bool setMembershipHighlight(const std::vector<std::string> &boundaryInkIds);
    void clearMembershipHighlight();
    void collectSmartGroupInkIds(const epaper::document::DocNode &sg, bool boundaryOnly,
        std::vector<std::string> *out) const;

    
    /** @implements [SRS-EP-12] UI-EP-06 enclose pulse + last-join highlight (CHL-0020) */
    std::unordered_set<std::string> m_blinkInkIds;
    qreal m_blinkWidthMul = 1.0;
    int m_blinkToken = 0;
    std::unordered_set<std::string> m_highlightInkIds;

/**
 * =================================================================================================
 * Tool modes / ToolChip
 * =================================================================================================
 */

public:


    QString toolMode() const { return m_toolMode; }
    void setToolMode(const QString &mode);
    Q_INVOKABLE void armTool(const QString &mode);  
    
    bool recogInkBoxArmed() const { return m_chip.recogInkBox; }
    bool recogConnectorArmed() const { return m_chip.recogConnector; }
    bool recogTogglesDimmed() const { return m_chip.recogDimmed(); }
    
    Q_INVOKABLE void toggleRecogInkBox();
    Q_INVOKABLE void toggleRecogConnector();
   
    QString lastStrokeLatch() const { return m_lastStrokeLatch; }

signals: 
    void toolModeChanged();
    void recogChanged();
    void lastStrokeLatchChanged();

private: 

    bool isSelectionTool() const;

    epaper::toolchip::ChipModel m_chip;
    QString m_toolMode = QStringLiteral("pen");
    QString m_lastStrokeLatch;
    
    Q_PROPERTY(QString toolMode READ toolMode WRITE setToolMode NOTIFY toolModeChanged)
    Q_PROPERTY(bool recogInkBoxArmed READ recogInkBoxArmed NOTIFY recogChanged)
    Q_PROPERTY(bool recogConnectorArmed READ recogConnectorArmed NOTIFY recogChanged)
    Q_PROPERTY(bool recogTogglesDimmed READ recogTogglesDimmed NOTIFY toolModeChanged)
    Q_PROPERTY(QString lastStrokeLatch READ lastStrokeLatch NOTIFY lastStrokeLatchChanged)
    
    
/**
 * =================================================================================================
 * Pointer routing and contact arbitration
 * 
 * Cycles: 
 *      onPointerStart → onPointerMove → onPointerEnd | onPointerCancel;
 *      onPinchStart → onPinchUpdate → onPinchEnd
 * =================================================================================================
 */

public:
    /**
     * Slot bound to QtInputFilter::penSample, so the parameter is the signal's
     * QPointF; it is raw, and converted at the boundary rather than in input/,
     * which must not depend on a type nested in this class.
     */
    void stashTabletSample(const QPointF &raw, const IngestChannels &ch);
     
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

private:
    PanelPt pinchArmPoint(qreal x, qreal y, qreal scale, bool positive) const;

    /**
     * Channels for this contact: the stashed QTabletEvent sample when one is valid,
     * otherwise bare defaults. Writes the matching raw point through @p raw.
     */
    IngestChannels stashedChannels(const PanelPt &panel, RawPt *raw) const;

    /** Last QTabletEvent sample — PointHandler only exposes pressure. */
    IngestChannels m_stashTablet;
    RawPt m_stashRaw;
    bool m_stashValid = false;

    bool m_fingerLockedUntilLift = false;

    bool m_pinchIgnore = false;
    qreal m_pinchArm = 80.0;
    qreal m_pinchScale0 = 1.0;

/**
 * =================================================================================================
 * Hand touch
 * 
 * Cycles: 
 *      beginFingerTouch → updateFingerTouch → endFingerTouch; 
 *      beginTwoFingerTouch → updateTwoFingerTouch → endTwoFingerTouch; 
 *      cancelHandTouch as the escape hatch
 * =================================================================================================
 */

public:
    bool handTouchArmed() const { return m_handTouchArmed; }
    Q_INVOKABLE void toggleHandTouch();
    Q_INVOKABLE void cancelHandTouch();


    /**
     * Capacitive one-finger path (STORY-EP-038). Knob / box / empty palm vs pan.
     * @implements [SRS-EP-21] one-finger canvas hit
     */
    bool beginFingerTouch(const PanelPt &canvasPos);
    void updateFingerTouch(const PanelPt &canvasPos, int fingerCount);
    void endFingerTouch(const PanelPt &canvasPos);

    /**
     * Two-finger local pan/pinch (STORY-EP-039). A one-finger manip already in
     * flight is reverted, not blocking: two contacts always mean navigate.
     * @implements [SRS-EP-24] two-finger canvas pan pinch
     */
    bool beginTwoFingerTouch(const PanelPt &a, const PanelPt &b);
    void updateTwoFingerTouch(const PanelPt &a, const PanelPt &b);
    void endTwoFingerTouch();

signals:
    void handTouchArmedChanged();


private:

    enum class FingerGesture { 
        None, Chip, Move, Resize, EmptyPending, EmptyPan, TwoFinger 
    } m_fingerGesture = FingerGesture::None;

    bool fingerHitsBox(const PanelPt &canvasPos) const;
    void applyLocalFingerPan(const PanelPt &canvasPos);
    void applyLocalTwoFinger(const PanelPt &a, const PanelPt &b);
    epaper::handtouch::TwoFingerContacts uvPair(const PanelPt &a, const PanelPt &b) const;

    bool m_handTouchArmed = true;
    PanelPt m_fingerDownPanel;
    WorldPt m_fingerDownWorld;
    WorldAabb m_fingerPanOrigin;
    QElapsedTimer m_fingerPanClock;
    PanelPt m_twoA;
    PanelPt m_twoB;
    epaper::handtouch::TwoFingerContacts m_twoOriginContacts{};

    Q_PROPERTY(bool handTouchArmed READ handTouchArmed NOTIFY handTouchArmedChanged)

/**
 * =================================================================================================
 * Selection and direct manipulation
 * 
 * Sub-cycle A — gesture: beginSelectionGesture → updateSelectionGesture → endSelectionGesture
 * Sub-cycle B — marquee/lasso: beginMarqueeOrLasso → finishMarqueeOrLasso
 * Sub-cycle C — live manip: startLiveManip → applyDragWorld → redrawLiveManipRegion → commitLiveManip | abortFingerManip
 * Sub-cycle D — handle entry: handleIndexAtPanel → tryBeginHandleAtPanel → beginHandleDrag
 * 
 * =================================================================================================
 */

public:

    Q_INVOKABLE void encloseSelection();
    void abortFingerManip();

    /** @implements [SRS-EP-11] handle drag in world space */
    void beginHandleDrag(int handleIndex, WorldPt world);

    Q_INVOKABLE void tapModeChip();

    QString manipulationUnavailable() const { return m_manipUnavailable; }
    QRectF manipulationUnavailableRect() const { return m_manipUnavailableRect; }
        
private:


    enum class SelGesture { None, Move, Resize, Marquee, Lasso };

    /** Rest-pose connector polylines in panel space — CanvasLayer origin hole (stroke, not AABB). */
    struct OriginConnStroke {
        QVector<PanelPt> panel;
        qreal width = 4;
    };

    /** Single source of truth for "a selection gesture is in flight". */
    bool selectionGestureActive() const { return m_selGesture != SelGesture::None; }

    /** User-initiated deselect. Not for host reconciliation or history pruning. */
    void clearSelection();
    /** Replace the selection; the first id becomes the pickable. Leaves m_drag alone. */
    void setSelection(const std::vector<std::string> &ids);

    QString hitLocalSmartGroup(WorldPt world) const;

    // Cycle A
    void beginSelectionGesture(const PanelPt &canvasPos);
    void updateSelectionGesture(const PanelPt &canvasPos);
    void endSelectionGesture();

    // Cycle B
    void beginMarqueeOrLasso(const PanelPt &canvasPos);
    void finishMarqueeOrLasso();

    // Cycle C
    void startLiveManip(const epaper::document::DocNode *subject,
        epaper::document::ResizeHandle handle, WorldPt world);
    void applyDragWorld(WorldPt world);
    void redrawLiveManipRegion();
    void commitLiveManip();
    // void abortFingerManip() is public

    // Cycle D
    int handleIndexAtPanel(const PanelPt &panel, double hitDu) const;
    bool tryBeginHandleAtPanel(const PanelPt &panel, double hitDu);
    // void beginHandleDrag(...) is public

    void captureOriginConnectorPunches(const std::string &sgId);
    void showManipUnavailable(const epaper::document::SmartBounds &wb);
    void sendManipPreviewToInfini();


    SelGesture m_selGesture = SelGesture::None;
    QStringList m_selectedIds;
    QString m_selectedPickableId;
    QVector<PanelPt> m_lassoPanel;
    PanelPt m_marqueeStartPanel;
    PanelPt m_marqueeEndPanel;
    
    epaper::gesture::ManipDrag m_drag;
    QRectF m_liveDirtyPrev;
    QRectF m_originPanelRect;
    QRectF m_originConnPunch;
    QVector<OriginConnStroke> m_originConnStrokes;

    QElapsedTimer m_selectionGhostClock;

    int m_toolIntentSeq = 0;

    QString m_manipUnavailable;
    QRectF m_manipUnavailableRect;

    static constexpr qint64 kSelectionGhostMinIntervalMs = 200;

    Q_PROPERTY(QString manipulationUnavailable READ manipulationUnavailable NOTIFY selectionChromeChanged)
    Q_PROPERTY(QRectF manipulationUnavailableRect READ manipulationUnavailableRect NOTIFY selectionChromeChanged)

/**
 * =================================================================================================
 * Selection chrome and ToolCanvas overlay
 *
 * Cycle: refreshSelectionChrome → damageToolChrome/damageToolChromeSegment → paintToolChrome
 * =================================================================================================
 */

public:

    void bindToolCanvas(class ToolCanvasItem *overlay);
    /** @implements [SRS-EP-12] ToolCanvasLayer stroke chrome (no document blit) */
    void paintToolChrome(QPainter *painter);

    QRectF encloseCtaRect() const { return m_encloseCtaRect; }
    bool encloseVisible() const { return m_encloseVisible; }
    QString encloseRefuseReason() const { return m_encloseRefuseReason; }
    QRectF selectionBoundsRect() const { return m_selectionBoundsRect; }
    int handleCount() const { return m_handleCount; }
    qreal handleSize() const { return m_handleSize; }
    bool modeChipVisible() const { return m_modeChipVisible; }
    QString modeChipLabel() const { return m_modeChipLabel; }
    QRectF modeChipRectProp() const { return m_modeChipRect; }

signals:
    void selectionChromeChanged();
    
private:

    void refreshSelectionChrome();
    void damageToolChrome(const QRectF &next);
    void damageToolChromeSegment(const QRectF &seg);
    void syncToolCanvasPresence();
    void paintLiveManipOnToolCanvas(QPainter *painter);
    
    ToolCanvasItem *m_toolCanvas = nullptr;
    QRectF m_toolChromePrev;
    
    /** Last panel-space chrome rect — dirty region for soft update during drag. */
    QRectF m_selectionChromeDirty;
    QRectF m_selectionBoundsRect;
    int m_handleCount = 0;
    qreal m_handleSize = 16.0;

    bool m_modeChipVisible = false;
    QString m_modeChipLabel;
    QRectF m_modeChipRect;

    QRectF m_encloseCtaRect;
    bool m_encloseVisible = false;
    QString m_encloseRefuseReason;

    Q_PROPERTY(QRectF encloseCtaRect READ encloseCtaRect NOTIFY selectionChromeChanged)
    Q_PROPERTY(bool encloseVisible READ encloseVisible NOTIFY selectionChromeChanged)
    Q_PROPERTY(QString encloseRefuseReason READ encloseRefuseReason NOTIFY selectionChromeChanged)
    Q_PROPERTY(QRectF selectionBoundsRect READ selectionBoundsRect NOTIFY selectionChromeChanged)
    Q_PROPERTY(int handleCount READ handleCount NOTIFY selectionChromeChanged)
    Q_PROPERTY(qreal handleSize READ handleSize NOTIFY selectionChromeChanged)
    Q_PROPERTY(bool modeChipVisible READ modeChipVisible NOTIFY selectionChromeChanged)
    Q_PROPERTY(QString modeChipLabel READ modeChipLabel NOTIFY selectionChromeChanged)
    Q_PROPERTY(QRectF modeChipRect READ modeChipRectProp NOTIFY selectionChromeChanged)

/**
 * =================================================================================================
 * Region sync and viewport follow
 *
 * Cycles: inbound applyViewport; 
 *          outbound maybePublishLocalViewport; 
 *          follow tapFollowToggle → applyFollowCamera → flushFollowOutbound
 * =================================================================================================
 */

public: 
    int viewportUpCount() const { return m_viewportUpCount; }

    /** @implements [SRS-EP-21] pen near outranks hand touch — called from QML */
    QString followDirection() const { return m_followDirection; }
    
    bool followPressed() const { return m_follow.ariaPressed(); }
    bool followUnavailable() const { return m_follow.ariaDisabled(); }
    Q_INVOKABLE void tapFollowToggle();

signals:
    void followChanged();

        
private:
    void applyViewport(const QJsonObject &obj);
    void maybePublishLocalViewport(bool settle);
    void applyFollowCamera();
    void flushFollowOutbound();
    void cacheInfiniViewport(const QJsonObject &obj);
    epaper::handtouch::FollowDirection followEnum() const;


    /** Reawa-style gut pose; legacy "portrait"/"landscape" normalized on ingest. */
    int m_viewportSeq = 0;
    int m_viewportUpCount = 0;
    epaper::follow::FollowSession m_follow;
    QString m_followDirection = QStringLiteral("none");

    Q_PROPERTY(QString followDirection READ followDirection NOTIFY followChanged)
    Q_PROPERTY(bool followPressed READ followPressed NOTIFY followChanged)
    Q_PROPERTY(bool followUnavailable READ followUnavailable NOTIFY followChanged)

/**
 * =================================================================================================
 * One-way sync wire
 * 
 * Cycle: syncBegin → syncPoint → syncEnd → flushOneWayWire;
 *        inbound onHostMessage 
 * =================================================================================================
 */

private:

    // Outbound
    void syncBegin();
    void syncPoint(const Point &pt);
    void syncEnd();
    void flushOneWayWire();

    // Inbound
    void onHostMessage(const QJsonObject &obj);
    
    StrokeSync *m_sync = nullptr;
    epaper::document::OneWaySyncSession m_oneWay;


/**
 * =================================================================================================
 * Chrome layout
 * =================================================================================================
 */

public:

    QRectF toolChipRect() const { return m_toolChipRect; }
    QRectF followToggleRect() const { return m_followToggleRect; }
    QRectF usbLinkRect() const { return m_usbLinkRect; }
    QRectF debugToggleRect() const { return m_debugToggleRect; }
    QRectF handTouchToggleRect() const { return m_handTouchToggleRect; }
    QRectF debugLogRect() const { return m_debugLogRect; }

signals:
    void toolChipRectChanged();
    void trailingChromeChanged();


private:
    void updateToolChipRect();

    QRectF m_toolChipRect;
    QRectF m_followToggleRect;
    QRectF m_usbLinkRect;
    QRectF m_debugToggleRect;
    QRectF m_handTouchToggleRect;
    QRectF m_debugLogRect;

    /** Device-local exclusive tool: sel_rect | sel_freeform | pen — never synced (SRS-EP-04). */
    Q_PROPERTY(QRectF toolChipRect READ toolChipRect NOTIFY toolChipRectChanged)
    Q_PROPERTY(QRectF followToggleRect READ followToggleRect NOTIFY trailingChromeChanged)
    Q_PROPERTY(QRectF usbLinkRect READ usbLinkRect NOTIFY trailingChromeChanged)
    Q_PROPERTY(QRectF debugToggleRect READ debugToggleRect NOTIFY trailingChromeChanged)
    Q_PROPERTY(QRectF handTouchToggleRect READ handTouchToggleRect NOTIFY trailingChromeChanged)
    Q_PROPERTY(QRectF debugLogRect READ debugLogRect NOTIFY trailingChromeChanged)


/**
 * =================================================================================================
 * Debug and diagnostics
 * =================================================================================================
 */


public:
    QString debugInfo() const { return m_debugInfo; }
   
    bool debugLogVisible() const { return m_debugLogVisible; }
    Q_INVOKABLE void toggleDebugLog();
 
signals:
    void debugChanged();
    void debugLogVisibleChanged();
    
private:    

    QString m_debugInfo;
    bool m_debugLogVisible = false;

};
