#pragma once

/**
 * SelectionContextBar — owns ToolActions and publishes chrome for QML.
 * @implements [SRS-EP-12] @implements [ADR-0033]
 * @implements [SRS-EP-32] Selected strip + tap-origin paste
 */

#include "action_list_model.hpp"
#include "../actions/copy_action.hpp"
#include "../actions/cut_action.hpp"
#include "../actions/enclose_action.hpp"
#include "../actions/ink_scale_action.hpp"
#include "../actions/paste_action.hpp"
#include "../contexts/selection_context.hpp"
#include "../host_caps.hpp"
#include "selection_overlay.hpp"

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <memory>
#include <vector>

namespace epaper {
namespace tools {

class SelectionContextBar final : public QObject {
    Q_OBJECT
    Q_PROPERTY(ActionListModel *actions READ actions CONSTANT)
    Q_PROPERTY(QRectF selectionBounds READ selectionBounds NOTIFY chromeChanged)
    Q_PROPERTY(int handleCount READ handleCount NOTIFY chromeChanged)
    Q_PROPERTY(qreal handleSize READ handleSize NOTIFY chromeChanged)
    Q_PROPERTY(QString refuseReason READ refuseReason NOTIFY chromeChanged)
    Q_PROPERTY(QString manipUnavailable READ manipUnavailable NOTIFY chromeChanged)
    Q_PROPERTY(QRectF manipUnavailableRect READ manipUnavailableRect NOTIFY chromeChanged)
    Q_PROPERTY(bool tapPasteAtOrigin READ tapPasteAtOrigin NOTIFY chromeChanged)
    Q_PROPERTY(QPointF tapOrigin READ tapOrigin NOTIFY chromeChanged)
public:
    explicit SelectionContextBar(QObject *parent = nullptr)
        : QObject(parent)
        , m_model(new ActionListModel(this))
    {
        m_owned.push_back(&m_enclose);
        m_owned.push_back(&m_inkScale);
        m_owned.push_back(&m_cut);
        m_owned.push_back(&m_copy);
        m_owned.push_back(&m_paste);
    }

    ActionListModel *actions() const { return m_model; }
    QRectF selectionBounds() const { return m_bounds; }
    int handleCount() const { return m_handleCount; }
    qreal handleSize() const { return m_handleSize; }
    QString refuseReason() const { return m_refuse; }
    QString manipUnavailable() const { return m_manip; }
    QRectF manipUnavailableRect() const { return m_manipRect; }
    bool tapPasteAtOrigin() const { return m_tapOriginValid && m_bounds.width() <= 0; }
    QPointF tapOrigin() const { return m_tapOrigin; }

    void refresh(HostCaps &caps, const SelectionOverlayState &chrome)
    {
        m_caps = &caps;
        m_bounds = chrome.selectionBoundsRect;
        m_handleCount = chrome.handleCount;
        m_handleSize = chrome.handleSize;
        m_refuse = chrome.encloseRefuseReason;
        m_manip = chrome.manipUnavailable;
        m_manipRect = chrome.manipUnavailableRect;
        m_tapOriginValid = caps.selection && caps.selection->phase() == SelectionPhase::Idle
            && caps.selection->ids().empty() && caps.selection->pasteOriginValid();
        if (m_tapOriginValid) {
            const PasteOrigin &o = caps.selection->pasteOrigin();
            m_tapOrigin = QPointF(o.panelX, o.panelY);
        } else {
            m_tapOrigin = QPointF();
        }
        m_model->rebuild(m_owned, caps);
        emit chromeChanged();
    }

    Q_INVOKABLE void trigger(const QString &id)
    {
        if (!m_caps)
            return;
        for (ToolAction *a : m_owned) {
            if (a && a->id() == id) {
                a->trigger(*m_caps);
                return;
            }
        }
    }

signals:
    void chromeChanged();

private:
    HostCaps *m_caps = nullptr;
    ActionListModel *m_model = nullptr;
    EncloseAction m_enclose;
    InkScaleAction m_inkScale;
    CutAction m_cut;
    CopyAction m_copy;
    PasteAction m_paste;
    std::vector<ToolAction *> m_owned;
    QRectF m_bounds;
    int m_handleCount = 0;
    qreal m_handleSize = 16.0;
    QString m_refuse;
    QString m_manip;
    QRectF m_manipRect;
    bool m_tapOriginValid = false;
    QPointF m_tapOrigin;
};

} // namespace tools
} // namespace epaper
