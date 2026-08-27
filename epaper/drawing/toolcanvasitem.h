#pragma once

#include <QQuickPaintedItem>
#include <QRectF>
#include <QString>

#include <QPainter>
#include <memory>

#include "tools/input_hub.hpp"
#include "tools/modes/pen_mode.hpp"
#include "tools/modes/selection_mode.hpp"
#include "tools/selection_context_host.hpp"
#include "tools/session_doc_context.hpp"
#include "tools/tablet_ink_sink.hpp"
#include "tools/tool_canvas_context.hpp"
#include "tools/connector_recognizer_modifier.hpp"
#include "tools/ink_box_recognizer_modifier.hpp"

#include "selection_session.hpp"

class CanvasSession;
class TabletCanvasItem;

/**
 * ToolCanvas — Qt router host; Q_INVOKABLE forwards to InputHub.
 * @implements [SRS-EP-12] @implements [ADR-0019] @implements [ADR-0033]
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

public:
    using PanelPt = QPointF;

    explicit ToolCanvasItem(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;
    void setStrokeWaveform(bool penInFlight);

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    void setSession(CanvasSession *session);
    CanvasSession *session() const { return m_session; }

    void setSurface(TabletCanvasItem *surface);
    TabletCanvasItem *surface() const { return m_surface; }

    Q_INVOKABLE void cancelInteraction();

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

    bool handTouchArmed() const { return m_hub.handTouch().armed(); }
    Q_INVOKABLE void toggleHandTouch();
    Q_INVOKABLE void cancelHandTouch();

    Q_INVOKABLE void encloseSelection();
    Q_INVOKABLE void tapModeChip();

    QRectF encloseCtaRect() const;
    bool encloseVisible() const;
    QString encloseRefuseReason() const;
    QRectF selectionBoundsRect() const;
    int handleCount() const;
    qreal handleSize() const;
    bool modeChipVisible() const;
    QString modeChipLabel() const;
    QRectF modeChipRectProp() const;
    QString manipulationUnavailable() const;
    QRectF manipulationUnavailableRect() const;

signals:
    void surfaceChanged();
    void sessionChanged();
    void handTouchArmedChanged();
    void selectionChromeChanged();

private:
    void syncToolHost();
    void syncActiveMode();
    void registerOperations();
    void onDocumentOrCameraChanged();
    epaper::tools::PointerSample sample(qreal x, qreal y, qreal pressure, bool pen) const;

    CanvasSession *m_session = nullptr;
    TabletCanvasItem *m_surface = nullptr;
    epaper::tools::InputHub m_hub;
    epaper::tools::PenMode m_penMode;
    epaper::tools::SelectionMode m_selectionMode;
    epaper::tools::SelectionContextHost m_selCtx;
    epaper::selection::SelectionSession m_selection;
    std::unique_ptr<epaper::tools::TabletInkSink> m_inkSink;
    std::unique_ptr<epaper::tools::SessionDocContext> m_docCtx;
    std::unique_ptr<epaper::tools::ToolCanvasContext> m_toolCtx;
    std::unique_ptr<epaper::tools::InkBoxRecognizerModifier> m_inkBoxRecog;
    std::unique_ptr<epaper::tools::ConnectorRecognizerModifier> m_connRecog;
    QMetaObject::Connection m_docConn;
    QMetaObject::Connection m_camConn;
    QMetaObject::Connection m_toolConn;
};
