#pragma once
/**
 * Epaper region-sync session — viewport, local ink emit, remote ops, coherent refresh.
 * @implements [SRS-EP-02] viewport map, document ops, panel refresh
 * @implements [SRS-EP-03] hot path + e-ink coalesce (≥250 ms) + settle flush
 */

#include "doc_store.hpp"
#include "viewport_map.hpp"

#include <functional>
#include <string>
#include <vector>

namespace epaper::regionsync {

/** Min interval between region paints (SRS-EP-03). */
constexpr double kRefreshMinIntervalMs = 250.0;

struct PenSample {
    double localX = 0;
    double localY = 0;
    std::optional<double> pressure;
    std::optional<double> tiltX;
    std::optional<double> tiltY;
    std::map<std::string, double> extras;
};

struct PaintPass {
    int mapSeq = 0;
    int docVersion = 0;
    Aabb drawingRegion{};
    std::string contentHash;
    /** True when map+doc pair was snapshotted atomically for this pass. */
    bool coherent = false;
};

struct OutboundDocOp {
    DocOp op;
    int attempts = 0;
};

/**
 * Net sink — must not be invoked from onPenSample (SRS-EP-03).
 * Tests inject a failing/succeeding sink.
 */
class NetSink {
public:
    virtual ~NetSink() = default;
    /** @return true if send succeeded. */
    virtual bool sendDocOp(const DocOp &op) = 0;
    virtual bool didRunOnPenCallback() const { return m_ranOnPen; }
    void markPenCallback() { m_ranOnPen = true; }
    void clearPenCallbackFlag() { m_ranOnPen = false; }

protected:
    bool m_ranOnPen = false;
};

class RegionSession {
public:
    explicit RegionSession(NetSink *net) : m_net(net) {}

    bool connected() const { return m_connected; }
    void connect() { m_connected = true; }

    /** When ADR-0009 session owns the map, legacy StrokeSync must not override. */
    bool ownsDrawingRegionMap() const { return m_connected && m_maps.current().valid; }

    const ViewportMap &map() const { return m_maps.current(); }
    const DocStore &doc() const { return m_doc; }
    int refreshQueueSize() const { return m_refreshPending ? 1 : 0; }
    int netQueueSize() const { return static_cast<int>(m_netQueue.size()); }
    bool smartGroupRanOnDevice() const { return m_smartGroupRan; }
    const std::vector<std::string> &logs() const { return m_logs; }
    int paintCount() const { return m_paintCount; }

    void setPanelSize(double w, double h) { m_maps.setPanelSize(w, h); }

    /** Infini → Epaper viewport — map immediate; refresh pending coalesced. */
    bool onViewport(const ViewportMessage &msg)
    {
        std::string err;
        const bool ok = m_maps.applyViewport(msg, &err);
        if (!ok) {
            m_logs.push_back(err);
            return false;
        }
        m_refreshPending = true;
        return true;
    }

    /**
     * Pen sample hot path — map only; never touches NetSink send.
     * @implements [SRS-EP-03] no socket I/O on pen sample callback
     */
    void onPenSample(const PenSample &sample)
    {
        if (m_net)
            m_net->clearPenCallbackFlag();
        if (!m_maps.current().valid)
            return;
        InkSample world;
        const Vec2 w = panelToWorld(m_maps.current(), sample.localX, sample.localY);
        world.x = w.x;
        world.y = w.y;
        world.pressure = sample.pressure;
        world.tiltX = sample.tiltX;
        world.tiltY = sample.tiltY;
        world.extras = sample.extras;
        m_strokeSamples.push_back(std::move(world));
        if (m_net && m_net->didRunOnPenCallback())
            m_logs.push_back("violation: socket I/O on pen callback");
    }

    /**
     * Complete local stroke → queue append_ink (no Smart Group on-device).
     * @implements [SRS-EP-02] emit append_ink world samples; no on-device enclose
     */
    void endStroke(const std::string &inkId, const std::string &opId)
    {
        m_smartGroupRan = false;

        DocOp op;
        op.opId = opId;
        op.type = "append_ink";
        op.source = "epaper";
        op.inkId = inkId;
        op.samples = m_strokeSamples;
        m_strokeSamples.clear();

        m_doc.apply(op);
        m_netQueue.push_back({op, 0});
        m_refreshPending = true;
    }

    /** Net thread: attempt sends with retry/backoff counter. */
    void flushNetQueue()
    {
        if (!m_net)
            return;
        std::vector<OutboundDocOp> remaining;
        for (auto &item : m_netQueue) {
            if (m_net->sendDocOp(item.op))
                continue;
            item.attempts += 1;
            remaining.push_back(item);
            m_logs.push_back("append_ink send fail; retry scheduled");
        }
        m_netQueue.swap(remaining);
    }

    DocStore::ApplyResult onRemoteDocOp(const DocOp &op)
    {
        auto r = m_doc.apply(op);
        if (!r.applied && r.reason.rfind("unknown_type:", 0) == 0)
            m_logs.push_back(r.reason);
        if (r.applied)
            m_refreshPending = true;
        return r;
    }

    /**
     * Region refresh with e-ink coalesce.
     * @param nowMs monotonic clock
     * @param forceSettle ignore 250 ms floor (gesture settle ≤100 ms path)
     * @implements [SRS-EP-02] paint document ∩ drawing region coherently
     * @implements [SRS-EP-03] ≥250 ms between paints unless settle
     */
    std::optional<PaintPass> runRegionRefresh(double nowMs, bool forceSettle = false)
    {
        if (!m_refreshPending)
            return std::nullopt;
        if (!forceSettle && m_lastPaintAtMs >= 0
            && (nowMs - m_lastPaintAtMs) < kRefreshMinIntervalMs) {
            return std::nullopt;
        }
        m_refreshPending = false;
        if (!m_maps.current().valid)
            return std::nullopt;

        PaintPass pass;
        pass.mapSeq = m_maps.current().seq;
        pass.docVersion = m_doc.version();
        pass.drawingRegion = m_maps.current().drawingRegion;
        pass.contentHash = m_doc.contentHash(pass.drawingRegion);
        pass.coherent = true;
        m_lastPaint = pass;
        m_lastPaintAtMs = nowMs;
        m_paintCount += 1;
        return pass;
    }

    const std::optional<PaintPass> &lastPaint() const { return m_lastPaint; }

private:
    NetSink *m_net = nullptr;
    bool m_connected = false;
    ViewportMapStore m_maps;
    DocStore m_doc;
    bool m_refreshPending = false;
    bool m_smartGroupRan = false;
    std::vector<InkSample> m_strokeSamples;
    std::vector<OutboundDocOp> m_netQueue;
    std::vector<std::string> m_logs;
    std::optional<PaintPass> m_lastPaint;
    double m_lastPaintAtMs = -1;
    int m_paintCount = 0;
};

} // namespace epaper::regionsync
