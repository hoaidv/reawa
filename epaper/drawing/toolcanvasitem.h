#pragma once

#include <QQuickPaintedItem>
#include <QElapsedTimer>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

#include <string>
#include <vector>

#include "canvas_frame.hpp"
#include "document/device_document.hpp"
#include "document/hand_touch.hpp"
#include "document/manipulate.hpp"
#include "document/viewport_follow.hpp"
#include "finger_gesture_machine.hpp"
#include "input/pen_sample.hpp"
#include "manip_session.hpp"
#include "primary_toolbar.hpp"
#include "selection_session.hpp"

class CanvasSession;
class TabletCanvasItem;

/** Rest-pose connector polylines in panel space — Tablet origin hole (stroke, not AABB). */
struct OriginConnStroke {
    QVector<QPointF> panel;
    qreal width = 4;
};

/** Snapshot for TabletCanvas white-hole during live manip — POD, no private access. */
struct OriginPunchSnapshot {
    QRectF panelRect;
    QVector<OriginConnStroke> connStrokes;
};

/**
 * ToolCanvas — pointer/finger interaction + selection chrome. Never blits the document.
 * @implements [SRS-EP-12] SelectionOverlay stroke chrome
 * @implements [ADR-0019] ToolCanvasLayer: Pen while lasso/marquee, Mono after pen-up
 */
class ToolCanvasItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(TabletCanvasItem *surface READ surface WRITE setSurface NOTIFY surfaceChanged)
    Q_PROPERTY(bool handTouchArmed READ handTouchArmed NOTIFY handTouchArmedChanged)
    Q_PROPERTY(QRectF encloseCtaRect READ encloseCtaRect NOTIFY selectionChromeChanged)
    Q_PROPERTY(bool encloseVisible READ encloseVisible NOTIFY selectionChromeChanged)
    Q_PROPERTY(QString encloseRefuseReason READ encloseRefuseReason NOTIFY selectionChromeChanged)
    Q_PROPERTY(QRectF selectionBoundsRect READ selectionBoundsRect NOTIFY selectionChromeChanged)
    Q_PROPERTY(int handleCount READ handleCount NOTIFY selectionChromeChanged)
    Q_PROPERTY(qreal handleSize READ handleSize NOTIFY selectionChromeChanged)
    Q_PROPERTY(bool modeChipVisible READ modeChipVisible NOTIFY selectionChromeChanged)
    Q_PROPERTY(QString modeChipLabel READ modeChipLabel NOTIFY selectionChromeChanged)
    Q_PROPERTY(QRectF modeChipRect READ modeChipRectProp NOTIFY selectionChromeChanged)
    Q_PROPERTY(QString manipulationUnavailable READ manipulationUnavailable NOTIFY selectionChromeChanged)
    Q_PROPERTY(QRectF manipulationUnavailableRect READ manipulationUnavailableRect NOTIFY selectionChromeChanged)

public:
    using PanelPt = QPointF;
    using WorldPt = epaper::canvasframe::WorldPt;
    using FrameUv = epaper::canvasframe::FrameUv;
    using IngestChannels = epaper::input::PenSample;

    /** Match TabletCanvasItem::RawPt for Surface ingestPen. */
    struct RawPt {
        qreal x = 0;
        qreal y = 0;
    };

    explicit ToolCanvasItem(QQuickItem *parent = nullptr);

    /**
     * =================================================================================================
     * Interaction API for TabletCanvas
     * =================================================================================================
     */
    void setSession(CanvasSession *session);
    CanvasSession *session() const { return m_session; }

    void setSurface(TabletCanvasItem *surface);
    TabletCanvasItem *surface() const { return m_surface; }

    void clearSelection();
    void cancelInteraction();
    bool liveManipActive() const;
    OriginPunchSnapshot originPunch() const;

    void onDocumentOrCameraChanged();
    bool selectionGestureActive() const { return m_selection.active(); }
    bool selectionToolArmed() const { return isSelectionTool(); }
    void beginSelectionGesture(const PanelPt &canvasPos);
    void updateSelectionGesture(const PanelPt &canvasPos);
    void endSelectionGesture();
    bool tryBeginHandleAtPanel(const PanelPt &panel, double hitDu);

    void paint(QPainter *painter) override;
    void setStrokeWaveform(bool penInFlight);

    /**
     * Qt DragHandler / PinchHandler canvas entry.
     * @implements [SRS-EP-04] Qt pointer routing
     */
    Q_INVOKABLE void onPointerStart(qreal x, qreal y, qreal pressure, bool pen);
    Q_INVOKABLE void onPointerMove(qreal x, qreal y, qreal pressure, bool pen);
    Q_INVOKABLE void onPointerEnd(qreal x, qreal y, bool pen);
    Q_INVOKABLE void onPointerCancel();
    Q_INVOKABLE void onFingerTap(qreal x, qreal y);
    Q_INVOKABLE void onSecondContact();
    Q_INVOKABLE void onContactsCleared();
    Q_INVOKABLE void onPinchStart(qreal x, qreal y, qreal scale);
    Q_INVOKABLE void onPinchUpdate(qreal x, qreal y, qreal scale);
    Q_INVOKABLE void onPinchEnd();

    bool handTouchArmed() const { return m_finger.armed; }
    Q_INVOKABLE void toggleHandTouch();
    Q_INVOKABLE void cancelHandTouch();

    bool beginFingerTouch(const PanelPt &canvasPos);
    void updateFingerTouch(const PanelPt &canvasPos, int fingerCount);
    void endFingerTouch(const PanelPt &canvasPos);
    bool beginTwoFingerTouch(const PanelPt &a, const PanelPt &b);
    void updateTwoFingerTouch(const PanelPt &a, const PanelPt &b);
    void endTwoFingerTouch();

    Q_INVOKABLE void encloseSelection();
    void abortFingerManip();
    void beginHandleDrag(int handleIndex, WorldPt world);
    Q_INVOKABLE void tapModeChip();

    QString manipulationUnavailable() const { return m_manipUnavailable; }
    QRectF manipulationUnavailableRect() const { return m_manipUnavailableRect; }

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
    void surfaceChanged();
    void handTouchArmedChanged();
    void selectionChromeChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    epaper::document::DeviceDocument &doc();
    const epaper::document::DeviceDocument &doc() const;
    epaper::canvasframe::CanvasFrame &frame();
    const epaper::canvasframe::CanvasFrame &frame() const;
    epaper::toolchip::ChipModel &chip();
    epaper::follow::FollowSession &follow();

    void applyCameraRegion(const epaper::handtouch::WorldAabb &region, bool markValid);
    void syncPanelSize() const;
    WorldPt panelToWorld(const PanelPt &panel) const;
    PanelPt worldToPanel(double wx, double wy) const;
    PanelPt worldToPanel(WorldPt w) const { return worldToPanel(w.x, w.y); }
    FrameUv panelToFrameUv(const PanelPt &panel) const;
    QRectF worldBoundsToPanel(const epaper::document::SmartBounds &wb) const;
    bool lodOkPanel(const epaper::document::SmartBounds &wb) const;

    bool isSelectionTool() const;
    QString exclusiveTool() const;
    void setSelection(const std::vector<std::string> &ids);
    void applySelectionIntent(const epaper::selection::SelectionResult &r);
    void applyManipIntent(const epaper::manip::ManipResult &r, bool restoreOrigin = false);
    void applyFingerIntent(const epaper::fingergesture::FingerResult &r,
                           const PanelPt &panel = PanelPt());

    void beginMarqueeOrLasso(const PanelPt &canvasPos);
    void finishMarqueeOrLasso();
    void startLiveManip(const epaper::document::DocNode *subject,
                        epaper::document::ResizeHandle handle, WorldPt world);
    void applyDragWorld(WorldPt world);
    void redrawLiveManipRegion();
    void commitLiveManip();

    int handleIndexAtPanel(const PanelPt &panel, double hitDu) const;
    void captureOriginConnectorPunches(const std::string &sgId);
    void showManipUnavailable(const epaper::document::SmartBounds &wb);
    void sendManipPreviewToInfini();

    QString hitLocalSmartGroup(WorldPt world) const;
    bool fingerHitsBox(const PanelPt &canvasPos) const;
    epaper::handtouch::TwoFingerContacts uvPair(const PanelPt &a, const PanelPt &b) const;
    void worldThroughPanOrigin(const PanelPt &panel, double *wx, double *wy) const;
    PanelPt pinchArmPoint(qreal x, qreal y, qreal scale, bool positive) const;
    epaper::handtouch::FollowDirection followEnum() const;

    void refreshSelectionChrome();
    void damageToolChrome(const QRectF &next);
    void damageToolChromeSegment(const QRectF &seg);
    void syncToolCanvasPresence();
    void paintLiveManipOnToolCanvas(QPainter *painter);
    void paintToolChrome(QPainter *painter);

    CanvasSession *m_session = nullptr;
    TabletCanvasItem *m_surface = nullptr;
    QMetaObject::Connection m_docConn;
    QMetaObject::Connection m_camConn;
    QMetaObject::Connection m_toolConn;

    epaper::fingergesture::FingerGestureMachine m_finger;
    QElapsedTimer m_fingerPanClock;

    epaper::selection::SelectionSession m_selection;
    epaper::manip::ManipSession m_manip;

    QRectF m_liveDirtyPrev;
    QRectF m_originPanelRect;
    QRectF m_originConnPunch;
    QVector<OriginConnStroke> m_originConnStrokes;

    QElapsedTimer m_selectionGhostClock;
    int m_toolIntentSeq = 0;

    QString m_manipUnavailable;
    QRectF m_manipUnavailableRect;

    bool m_pinchIgnore = false;
    qreal m_pinchArm = 80.0;
    qreal m_pinchScale0 = 1.0;

    QRectF m_toolChromePrev;
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

    static constexpr qint64 kSelectionGhostMinIntervalMs = 200;
    static constexpr double kMinMarqueeGesture = 8.0;
};
