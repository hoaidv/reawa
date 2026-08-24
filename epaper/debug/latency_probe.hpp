#pragma once
/**
 * Resident-document latency probe — stub fixture + hit-test, not a production tree.
 *
 * @implements [SRS-EP-13] 500-node / 50k-sample ink-latency probe
 * @implements [STORY-EP-013] measure ink with resident stub
 *
 * Gated: production paint stays clean unless RM_DOC_PROBE=1 (or Harness::enable()).
 * Hit-test and timing live on the ingest path, never inside paint().
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace epaper {
namespace latencyprobe {

constexpr int kNodeCount = 500;
constexpr int kSamplesPerNode = 100;
constexpr int kSampleCount = kNodeCount * kSamplesPerNode; // 50_000
/** Panel-ish world the fixture is scattered across (RM2 1404×1872). */
constexpr float kWorldW = 1404.f;
constexpr float kWorldH = 1872.f;
/** Hit radius in world units (polyline distance). */
constexpr float kHitRadius = 6.f;

struct Sample {
    float x = 0;
    float y = 0;
};

struct InkNode {
    std::uint32_t id = 0;
    float minX = 0;
    float minY = 0;
    float maxX = 0;
    float maxY = 0;
    std::uint32_t sampleOffset = 0;
    std::uint32_t sampleCount = 0;
};

struct Percentiles {
    int n = 0;
    std::int64_t p50Ns = -1;
    std::int64_t p95Ns = -1;
    std::int64_t p99Ns = -1;
};

inline std::int64_t percentileNs(std::vector<std::int64_t> samples, double p)
{
    if (samples.empty())
        return -1;
    std::sort(samples.begin(), samples.end());
    const int idx = std::max(0, std::min(int(p * (samples.size() - 1)), int(samples.size()) - 1));
    return samples[std::size_t(idx)];
}

inline Percentiles summarizeNs(const std::vector<std::int64_t> &samples)
{
    Percentiles out;
    out.n = int(samples.size());
    out.p50Ns = percentileNs(samples, 0.50);
    out.p95Ns = percentileNs(samples, 0.95);
    out.p99Ns = percentileNs(samples, 0.99);
    return out;
}

/** µs, rounded half-up — for comparing to SRS-EP-01 arrival→flush dumps. */
inline std::int64_t nsToUs(std::int64_t ns)
{
    if (ns < 0)
        return -1;
    return (ns + 500) / 1000;
}

inline float dist2ToSegment(float px, float py, float ax, float ay, float bx, float by)
{
    const float dx = bx - ax;
    const float dy = by - ay;
    const float len2 = dx * dx + dy * dy;
    float t = 0.f;
    if (len2 > 1e-12f)
        t = ((px - ax) * dx + (py - ay) * dy) / len2;
    if (t < 0.f)
        t = 0.f;
    else if (t > 1.f)
        t = 1.f;
    const float qx = ax + t * dx;
    const float qy = ay + t * dy;
    const float ex = px - qx;
    const float ey = py - qy;
    return ex * ex + ey * ey;
}

/**
 * In-memory ink-node fixture. Not DeviceDocument; not a writer; not undo/sync.
 * @implements [SRS-EP-13] document-scale fixture (500 / 50k)
 */
class StubDocument {
public:
    void fill()
    {
        m_nodes.clear();
        m_samples.clear();
        m_nodes.reserve(kNodeCount);
        m_samples.reserve(kSampleCount);

        // Deterministic LCG so host and device share the same geometry.
        std::uint32_t rng = 0xC0FFEEu;
        auto rnd = [&]() -> std::uint32_t {
            rng = rng * 1664525u + 1013904223u;
            return rng;
        };
        auto unit = [&]() -> float { return float(rnd() >> 8) * (1.f / 16777216.f); };

        const int cols = 20;
        const int rows = 25; // 500 cells
        const float cellW = kWorldW / float(cols);
        const float cellH = kWorldH / float(rows);

        for (int i = 0; i < kNodeCount; ++i) {
            const int col = i % cols;
            const int row = i / cols;
            const float ox = (float(col) + 0.15f + 0.2f * unit()) * cellW;
            const float oy = (float(row) + 0.15f + 0.2f * unit()) * cellH;
            InkNode node;
            node.id = std::uint32_t(i);
            node.sampleOffset = std::uint32_t(m_samples.size());
            node.sampleCount = std::uint32_t(kSamplesPerNode);

            float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
            float x = ox;
            float y = oy;
            for (int s = 0; s < kSamplesPerNode; ++s) {
                x += (unit() - 0.45f) * 2.4f;
                y += (unit() - 0.35f) * 2.8f;
                m_samples.push_back({x, y});
                minX = std::min(minX, x);
                minY = std::min(minY, y);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
            }
            node.minX = minX - kHitRadius;
            node.minY = minY - kHitRadius;
            node.maxX = maxX + kHitRadius;
            node.maxY = maxY + kHitRadius;
            m_nodes.push_back(node);
        }
    }

    int nodeCount() const { return int(m_nodes.size()); }
    int sampleCount() const { return int(m_samples.size()); }
    const std::vector<InkNode> &nodes() const { return m_nodes; }
    const std::vector<Sample> &samples() const { return m_samples; }

    /** Touch one node AABB so the fixture stays in the working set (ingest path). */
    float touchResident(int sampleIndex) const
    {
        if (m_nodes.empty())
            return 0.f;
        const InkNode &n = m_nodes[std::size_t(sampleIndex) % m_nodes.size()];
        return n.minX + n.maxY;
    }

    /**
     * Topmost-first (reverse index) AABB cull, then polyline distance.
     * @implements [SRS-EP-13] hit-test probe on the ingest path
     * @return node id, or -1 on miss
     */
    int hitTest(float px, float py) const
    {
        const float r2 = kHitRadius * kHitRadius;
        for (int i = int(m_nodes.size()) - 1; i >= 0; --i) {
            const InkNode &n = m_nodes[std::size_t(i)];
            if (px < n.minX || px > n.maxX || py < n.minY || py > n.maxY)
                continue;
            const Sample *s = m_samples.data() + n.sampleOffset;
            const int count = int(n.sampleCount);
            if (count <= 0)
                continue;
            if (count == 1) {
                const float dx = px - s[0].x;
                const float dy = py - s[0].y;
                if (dx * dx + dy * dy <= r2)
                    return int(n.id);
                continue;
            }
            for (int k = 1; k < count; ++k) {
                if (dist2ToSegment(px, py, s[k - 1].x, s[k - 1].y, s[k].x, s[k].y) <= r2)
                    return int(n.id);
            }
        }
        return -1;
    }

    /** Known-hit query: midpoint of a node's first segment. */
    Sample knownHitPoint(int nodeIndex) const
    {
        const InkNode &n = m_nodes[std::size_t(nodeIndex)];
        const Sample a = m_samples[n.sampleOffset];
        const Sample b = m_samples[n.sampleOffset + 1];
        return {(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f};
    }

private:
    std::vector<InkNode> m_nodes;
    std::vector<Sample> m_samples;
};

/**
 * Measurement harness. Allocate once; record outside the paint loop.
 * @implements [SRS-EP-13] instrumentation outside the paint loop
 */
class Harness {
public:
    void enable()
    {
        if (m_enabled)
            return;
        m_doc.fill();
        m_enabled = true;
        m_hitNs.clear();
        m_hitNs.reserve(4096);
        m_samplesSeen = 0;
        m_samplesDropped = 0;
        m_paintLoopHits = 0;
        m_sink = 0.f;
    }

    void setEverySample(bool on) { m_everySample = on; }

    bool enabled() const { return m_enabled; }
    bool everySample() const { return m_everySample; }
    const StubDocument &document() const { return m_doc; }
    std::int64_t samplesSeen() const { return m_samplesSeen; }
    std::int64_t samplesDropped() const { return m_samplesDropped; }
    std::int64_t paintLoopHits() const { return m_paintLoopHits; }
    const std::vector<std::int64_t> &hitNs() const { return m_hitNs; }

    /** Tests (and only tests) mark the simulated paint() scope. */
    void enterPaint() { m_inPaint = true; }
    void leavePaint() { m_inPaint = false; }
    bool inPaint() const { return m_inPaint; }

    /**
     * Ingest-path hook. Never skips the caller; never allocates after enable().
     * @param press  pen-down (always hit-tests) vs move/release
     */
    int onIngest(float x, float y, bool press)
    {
        if (!m_enabled) {
            ++m_samplesDropped;
            return -1;
        }
        if (m_inPaint)
            ++m_paintLoopHits;

        ++m_samplesSeen;
        m_sink += m_doc.touchResident(int(m_samplesSeen));

        const bool runHit = press || m_everySample;
        if (!runHit)
            return -1;

        const auto t0 = std::chrono::steady_clock::now();
        const int id = m_doc.hitTest(x, y);
        const auto t1 = std::chrono::steady_clock::now();
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        m_hitNs.push_back(ns);
        m_sink += float(id);
        return id;
    }

    Percentiles hitPercentiles() const { return summarizeNs(m_hitNs); }

    std::string dumpText() const
    {
        char buf[768];
        const Percentiles p = hitPercentiles();
        std::snprintf(buf, sizeof(buf),
                      "[doc-probe] fixture nodes=%d samples=%d enabled=%d every_sample=%d\n"
                      "[doc-probe] hit-test n=%d p50=%lldns p95=%lldns p99=%lldns "
                      "(p95=%lldus)\n"
                      "[doc-probe] samples processed=%lld dropped=%lld paint-loop-hits=%lld sink=%.1f\n",
                      m_doc.nodeCount(), m_doc.sampleCount(), int(m_enabled), int(m_everySample), p.n,
                      static_cast<long long>(p.p50Ns), static_cast<long long>(p.p95Ns),
                      static_cast<long long>(p.p99Ns), static_cast<long long>(nsToUs(p.p95Ns)),
                      static_cast<long long>(m_samplesSeen), static_cast<long long>(m_samplesDropped),
                      static_cast<long long>(m_paintLoopHits), double(m_sink));
        return std::string(buf);
    }

    void dump() const
    {
        if (!m_enabled)
            return;
        std::fputs(dumpText().c_str(), stderr);
        std::fflush(stderr);
    }

    float sink() const { return m_sink; }

private:
    StubDocument m_doc;
    std::vector<std::int64_t> m_hitNs;
    std::int64_t m_samplesSeen = 0;
    std::int64_t m_samplesDropped = 0;
    std::int64_t m_paintLoopHits = 0;
    float m_sink = 0.f;
    bool m_enabled = false;
    bool m_everySample = false;
    bool m_inPaint = false;
};

inline Harness &harness()
{
    static Harness h;
    return h;
}

/** Host-only simulated ink emit — not Qt paint(); used to compare probe-on vs off. */
inline void simulatePaintSegment(std::vector<Sample> &ink, float x0, float y0, float x1, float y1)
{
    ink.push_back({x0, y0});
    ink.push_back({x1, y1});
}

struct IngestSimResult {
    Percentiles pressToFirstNs;
    std::int64_t samplesEmitted = 0;
    std::int64_t samplesDropped = 0;
    float sink = 0;
};

/**
 * Synthetic ingest: N strokes × samplesPerStroke. Probe (if any) runs on press
 * plus a resident touch per sample — never from simulatePaintSegment.
 */
inline IngestSimResult simulateIngest(int strokes, int samplesPerStroke, Harness *probe)
{
    IngestSimResult out;
    std::vector<std::int64_t> pressToFirst;
    pressToFirst.reserve(std::size_t(strokes));
    std::vector<Sample> ink;
    ink.reserve(std::size_t(strokes * samplesPerStroke * 2));

    float x = 80.f;
    float y = 120.f;
    for (int s = 0; s < strokes; ++s) {
        const auto tPress = std::chrono::steady_clock::now();
        if (probe) {
            probe->onIngest(x, y, true);
        }
        float px = x;
        float py = y;
        for (int i = 0; i < samplesPerStroke; ++i) {
            x += 1.7f;
            y += (i % 3 == 0) ? 0.8f : -0.3f;
            if (probe)
                probe->onIngest(x, y, false);
            simulatePaintSegment(ink, px, py, x, y);
            px = x;
            py = y;
            ++out.samplesEmitted;
            if (i == 0) {
                const auto tFirst = std::chrono::steady_clock::now();
                pressToFirst.push_back(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(tFirst - tPress).count());
            }
        }
        y += 18.f;
        if (y > kWorldH - 40.f) {
            y = 80.f;
            x = std::fmod(x + 40.f, kWorldW - 80.f) + 40.f;
        }
    }
    out.pressToFirstNs = summarizeNs(pressToFirst);
    if (probe) {
        out.samplesDropped = probe->samplesDropped();
        out.sink = probe->sink();
    }
    // Keep the compiler from deleting the ink buffer.
    if (!ink.empty())
        out.sink += ink.front().x + ink.back().y;
    return out;
}

} // namespace latencyprobe
} // namespace epaper
