/**
 * Host analog for STORY-EP-031 / [SRS-EP-14] [SRS-EP-20] recognition + warp cost.
 * Not a device p95. Build: ./tests/run_device_document_test.sh
 */

#include "document/connector_warp.hpp"
#include "document/device_document.hpp"
#include "document/recognizer_dispatch.hpp"
#include "debug/latency_probe.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace epaper::document;
using epaper::latencyprobe::nsToUs;
using epaper::latencyprobe::Percentiles;
using epaper::latencyprobe::summarizeNs;

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

static std::vector<InkSample> pts(std::initializer_list<std::pair<double, double>> xy)
{
    std::vector<InkSample> out;
    int i = 0;
    for (const auto &p : xy) {
        InkSample s;
        s.x = p.first;
        s.y = p.second;
        s.t = static_cast<double>(i++);
        out.push_back(s);
    }
    return out;
}

static std::vector<InkSample> lineXY(double x0, double y0, double x1, double y1, int n)
{
    std::vector<InkSample> out;
    for (int i = 0; i < n; ++i) {
        InkSample s;
        const double t = n == 1 ? 0 : double(i) / double(n - 1);
        s.x = x0 + t * (x1 - x0);
        s.y = y0 + t * (y1 - y0);
        s.t = double(i);
        out.push_back(s);
    }
    return out;
}

static std::vector<InkSample> wiggleXY(double x0, double y0, double x1, double y1, int n)
{
    std::vector<InkSample> out;
    for (int i = 0; i < n; ++i) {
        InkSample s;
        const double t = n == 1 ? 0 : double(i) / double(n - 1);
        s.x = x0 + t * (x1 - x0);
        s.y = y0 + t * (y1 - y0) + 18.0 * std::sin(t * 6.283185307179586 * 2.0);
        s.t = double(i);
        out.push_back(s);
    }
    return out;
}

static void appendInk(DeviceDocument &doc, const std::string &id,
                      const std::vector<InkSample> &samples)
{
    JsonValue::Object style;
    style.emplace_back("stroke", JsonValue::string("#1C2430"));
    style.emplace_back("strokeWidth", JsonValue::number(2));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("samples", inkSamplesToJson(samples));
    payload.emplace_back("style", JsonValue::object(std::move(style)));
    DocOp op;
    op.opId = "append_ink:" + id;
    op.type = "append_ink";
    op.payload = JsonValue::object(std::move(payload));
    CHECK(doc.commitOp(op).applied);
}

static void addSg(DeviceDocument &doc, const std::string &id, double x, double y, double w, double h)
{
    const double xs[5] = {0, w, w, 0, 0};
    const double ys[5] = {0, 0, h, h, 0};
    std::vector<InkSample> bpoly;
    for (int i = 0; i < 5; ++i) {
        InkSample s;
        s.x = xs[i];
        s.y = ys[i];
        s.t = double(i);
        bpoly.push_back(s);
    }
    JsonValue::Object b;
    b.emplace_back("x", JsonValue::number(0));
    b.emplace_back("y", JsonValue::number(0));
    b.emplace_back("width", JsonValue::number(w));
    b.emplace_back("height", JsonValue::number(h));
    JsonValue::Object t;
    t.emplace_back("x", JsonValue::number(x));
    t.emplace_back("y", JsonValue::number(y));
    t.emplace_back("rotation", JsonValue::number(0));
    t.emplace_back("scaleX", JsonValue::number(1));
    t.emplace_back("scaleY", JsonValue::number(1));
    Style st;
    JsonValue kids = JsonValue::array({inkChildToJson(id + "_b", "boundary", bpoly, st, std::nullopt)});
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    payload.emplace_back("transform", JsonValue::object(std::move(t)));
    payload.emplace_back("inkScaleMode", JsonValue::string("fixedInk"));
    payload.emplace_back("children", std::move(kids));
    DocOp op;
    op.opId = "create_smart_group:" + id;
    op.type = "create_smart_group";
    op.payload = JsonValue::object(std::move(payload));
    CHECK(doc.commitOp(op).applied);
}

static void createSgEmpty(DeviceDocument &doc, const std::string &id, double tx, double ty, double w,
                          double h)
{
    JsonValue::Object b;
    b.emplace_back("x", JsonValue::number(0));
    b.emplace_back("y", JsonValue::number(0));
    b.emplace_back("width", JsonValue::number(w));
    b.emplace_back("height", JsonValue::number(h));
    JsonValue::Object t;
    t.emplace_back("x", JsonValue::number(tx));
    t.emplace_back("y", JsonValue::number(ty));
    t.emplace_back("rotation", JsonValue::number(0));
    t.emplace_back("scaleX", JsonValue::number(1));
    t.emplace_back("scaleY", JsonValue::number(1));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    payload.emplace_back("transform", JsonValue::object(std::move(t)));
    payload.emplace_back("inkScaleMode", JsonValue::string("fixedInk"));
    payload.emplace_back("children", JsonValue::array({}));
    DocOp op;
    op.opId = "create_smart_group:" + id;
    op.type = "create_smart_group";
    op.payload = JsonValue::object(std::move(payload));
    CHECK(doc.commitOp(op).applied);
}

static RecogDispatchResult penUp(DeviceDocument &doc, const std::string &id,
                                 const std::vector<InkSample> &samples)
{
    EncloseStrokeInput stroke;
    stroke.id = id;
    stroke.samples = samples;
    RecogLatch latch;
    return dispatchPenUp(doc, stroke, latch);
}

static std::int64_t nowNs()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

template <typename Fn>
static Percentiles benchNs(int n, int warmup, Fn fn)
{
    for (int i = 0; i < warmup; ++i)
        (void)fn();
    std::vector<std::int64_t> samples;
    samples.reserve(std::size_t(n));
    for (int i = 0; i < n; ++i)
        samples.push_back(fn());
    return summarizeNs(samples);
}

static void printRow(const char *name, const Percentiles &p, std::int64_t barUs, const char *barName,
                     bool showHz)
{
    const std::int64_t p50 = nsToUs(p.p50Ns);
    const std::int64_t p95 = nsToUs(p.p95Ns);
    const std::int64_t p99 = nsToUs(p.p99Ns);
    const char *verdict = (barUs < 0 || p95 <= barUs) ? "under" : "OVER";
    std::cout << name << " n=" << p.n << " p50=" << p50 << "us p95=" << p95 << "us p99=" << p99
              << "us";
    if (barUs >= 0)
        std::cout << "  bar " << barName << "=" << barUs << "us host-" << verdict;
    if (showHz && p.p95Ns > 0) {
        const double hz = 1e9 / double(p.p95Ns);
        std::cout << "  ~" << int(hz + 0.5) << " Hz @ p95 (CPU only)";
    }
    std::cout << "\n";
}

static RestVec unitOf(RestVec a)
{
    const double l = std::hypot(a.x, a.y);
    return l > 1e-12 ? RestVec{a.x / l, a.y / l} : RestVec{1, 0};
}

static void endsFromSpine(const RestShape &rs, WarpEnd *e0, WarpEnd *e1)
{
    e0->p = rs.spine.front();
    e1->p = rs.spine.back();
    e0->f = unitOf({rs.spine[1].x - rs.spine[0].x, rs.spine[1].y - rs.spine[0].y});
    const size_t n = rs.spine.size();
    e1->f = unitOf({rs.spine[n - 2].x - rs.spine[n - 1].x, rs.spine[n - 2].y - rs.spine[n - 1].y});
}

int main()
{
    constexpr int kWarm = 8;
    constexpr int kRecogN = 80;
    constexpr int kWarpN = 400;
    constexpr int kLiveN = 200;
    constexpr std::int64_t kEncloseBarUs = 500000;     // SRS-EP-14 ≤500 ms
    constexpr std::int64_t kMembershipBarUs = 300000;  // SRS-EP-14 ≤300 ms
    constexpr std::int64_t kConnectorBarUs = 500000;   // SRS-EP-20 ≤500 ms
    constexpr std::int64_t kLiveBarUs = 200000;        // SRS-EP-20 stall ≤200 ms / ≥5 Hz

    {
        DeviceDocument doc;
        appendInk(doc, "ink_in", pts({{60, 60}, {80, 70}, {70, 90}}));
        CHECK(penUp(doc, "enclose_1", pts({{40, 40}, {160, 40}, {160, 160}, {40, 160}, {40, 40}}))
                  .outcome
              == RecogOutcome::Enclose);
    }
    {
        DeviceDocument doc;
        createSgEmpty(doc, "sg_1", 40, 40, 120, 120);
        CHECK(penUp(doc, "box_stroke", pts({{50, 50}, {150, 50}, {150, 150}, {50, 150}, {50, 50}}))
                  .outcome
              == RecogOutcome::Membership);
    }
    {
        DeviceDocument doc;
        addSg(doc, "A", 0, 0, 80, 80);
        addSg(doc, "C", 300, 0, 80, 80);
        CHECK(penUp(doc, "ink_ac", lineXY(78, 40, 302, 40, 24)).outcome == RecogOutcome::Connector);
    }
    {
        DeviceDocument doc;
        addSg(doc, "A", 0, 0, 80, 80);
        addSg(doc, "C", 300, 0, 80, 80);
        appendInk(doc, "s1", lineXY(78, 40, 140, 40, 8));
        appendInk(doc, "s2", lineXY(140, 40, 220, 40, 8));
        CHECK(penUp(doc, "s3", lineXY(220, 40, 302, 40, 8)).outcome == RecogOutcome::Connector);
    }
    {
        DeviceDocument doc;
        CHECK(penUp(doc, "raw", lineXY(10, 400, 90, 410, 16)).outcome == RecogOutcome::Ink);
    }

    const Percentiles enclose = benchNs(kRecogN, kWarm, []() -> std::int64_t {
        DeviceDocument doc;
        appendInk(doc, "ink_in", pts({{60, 60}, {80, 70}, {70, 90}}));
        return penUp(doc, "enclose_1", pts({{40, 40}, {160, 40}, {160, 160}, {40, 160}, {40, 40}})).ns;
    });
    CHECK(nsToUs(enclose.p95Ns) <= kEncloseBarUs);

    const Percentiles membership = benchNs(kRecogN, kWarm, []() -> std::int64_t {
        DeviceDocument doc;
        createSgEmpty(doc, "sg_1", 40, 40, 120, 120);
        return penUp(doc, "box_stroke", pts({{50, 50}, {150, 50}, {150, 150}, {50, 150}, {50, 50}}))
            .ns;
    });
    CHECK(nsToUs(membership.p95Ns) <= kMembershipBarUs);

    const Percentiles ux1 = benchNs(kRecogN, kWarm, []() -> std::int64_t {
        DeviceDocument doc;
        addSg(doc, "A", 0, 0, 80, 80);
        addSg(doc, "C", 300, 0, 80, 80);
        return penUp(doc, "ink_ac", lineXY(78, 40, 302, 40, 24)).ns;
    });
    CHECK(nsToUs(ux1.p95Ns) <= kConnectorBarUs);

    const Percentiles ux2 = benchNs(kRecogN, kWarm, []() -> std::int64_t {
        DeviceDocument doc;
        addSg(doc, "A", 0, 0, 80, 80);
        addSg(doc, "C", 300, 0, 80, 80);
        appendInk(doc, "s1", lineXY(78, 40, 140, 40, 8));
        appendInk(doc, "s2", lineXY(140, 40, 220, 40, 8));
        return penUp(doc, "s3", lineXY(220, 40, 302, 40, 8)).ns;
    });
    CHECK(nsToUs(ux2.p95Ns) <= kConnectorBarUs);

    const Percentiles rawInk = benchNs(kRecogN, kWarm, []() -> std::int64_t {
        DeviceDocument doc;
        return penUp(doc, "raw", lineXY(10, 400, 90, 410, 16)).ns;
    });
    CHECK(nsToUs(rawInk.p95Ns) <= kConnectorBarUs);

    const RestShape rs = buildRestShape({wiggleXY(0, 0, 400, 0, 80)});
    CHECK(rs.spine.size() >= 2);
    WarpEnd e0, e1;
    endsFromSpine(rs, &e0, &e1);

    const Percentiles morph = benchNs(kWarpN, kWarm, [&]() -> std::int64_t {
        const auto t0 = nowNs();
        const WarpResult w = warpConnector(rs, e0, e1, "morph");
        const auto t1 = nowNs();
        CHECK(!w.samples.empty());
        return t1 - t0;
    });

    const Percentiles cubic = benchNs(kWarpN, kWarm, [&]() -> std::int64_t {
        WarpEnd a = e0;
        WarpEnd b = e1;
        a.p.x += 40;
        a.p.y += 12;
        b.p.x -= 8;
        b.p.y += 20;
        const auto t0 = nowNs();
        const WarpResult w = warpConnector(rs, a, b, "cubic");
        const auto t1 = nowNs();
        CHECK(!w.samples.empty());
        return t1 - t0;
    });

    DeviceDocument liveMorph;
    addSg(liveMorph, "A", 0, 0, 80, 80);
    addSg(liveMorph, "C", 300, 0, 80, 80);
    CHECK(penUp(liveMorph, "ink_ac", lineXY(78, 40, 302, 40, 24)).outcome == RecogOutcome::Connector);
    DocNode *aMorph = nullptr;
    for (auto &n : liveMorph.rootChildren) {
        if (n.id == "A")
            aMorph = &n;
    }
    CHECK(aMorph);
    const SmartTransform originMorph = aMorph->transform;
    const SmartBounds boundsMorph = aMorph->smartBounds;
    int liveI = 0;
    const Percentiles liveMorphP = benchNs(kLiveN, kWarm, [&]() -> std::int64_t {
        SmartTransform t = originMorph;
        t.x += 2.0 * (++liveI);
        const auto t0 = nowNs();
        liveMorph.applyLiveSmartGeometry("A", t, boundsMorph);
        refreshConnectorsBoundTo(liveMorph, "A");
        return nowNs() - t0;
    });
    CHECK(nsToUs(liveMorphP.p95Ns) <= kLiveBarUs);

    DeviceDocument liveCubic;
    addSg(liveCubic, "A", 0, 0, 80, 80);
    addSg(liveCubic, "C", 300, 0, 80, 80);
    const RecogDispatchResult d2 = penUp(liveCubic, "ink_ac", lineXY(78, 40, 302, 40, 24));
    CHECK(d2.outcome == RecogOutcome::Connector);
    DocNode *conn = nullptr;
    DocNode *aCubic = nullptr;
    for (auto &n : liveCubic.rootChildren) {
        if (n.id == d2.connector.connectorId)
            conn = &n;
        if (n.id == "A")
            aCubic = &n;
    }
    CHECK(conn);
    conn->warpStyle = "cubic";
    refreshConnectorWarp(liveCubic, *conn);
    CHECK(aCubic);
    const SmartTransform originCubic = aCubic->transform;
    const SmartBounds boundsCubic = aCubic->smartBounds;
    int liveJ = 0;
    const Percentiles liveCubicP = benchNs(kLiveN, kWarm, [&]() -> std::int64_t {
        SmartTransform t = originCubic;
        t.x += 2.0 * (++liveJ);
        const auto t0 = nowNs();
        liveCubic.applyLiveSmartGeometry("A", t, boundsCubic);
        refreshConnectorsBoundTo(liveCubic, "A");
        return nowNs() - t0;
    });
    CHECK(nsToUs(liveCubicP.p95Ns) <= kLiveBarUs);

    std::cout << "=== STORY-EP-031 host recog/warp bench ===\n";
    std::cout << "caveat: NOT device p95. Host (this machine) only.\n";
    std::cout << "SRS-EP-14: enclose p95 ≤500ms; membership p95 ≤300ms.\n";
    std::cout << "SRS-EP-20: connector visible p95 ≤500ms; live re-warp ≥5 Hz / stall ≤200ms; "
                 "0 full-panel invalidations (paint, not this bench).\n";
    std::cout << "Live paint: CanvasLayer origin hole is a stroke punch (not AABB fill). "
                 "Live connector stays on ToolCanvas. Mid-gesture e-ink overlay dirt is BR-B15 "
                 "allowance; settled frame must be clean.\n\n";
    printRow("recog enclose→ink-box", enclose, kEncloseBarUs, "SRS-EP-14", false);
    printRow("recog draw-into membership", membership, kMembershipBarUs, "SRS-EP-14", false);
    printRow("recog connector UX1 single", ux1, kConnectorBarUs, "SRS-EP-20", false);
    printRow("recog connector UX2 multi", ux2, kConnectorBarUs, "SRS-EP-20", false);
    printRow("recog raw ink (miss)", rawInk, kConnectorBarUs, "SRS-EP-20", false);
    printRow("warp Morph (pure, rest pose)", morph, kLiveBarUs, "SRS-EP-20 stall", true);
    printRow("warp Cubic (pure, displaced)", cubic, kLiveBarUs, "SRS-EP-20 stall", true);
    printRow("live re-warp Morph (apply+refresh)", liveMorphP, kLiveBarUs, "SRS-EP-20 stall", true);
    printRow("live re-warp Cubic (apply+refresh)", liveCubicP, kLiveBarUs, "SRS-EP-20 stall", true);

    if (g_fails) {
        std::cerr << "recog_warp_bench: " << g_fails << " failed\n";
        return 1;
    }
    std::cout << "recog_warp_bench: OK\n";
    return 0;
}
