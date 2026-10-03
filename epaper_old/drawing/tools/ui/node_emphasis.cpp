#include "node_emphasis.hpp"

#include "../contexts/session_doc_context.hpp"
#include "../contexts/tool_context.hpp"
#include "document/manipulate.hpp"
#include "document/nested_inkbox.hpp"
#include "rendering/rendering.hpp"
#include "rendering/rendering_qt.hpp"

#include <QPainter>
#include <QPen>
#include <QTimer>
#include <algorithm>
#include <memory>

namespace epaper {
namespace tools {

namespace {

SessionDocContext *sessionDoc(HostCaps &caps)
{
    return dynamic_cast<SessionDocContext *>(caps.doc);
}

} // namespace

QRectF NodeEmphasis::panelBound(HostCaps &caps, const std::string &id) const
{
    SessionDocContext *sess = sessionDoc(caps);
    if (!sess || !caps.toolUi)
        return {};
    const epaper::document::DocNode *n = sess->document().find(id);
    if (!n)
        return {};
    epaper::document::SmartBounds b;
    if (!epaper::document::composedBoundsOf(sess->document(), *n, b))
        return {};
    const qreal pad = std::max(16.0, 8.0 * caps.toolUi->panelScale());
    return caps.toolUi->worldBoundsToPanel(b).adjusted(-pad, -pad, pad, pad);
}

QRectF NodeEmphasis::computeDirty(HostCaps &caps) const
{
    QRectF u;
    auto acc = [&](const QRectF &r) {
        if (r.isEmpty())
            return;
        u = u.isEmpty() ? r : u.united(r);
    };
    for (const auto &id : m_blinkIds)
        acc(panelBound(caps, id));
    for (const auto &id : m_stampIds)
        acc(panelBound(caps, id));
    for (const auto &kv : m_aabbRects)
        acc(kv.second);
    return u;
}

void NodeEmphasis::damage(HostCaps &caps, const QRectF &prev)
{
    if (!caps.toolUi)
        return;
    m_dirty = computeDirty(caps);
    QRectF u = m_dirty;
    if (!prev.isEmpty())
        u = u.isEmpty() ? prev : u.united(prev);
    if (!u.isEmpty())
        caps.toolUi->damageChrome(u);
}

void NodeEmphasis::recompute(HostCaps &caps)
{
    m_dirty = computeDirty(caps);
}

void NodeEmphasis::blink(HostCaps &caps, const std::vector<std::string> &ids, int ms)
{
    m_caps = caps;
    const QRectF prev = m_dirty;
    m_blinkIds.clear();
    for (const auto &id : ids) {
        if (!id.empty())
            m_blinkIds.insert(id);
    }
    ++m_blinkToken;
    const int token = m_blinkToken;
    if (caps.toolUi) {
        caps.toolUi->setOverlayVisible(true);
        caps.toolUi->setStrokeWaveform(false); // [D14] Mono; never steal Pen
    }
    damage(caps, prev);
    if (!m_timerHost)
        return;
    QTimer::singleShot(ms, m_timerHost, [this, token]() {
        if (token != m_blinkToken)
            return;
        const QRectF prev = m_dirty;
        m_blinkIds.clear();
        damage(m_caps, prev);
        if (m_caps.toolUi)
            m_caps.toolUi->syncOverlayPresence();
    });
}

void NodeEmphasis::setStrokeStamp(HostCaps &caps, const std::vector<std::string> &ids,
                                  StrokeStamp stamp)
{
    std::unordered_set<std::string> next;
    if (stamp != StrokeStamp::Off) {
        for (const auto &id : ids) {
            if (!id.empty())
                next.insert(id);
        }
    }
    if (stamp == m_stamp && next == m_stampIds)
        return;
    const QRectF prev = m_dirty;
    m_stampIds = std::move(next);
    m_stamp = stamp;
    if (stamp == StrokeStamp::Off) {
        damage(caps, prev);
        return;
    }
    if (caps.toolUi) {
        caps.toolUi->setOverlayVisible(true);
        caps.toolUi->setStrokeWaveform(false);
    }
    damage(caps, prev);
}

void NodeEmphasis::clearStrokeStamp(HostCaps &caps)
{
    if (m_stampIds.empty() && m_stamp == StrokeStamp::Off)
        return;
    const QRectF prev = m_dirty;
    m_stampIds.clear();
    m_stamp = StrokeStamp::Off;
    damage(caps, prev);
    if (caps.toolUi)
        caps.toolUi->syncOverlayPresence();
}

void NodeEmphasis::showAabb(HostCaps &caps, const std::string &id)
{
    showAabb(caps, QString::fromStdString(id), panelBound(caps, id));
}

void NodeEmphasis::showAabb(HostCaps &caps, const QString &nodeId, const QRectF &panelRect)
{
    const QRectF prev = m_dirty;
    m_aabbRects[nodeId.toStdString()] = panelRect;
    if (caps.toolUi) {
        caps.toolUi->setOverlayVisible(true);
        if (!panelRect.isEmpty()) {
            const QRectF r = panelRect.toAlignedRect();
            caps.toolUi->damageChromeSegment(QRectF(r.left(), r.top(), r.width(), 4));
            caps.toolUi->damageChromeSegment(QRectF(r.left(), r.bottom() - 4, r.width(), 4));
            caps.toolUi->damageChromeSegment(QRectF(r.left(), r.top(), 4, r.height()));
            caps.toolUi->damageChromeSegment(QRectF(r.right() - 4, r.top(), 4, r.height()));
        }
    }
    m_dirty = computeDirty(caps);
    (void)prev;
}

void NodeEmphasis::hideAabb(HostCaps &caps, const std::string &id)
{
    const auto it = m_aabbRects.find(id);
    if (it == m_aabbRects.end())
        return;
    const QRectF prev = it->second;
    m_aabbRects.erase(it);
    damage(caps, prev);
}

void NodeEmphasis::hideAllAabbs(HostCaps &caps)
{
    if (m_aabbRects.empty())
        return;
    const QRectF prev = m_dirty;
    m_aabbRects.clear();
    damage(caps, prev);
}

void NodeEmphasis::paint(QPainter *painter, HostCaps &caps)
{
    if (!painter || !active())
        return;
    SessionDocContext *sess = sessionDoc(caps);
    if (!sess)
        return;

    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    epaper::render::FrameProjector proj;
    proj.frame = &sess->frame();
    epaper::render::RenderRequest req;
    req.sharp = true;
    // Only the emphasized ids — never re-stroke neighbors in Mono (dash + hitch).
    for (const auto &id : m_blinkIds) {
        req.includeIds.insert(id);
        req.styles[id] = epaper::render::StyleOverride{2.0};
    }
    const double stampMul = m_stamp == StrokeStamp::Off ? 1.0 : 2.0;
    for (const auto &id : m_stampIds) {
        req.includeIds.insert(id);
        req.styles[id] = epaper::render::StyleOverride{stampMul};
    }
    if (!m_dirty.isEmpty() && caps.toolUi) {
        const auto tl = caps.toolUi->panelToWorld(m_dirty.topLeft());
        const auto br = caps.toolUi->panelToWorld(m_dirty.bottomRight());
        epaper::render::WorldAabb w;
        w.minX = std::min(tl.x, br.x);
        w.maxX = std::max(tl.x, br.x);
        w.minY = std::min(tl.y, br.y);
        w.maxY = std::max(tl.y, br.y);
        req.worldClip = w;
    }

    if (!req.includeIds.empty()) {
        epaper::render::DocumentRenderer renderer;
        renderer.setAlgorithm(std::make_unique<epaper::render::HierarchyCullAlgorithm>());
        epaper::render::QPainterPixelSink sink(painter);
        renderer.render(sess->document(), proj, req, sink);
    }

    if (m_stamp == StrokeStamp::Dotted && !m_stampIds.empty()) {
        QPen dotted(Qt::black);
        dotted.setWidthF(3.0);
        dotted.setStyle(Qt::DotLine);
        painter->setPen(dotted);
        painter->setBrush(Qt::NoBrush);
        for (const auto &id : m_stampIds) {
            const QRectF r = panelBound(caps, id);
            if (!r.isEmpty())
                painter->drawRect(r);
        }
    }

    QPen aabbPen(Qt::black);
    aabbPen.setWidthF(3.0);
    aabbPen.setCosmetic(true);
    aabbPen.setStyle(Qt::DotLine);
    painter->setPen(aabbPen);
    painter->setBrush(Qt::NoBrush);
    for (const auto &kv : m_aabbRects) {
        if (!kv.second.isEmpty())
            painter->drawRect(kv.second);
    }

    painter->restore();
}

} // namespace tools
} // namespace epaper
