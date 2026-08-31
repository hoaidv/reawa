#pragma once

#include <QQuickPaintedItem>
#include <QVector>
#include <QPointF>
#include <QEvent>
#include <QElapsedTimer>
#include <QImage>
#include <QRectF>
#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "canvas_frame.hpp"
#include "canvas_session.h"
#include "document/device_document.hpp"
#include "document/hand_touch.hpp"
#include "document/one_way_sync.hpp"
#include "document/viewport_follow.hpp"
#include "input/pen_sample.hpp"
#include "debug/rasterize_probe.hpp"
#include "primary_toolbar.hpp"
#include "rendering/rendering.hpp"
#include "stroke_capture.hpp"
#include "camera_sharp_job.hpp"
#include "util/latest_job.hpp"

#include <optional>

class StrokeSync;

/**
 * Pen ink + device document rasterize for the sync region.
 * @implements [SRS-EP-01]
 * @implements [SRS-EP-02] vector ∩ drawingRegion paint (no bitmap push)
 * @implements [SRS-EP-07] local tree paint + stroke ingest
 * @implements [SRS-EP-08] one-way sync handshake and publish
 * @implements [SRS-EP-11] live SmartGroup manipulation
 * @implements [SRS-EP-21] one-finger pick move palm pan
 * @implements [SRS-EP-23] finger exclusive-tool switch
 * @implements [SRS-EP-03] camera coalesce paint attribution
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
    ~TabletCanvasItem() override;

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void componentComplete() override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;

private:
    int m_updateNodeLogs = 0;

/**
 * =================================================================================================
 * Canvas session — shared document / frame / chip / follow
 * =================================================================================================
 */

public:
    CanvasSession *session() { return &m_session; }
    const CanvasSession *session() const { return &m_session; }

    Q_PROPERTY(CanvasSession *session READ session CONSTANT)

/**
 * =================================================================================================
 * Canvas frame — orientation, camera region, transforms
 *
 * Uses m_session.frame. Mutators return FrameIntent; applyFrameIntent() is the only place
 * that turns those into Qt effects.
 * =================================================================================================
 */

public:
    /** Panel (canvas item) → document world. */
    WorldPt panelToWorld(const PanelPt &panel) const;

protected:
private:
    qreal ingestPanelHeight() const;
    void syncFramePanelSize() const;
    void applyFrameIntent(epaper::canvasframe::FrameIntent intent);

    bool orientationLandscape() const { return m_session.frame.landscape(); }
    bool orientationInvertX() const { return m_session.frame.invertX(); }
    bool orientationInvertY() const { return m_session.frame.invertY(); }
    FrameUv panelToFrameUv(const PanelPt &panel) const;
    PanelPt frameUvToPanel(FrameUv uv) const;
    PanelPt worldToPanel(double wx, double wy) const;
    PanelPt worldToPanel(WorldPt w) const { return worldToPanel(w.x, w.y); }
    double panelScale() const;
    QRectF worldBoundsToPanel(const epaper::document::SmartBounds &wb) const;
    bool viewportZoomedOut() const;
    bool lodOkPanel(const epaper::document::SmartBounds &wb) const;

    CanvasSession m_session;

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

    QImage m_image;
    bool m_paintsInk = true;
    QRectF m_pendingDirty;
    QElapsedTimer m_flushClock;
    static constexpr qint64 kFlushIntervalMs = 8;

/**
 * =================================================================================================
 * Device document and undo history
 * 
 * Cycle: requestUndo/requestRedo → applyHistoryRestore → notifyHistory → noteDocumentMutated
 * =================================================================================================
 */

 
public:
    bool canUndo() const { return m_session.document.undoDepth() > 0; }
    bool canRedo() const { return m_session.document.redoDepth() > 0; }
    Q_INVOKABLE void requestUndo();
    Q_INVOKABLE void requestRedo();

signals:
    void historyChanged();

private:
    void applyHistoryRestore(bool isUndo);

    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)

/**
 * =================================================================================================
 * Pen stroke capture and ingest
 * 
 * Cycles: 
 *      sample entry (ingestMappedTablet); 
 *      stroke life (beginStroke → appendPoint → endStroke → ingestCurrentStroke)
 * =================================================================================================
 */
public:
    int strokeCount() const { return m_stroke.strokeCount; }
    QPointF lastPoint() const { return PanelPt(m_stroke.lastPanelX, m_stroke.lastPanelY); }

    void ingestMappedTablet(QEvent::Type type, const PanelPt &canvasPos, RawPt rawPos,
        const IngestChannels &ch);
        
    std::string ingestDumpText() const;

signals:
    void strokeCountChanged();

private:
    Point makePoint(const epaper::strokecapture::Sample &s) const;
    epaper::strokecapture::Channels toChannels(const IngestChannels &ch) const;
    void applyStrokeIntent(const epaper::strokecapture::StrokeResult &r);
    void applyContactPress(const PanelPt &canvasPos, const IngestChannels &ch);
    void beginStroke(const PanelPt &canvasPos, const IngestChannels &ch);
    void appendPoint(const PanelPt &canvasPos, const IngestChannels &ch);
    void endStroke();
    void ingestCurrentStroke(const epaper::document::FinishedStroke &stroke);
    qreal worldStrokeWidth(qreal pressure) const;

    epaper::strokecapture::StrokeCapture m_stroke;
    bool m_needEncloseRasterize = false;
    std::vector<std::int64_t> m_ingestNs;
    int m_ingestApplied = 0;
    int m_ingestRejected = 0;

    Q_PROPERTY(int strokeCount READ strokeCount NOTIFY strokeCountChanged)
    Q_PROPERTY(QPointF lastPoint READ lastPoint NOTIFY debugChanged)

/**
 * =================================================================================================
 * Rasterize scheduling and document tree paint
 * 
 * Cycle: blit preview + LatestJob vector ([SRS-EP-03]); dirty/enclose stay GUI InPlaceDirty.
 * Camera vector is never on the pointer stack ([SRS-EP-01]).
 * =================================================================================================
 */

private:

    void scheduleVectorRasterize(bool sharp);
    void rasterizeVectors(bool sharp, const QRectF &panelDirty = QRectF());
    void armRasterTimer();
    void onRasterTimer();
    void bumpSnapEpoch();
    void submitCameraSharp();
    void onCameraSharpResult(epaper::camerasharp::Result result);
    void applyCameraSharp(epaper::camerasharp::Result result);
    bool blitOnto(QImage *dst, const QImage &src, epaper::rasterprobe::CamBox srcCam);
    epaper::rasterprobe::CamBox currentCamBox() const;
    bool tryPaintCameraBlit(bool sharp, epaper::rasterprobe::CamDelta cam,
                            const epaper::rasterprobe::CameraPlan &plan, int *warpMs,
                            int *renderMs);
    void finishRasterize(const epaper::rasterprobe::Record &rec,
                         const epaper::rasterprobe::CamBox &next);
    QRectF padRasterDirty(const QRectF &panel) const;
    epaper::render::WorldAabb panelRectToWorldClip(const QRectF &panel) const;
    QRectF panelBoundOfNodeId(const std::string &id) const;
    void emitRecogChrome(int kind, const std::vector<std::string> &ids);
    void collectSmartGroupInkIds(const epaper::document::DocNode &sg, bool boundaryOnly,
        std::vector<std::string> *out) const;

    bool m_rasterizePending = false;
    bool m_rasterizeSharp = false;
    /** Deferred sharp refresh queued while a stroke was in flight. */
    bool m_rasterizeDeferredSharp = false;
    /** Erase primary down — defer FullClear so the ghost is not wiped. Not set
     *  for move/resize: those need an immediate rasterize with suppressIds. */
    bool m_erasePointerActive = false;
    /** [D05] documentMutated after noteDocumentDirty must not FullClear. */
    bool m_consumeMutatedRasterize = false;
    QRectF m_pendingInPlaceDirty;
    QRectF m_encloseDirtyPanel;
    epaper::render::DocumentRenderer m_renderer;
    epaper::rasterprobe::Why m_rasterWhy = epaper::rasterprobe::Why::Unknown;
    epaper::rasterprobe::CamBox m_rasterCam;
    std::string m_rasterOrient;
    QTimer m_rasterTimer;
    bool m_pointerBusy = false;
    /** Blit preview is on screen; LatestJob owes a vector at the current camera. */
    bool m_cameraNeedsSharp = false;
    std::uint64_t m_snapEpoch = 0;
    std::uint64_t m_cameraJobSeq = 0;
    std::shared_ptr<const std::vector<epaper::document::DocNode>> m_cameraSnap;
    std::uint64_t m_cameraSnapEpoch = 0;
    epaper::LatestJob<epaper::camerasharp::Job, epaper::camerasharp::Result> m_cameraJob;
    std::optional<epaper::camerasharp::Result> m_heldSharp;

    QElapsedTimer m_refreshClock;
    static constexpr qint64 kRefreshMinIntervalMs = 250;
    
/**
 * =================================================================================================
 * Connector ink rendering
 * =================================================================================================
 */

/**
 * =================================================================================================
 * Recognizer feedback
 *
 * Recognizer feedback is ToolCanvas NodeEmphasis (CHL-0030). Tablet only emits ids.
 * =================================================================================================
 */

/**
 * =================================================================================================
 * Tool modes / ToolChip
 * =================================================================================================
 */

public:


    QString toolMode() const { return m_session.exclusiveTool(); }
    void setToolMode(const QString &mode);
    Q_INVOKABLE void armTool(const QString &mode);
    Q_INVOKABLE bool togglePenEraser();
    Q_INVOKABLE bool beginTempErase();
    Q_INVOKABLE bool endTempErase();  
    
    bool recogInkBoxArmed() const { return m_session.chip.recogInkBox; }
    bool recogConnectorArmed() const { return m_session.chip.recogConnector; }
    bool recogTogglesDimmed() const { return m_session.chip.recogDimmed(); }
    
    Q_INVOKABLE void toggleRecogInkBox();
    Q_INVOKABLE void toggleRecogConnector();
   
    QString lastStrokeLatch() const { return m_lastStrokeLatch; }

signals: 
    void toolModeChanged();
    void recogChanged();
    void lastStrokeLatchChanged();

private: 

    QString m_lastStrokeLatch;
    
    Q_PROPERTY(QString toolMode READ toolMode WRITE setToolMode NOTIFY toolModeChanged)
    Q_PROPERTY(bool recogInkBoxArmed READ recogInkBoxArmed NOTIFY recogChanged)
    Q_PROPERTY(bool recogConnectorArmed READ recogConnectorArmed NOTIFY recogChanged)
    Q_PROPERTY(bool recogTogglesDimmed READ recogTogglesDimmed NOTIFY toolModeChanged)
    Q_PROPERTY(QString lastStrokeLatch READ lastStrokeLatch NOTIFY lastStrokeLatchChanged)
    
    

/**
 * =================================================================================================
 * Surface API for ToolCanvas
 * =================================================================================================
 */

public:
    void ingestPen(QEvent::Type type, const PanelPt &canvasPos, RawPt rawPos,
                   const IngestChannels &ch);
    IngestChannels stashedChannels(const PanelPt &panel, RawPt *raw) const;
    void clearStash();
    bool strokeActive() const { return m_stroke.active; }
    void setErasePointerActive(bool on);
    /** True while a ToolCanvas pointer/pinch callback is on the stack. Vector
     *  FullClear must wait; camera blit may still run. */
    void setPointerBusy(bool on);
    /** Arm the coalesce timer for work deferred during pointerBusy — never
     *  FullClear synchronously on pointer-up (next down is ~30 ms). */
    void flushDeferredRasterize();
    void cancelActiveStroke();

    void scheduleDocumentRasterize(bool sharp);
    void scheduleDirtyRasterize(const QRectF &panelDirty, bool sharp);
    void publishManipPreview(const std::string &nodeId,
                             const epaper::document::SmartTransform &liveT,
                             const epaper::document::SmartBounds *liveB);
    void flushWire();
    void notifyHistory();
    void maybePublishLocalViewport(bool settle);
    void ensureLocalDrawingRegion();
    void setInteractionDebug(const QString &info);
    bool lodOkWorld(const epaper::document::SmartBounds &wb) const;
    QRectF boundConnectorsPanelUnion(const std::string &sgId) const;
    qreal connectorPanelStrokeWidth(const epaper::document::DocNode &conn) const;
    QRectF warpedConnectorPanelRect(const epaper::document::DocNode &conn) const;

/**
 * =================================================================================================
 * Pointer sample stash (digitizer → Tool routes via Surface ingestPen)
 * =================================================================================================
 */

public:
    void stashTabletSample(const QPointF &raw, const IngestChannels &ch);

private:
    IngestChannels m_stashTablet;
    RawPt m_stashRaw;
    bool m_stashValid = false;

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
    QString followDirection() const { return m_session.followDirection(); }
    
    bool followPressed() const { return m_session.follow.ariaPressed(); }
    bool followUnavailable() const { return m_session.follow.ariaDisabled(); }
    Q_INVOKABLE void tapFollowToggle();

signals:
    void followChanged();

        
private:
    void applyViewport(const QJsonObject &obj);
    void applyFollowCamera();
    void flushFollowOutbound();
    void cacheInfiniViewport(const QJsonObject &obj);
    epaper::handtouch::FollowDirection followEnum() const;


    /** Reawa-style gut pose; legacy "portrait"/"landscape" normalized on ingest. */
    int m_viewportSeq = 0;
    int m_viewportUpCount = 0;

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
