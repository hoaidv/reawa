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
#include "canvas_session.h"
#include "document/device_document.hpp"
#include "document/hand_touch.hpp"
#include "document/manipulate.hpp"
#include "finger_gesture_machine.hpp"
#include "input/pen_sample.hpp"
#include "manip_session.hpp"
#include "rendering/rendering.hpp"
#include "selection_session.hpp"
#include "tools/connector_recognizer_modifier.hpp"
#include "tools/ink_box_recognizer_modifier.hpp"
#include "tools/input_hub.hpp"
#include "tools/modes/pen_mode.hpp"
#include "tools/operations/ink_stroke_operation.hpp"
#include "tools/tablet_ink_sink.hpp"

#include <memory>

class TabletCanvasItem;

/**
 * ToolCanvas — pointer/finger interaction + selection chrome. Never blits the document.
 * @implements [SRS-EP-12] SelectionOverlay stroke chrome
 * @implements [ADR-0019] ToolCanvasLayer: Pen while lasso/marquee, Mono after pen-up
 */
class ToolCanvasItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(TabletCanvasItem *surface READ surface WRITE setSurface NOTIFY surfaceChanged)
    Q_PROPERTY(CanvasSession *session READ session WRITE setSession NOTIFY sessionChanged)
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

/**
 * =================================================================================================
 * Types
 * =================================================================================================
 */

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

/**
 * =================================================================================================
 * Construction and Qt item lifecycle
 * =================================================================================================
 */

public:
    explicit ToolCanvasItem(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;
    void setStrokeWaveform(bool penInFlight);

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

/**
 * =================================================================================================
 * Host wiring (composition / QML)
 *
 * Session and surface are injected separately. Tool never registers itself on Tablet.
 * cancelInteraction is QML (pen-near); selection gestures stay private.
 * =================================================================================================
 */

public:
    void setSession(CanvasSession *session);
    CanvasSession *session() const { return m_session; }

    void setSurface(TabletCanvasItem *surface);
    TabletCanvasItem *surface() const { return m_surface; }

    Q_INVOKABLE void cancelInteraction();

signals:
    void surfaceChanged();
    void sessionChanged();

private:
    void onDocumentOrCameraChanged();
    void clearSelection();
    void syncToolHost();
    void syncActiveMode();
    void feedInkStroke(QEvent::Type type, const PanelPt &panel, qreal pressure);

    QString exclusiveTool() const;
    epaper::document::DeviceDocument &doc();
    const epaper::document::DeviceDocument &doc() const;
    epaper::canvasframe::CanvasFrame &frame();
    const epaper::canvasframe::CanvasFrame &frame() const;

    CanvasSession *m_session = nullptr;
    TabletCanvasItem *m_surface = nullptr;
    epaper::tools::InputHub m_hub;
    epaper::tools::PenMode m_penMode;
    std::unique_ptr<epaper::tools::TabletInkSink> m_inkSink;
    std::unique_ptr<epaper::tools::InkStrokeOperation> m_inkStroke;
    std::unique_ptr<epaper::tools::InkBoxRecognizerModifier> m_inkBoxRecog;
    std::unique_ptr<epaper::tools::ConnectorRecognizerModifier> m_connRecog;
    QMetaObject::Connection m_docConn;
    QMetaObject::Connection m_camConn;
    QMetaObject::Connection m_toolConn;

/**
 * =================================================================================================
 * Canvas frame helpers (via session)
 * =================================================================================================
 */

private:
    void applyCameraRegion(const epaper::handtouch::WorldAabb &region, bool markValid);
    WorldPt panelToWorld(const PanelPt &panel) const;
    PanelPt worldToPanel(double wx, double wy) const;
    PanelPt worldToPanel(WorldPt w) const { return worldToPanel(w.x, w.y); }
    FrameUv panelToFrameUv(const PanelPt &panel) const;
    QRectF worldBoundsToPanel(const epaper::document::SmartBounds &wb) const;
    bool lodOkPanel(const epaper::document::SmartBounds &wb) const;
    epaper::handtouch::FollowDirection followEnum() const;

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
     * Qt DragHandler / PinchHandler canvas entry (ToolCanvas.qml).
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

private:
    PanelPt pinchArmPoint(qreal x, qreal y, qreal scale, bool positive) const;

    bool m_pinchIgnore = false;
    qreal m_pinchArm = 80.0;
    qreal m_pinchScale0 = 1.0;

/**
 * =================================================================================================
 * Hand touch
 *
 * Owns m_finger (FingerGestureMachine). Mutators return FingerIntent;
 * applyFingerIntent() alone performs session/Surface effects.
 *
 * Cycles:
 *      beginFingerTouch → updateFingerTouch → endFingerTouch;
 *      beginTwoFingerTouch → updateTwoFingerTouch → endTwoFingerTouch;
 *      cancelHandTouch as the escape hatch
 * =================================================================================================
 */

public:
    bool handTouchArmed() const { return m_finger.armed; }
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
     * Two-finger local pan/pinch (STORY-EP-039).
     * @implements [SRS-EP-24] two-finger canvas pan pinch
     */
    bool beginTwoFingerTouch(const PanelPt &a, const PanelPt &b);
    void updateTwoFingerTouch(const PanelPt &a, const PanelPt &b);
    void endTwoFingerTouch();

signals:
    void handTouchArmedChanged();

private:
    void applyFingerIntent(const epaper::fingergesture::FingerResult &r,
                           const PanelPt &panel = PanelPt());
    bool fingerHitsBox(const PanelPt &canvasPos) const;
    epaper::handtouch::TwoFingerContacts uvPair(const PanelPt &a, const PanelPt &b) const;
    void worldThroughPanOrigin(const PanelPt &panel, double *wx, double *wy) const;

    epaper::fingergesture::FingerGestureMachine m_finger;
    QElapsedTimer m_fingerPanClock;

/**
 * =================================================================================================
 * Selection and direct manipulation
 *
 * Owns m_selection (SelectionSession) and m_manip (ManipSession). Mutators return
 * intents; applySelectionIntent / applyManipIntent alone perform session/Surface effects.
 *
 * Sub-cycle A — gesture: beginSelectionGesture → updateSelectionGesture → endSelectionGesture
 * Sub-cycle B — marquee/lasso: beginMarqueeOrLasso → finishMarqueeOrLasso
 * Sub-cycle C — live manip: startLiveManip → applyDragWorld → redrawLiveManipRegion → commitLiveManip | abortFingerManip
 * Sub-cycle D — handle entry: handleIndexAtPanel → tryBeginHandleAtPanel → beginHandleDrag
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

signals:
    void selectionChromeChanged();

private:
    bool isSelectionTool() const;
    bool selectionToolArmed() const { return isSelectionTool(); }
    bool selectionGestureActive() const { return m_selection.active(); }
    void setSelection(const std::vector<std::string> &ids);

    QString hitLocalSmartGroup(WorldPt world) const;

    void applySelectionIntent(const epaper::selection::SelectionResult &r);
    void applyManipIntent(const epaper::manip::ManipResult &r, bool restoreOrigin = false);

    void beginSelectionGesture(const PanelPt &canvasPos);
    void updateSelectionGesture(const PanelPt &canvasPos);
    void endSelectionGesture();
    bool tryBeginHandleAtPanel(const PanelPt &panel, double hitDu);

    void beginMarqueeOrLasso(const PanelPt &canvasPos);
    void finishMarqueeOrLasso();

    void startLiveManip(const epaper::document::DocNode *subject,
                        epaper::document::ResizeHandle handle, WorldPt world);
    void applyDragWorld(WorldPt world);
    void redrawLiveManipRegion();
    void commitLiveManip();

    int handleIndexAtPanel(const PanelPt &panel, double hitDu) const;

    void showManipUnavailable(const epaper::document::SmartBounds &wb);
    void sendManipPreviewToInfini();

    epaper::selection::SelectionSession m_selection;
    epaper::manip::ManipSession m_manip;
    epaper::render::DocumentRenderer m_renderer;

    QRectF m_liveDirtyPrev;
    QRectF m_originPanelRect;

    QElapsedTimer m_selectionGhostClock;
    int m_toolIntentSeq = 0;

    QString m_manipUnavailable;
    QRectF m_manipUnavailableRect;

    static constexpr qint64 kSelectionGhostMinIntervalMs = 200;
    static constexpr double kMinMarqueeGesture = 8.0;

/**
 * =================================================================================================
 * Selection chrome overlay
 *
 * Cycle: refreshSelectionChrome → damageToolChrome/damageToolChromeSegment → paintToolChrome
 * =================================================================================================
 */

public:
    QRectF encloseCtaRect() const { return m_encloseCtaRect; }
    bool encloseVisible() const { return m_encloseVisible; }
    QString encloseRefuseReason() const { return m_encloseRefuseReason; }
    QRectF selectionBoundsRect() const { return m_selectionBoundsRect; }
    int handleCount() const { return m_handleCount; }
    qreal handleSize() const { return m_handleSize; }
    bool modeChipVisible() const { return m_modeChipVisible; }
    QString modeChipLabel() const { return m_modeChipLabel; }
    QRectF modeChipRectProp() const { return m_modeChipRect; }

private:
    void refreshSelectionChrome();
    void damageToolChrome(const QRectF &next);
    void damageToolChromeSegment(const QRectF &seg);
    void syncToolCanvasPresence();
    void paintLiveManipOnToolCanvas(QPainter *painter);
    void paintToolChrome(QPainter *painter);

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
};
