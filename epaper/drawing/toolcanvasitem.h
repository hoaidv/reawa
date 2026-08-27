#pragma once

#include <QQuickPaintedItem>
#include <QString>

#include "tools/input_hub.hpp"
#include "tools/modes/pen_mode.hpp"
#include "tools/modes/selection_mode.hpp"
#include "tools/contexts/selection_context.hpp"
#include "tools/contexts/session_doc_context.hpp"
#include "tools/tablet_ink_sink.hpp"
#include "tools/contexts/tool_canvas_context.hpp"
#include "tools/ui/selection_context_bar.hpp"

#include <QPainter>
#include <memory>

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
    Q_PROPERTY(epaper::tools::SelectionContextBar *selectionBar READ selectionBar CONSTANT)

public:
    using PanelPt = QPointF;

    explicit ToolCanvasItem(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;
    void setStrokeWaveform(bool penInFlight);
    epaper::tools::SelectionContextBar *selectionBar() { return &m_selBar; }

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

signals:
    void surfaceChanged();
    void sessionChanged();
    void handTouchArmedChanged();
    void selectionChromeChanged();

private:
    void syncToolHost();
    void syncActiveMode();
    void registerOperations();
    void registerInterventions();
    epaper::tools::PointerSample sample(qreal x, qreal y, qreal pressure, bool pen) const;

    CanvasSession *m_session = nullptr;
    TabletCanvasItem *m_surface = nullptr;
    epaper::tools::InputHub m_hub;
    epaper::tools::PenMode m_penMode;
    epaper::tools::SelectionMode m_selectionMode;
    epaper::tools::SelectionContext m_selCtx;
    epaper::tools::SelectionContextBar m_selBar;
    std::unique_ptr<epaper::tools::TabletInkSink> m_inkSink;
    std::unique_ptr<epaper::tools::SessionDocContext> m_docCtx;
    std::unique_ptr<epaper::tools::ToolCanvasContext> m_toolCtx;
    QMetaObject::Connection m_docConn;
    QMetaObject::Connection m_camConn;
    QMetaObject::Connection m_toolConn;
};
