#pragma once

/**
 * NodeEmphasis — ToolCanvas blink / stroke-stamp / AABB highlight.
 * Host-owned; not SelectionOverlay; not inlined into ToolCanvasItem::paint.
 * @implements [SRS-EP-12] @implements [CHL-0030]
 */

#include "../host_caps.hpp"

#include <QObject>
#include <QRectF>
#include <QString>

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class QPainter;

namespace epaper {
namespace tools {

enum class StrokeStamp {
    Off = 0,
    Bold = 1,
    Dotted = 2, // reserved (deletion look); unused in product this slice
};

class NodeEmphasis {
public:
    void setTimerHost(QObject *host) { m_timerHost = host; }

    bool active() const
    {
        return !m_blinkIds.empty() || !m_stampIds.empty() || !m_aabbRects.empty();
    }

    QRectF dirtyUnion() const { return m_dirty; }

    void blink(HostCaps &caps, const std::vector<std::string> &ids, int ms = 250);
    void setStrokeStamp(HostCaps &caps, const std::vector<std::string> &ids, StrokeStamp stamp);
    void clearStrokeStamp(HostCaps &caps);
    void showAabb(HostCaps &caps, const std::string &nodeId);
    void showAabb(HostCaps &caps, const QString &nodeId, const QRectF &panelRect);
    void hideAabb(HostCaps &caps, const std::string &nodeId);
    void hideAllAabbs(HostCaps &caps);
    void recompute(HostCaps &caps);
    void paint(QPainter *painter, HostCaps &caps);

private:
    QRectF computeDirty(HostCaps &caps) const;
    void damage(HostCaps &caps, const QRectF &prev);
    QRectF panelBound(HostCaps &caps, const std::string &id) const;

    QObject *m_timerHost = nullptr;
    HostCaps m_caps;
    int m_blinkToken = 0;
    std::unordered_set<std::string> m_blinkIds;
    std::unordered_set<std::string> m_stampIds;
    StrokeStamp m_stamp = StrokeStamp::Off;
    std::unordered_map<std::string, QRectF> m_aabbRects;
    QRectF m_dirty;
};

} // namespace tools
} // namespace epaper
