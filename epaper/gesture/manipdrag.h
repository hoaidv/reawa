#pragma once

/**
 * Live move/resize session in world space.
 * Canvas calls begin / setCurrentWorld / setLive / reset — not field poking.
 * @implements [SRS-EP-11] selection hit-test move resize
 * @implements [SRS-EP-12] ToolLayer handle drag
 */

#include "document/manipulate.hpp"

#include <QPointF>
#include <QString>

namespace epaper {
namespace gesture {

inline epaper::document::ResizeHandle handleFromIndex(int index)
{
    using epaper::document::ResizeHandle;
    static const ResizeHandle k[] = {ResizeHandle::Nw, ResizeHandle::N, ResizeHandle::Ne,
                                     ResizeHandle::E,  ResizeHandle::Se, ResizeHandle::S,
                                     ResizeHandle::Sw, ResizeHandle::W};
    if (index < 0 || index >= 8)
        return ResizeHandle::None;
    return k[index];
}

/** World-space drag; canvas maps panel→world once at the handler edge. */
class ManipDrag {
public:
    void begin(QString id, epaper::document::ResizeHandle handle, const QPointF &world,
               epaper::document::SmartTransform originT, epaper::document::SmartBounds originB)
    {
        *this = ManipDrag{};
        m_active = true;
        m_nodeId = std::move(id);
        m_handle = handle;
        m_originWorld = world;
        m_currentWorld = world;
        m_originT = originT;
        m_liveT = originT;
        m_originB = originB;
        m_liveB = originB;
    }

    void setCurrentWorld(const QPointF &world) { m_currentWorld = world; }
    void setLive(epaper::document::SmartTransform t, epaper::document::SmartBounds b)
    {
        m_liveT = t;
        m_liveB = b;
    }
    void reset() { *this = ManipDrag{}; }
    void clearNodeId() { m_nodeId.clear(); }

    bool active() const { return m_active; }
    bool resizing() const { return m_handle != epaper::document::ResizeHandle::None; }
    QString nodeId() const { return m_nodeId; }
    epaper::document::ResizeHandle handle() const { return m_handle; }
    QPointF originWorld() const { return m_originWorld; }
    QPointF currentWorld() const { return m_currentWorld; }
    QPointF deltaWorld() const { return m_currentWorld - m_originWorld; }
    epaper::document::SmartTransform originT() const { return m_originT; }
    epaper::document::SmartTransform liveT() const { return m_liveT; }
    epaper::document::SmartBounds originB() const { return m_originB; }
    epaper::document::SmartBounds liveB() const { return m_liveB; }

private:
    bool m_active = false;
    QString m_nodeId;
    epaper::document::ResizeHandle m_handle = epaper::document::ResizeHandle::None;
    QPointF m_originWorld;
    QPointF m_currentWorld;
    epaper::document::SmartTransform m_originT;
    epaper::document::SmartTransform m_liveT;
    epaper::document::SmartBounds m_originB;
    epaper::document::SmartBounds m_liveB;
};

} // namespace gesture
} // namespace epaper
