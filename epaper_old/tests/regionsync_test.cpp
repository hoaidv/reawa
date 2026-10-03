/**
 * Host tests for Epaper region sync (no Qt required).
 * @implements [SRS-EP-02]
 * @implements [SRS-EP-03]
 * @implements [ADR-0012]
 *
 * Build: ./tests/run_regionsync_test.sh
 */

#include "regionsync/region_session.hpp"

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

using namespace epaper::regionsync;

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

class RecordingNet : public NetSink {
public:
    bool failNext = false;
    std::vector<DocOp> sent;
    bool sendDocOp(const DocOp &op) override
    {
        if (failNext) {
            failNext = false;
            return false;
        }
        sent.push_back(op);
        return true;
    }
};

/** Illegal sink that "sends" during pen callback if invoked. */
class HotPathSpyNet : public NetSink {
public:
    bool sendDocOp(const DocOp &) override
    {
        markPenCallback();
        return true;
    }
};

static ViewportMessage makeVp(int seq, double tx, double ty, double scale, Aabb region = {0, 0, 40, 20})
{
    ViewportMessage m;
    m.seq = seq;
    m.translate = {tx, ty};
    m.scale = scale;
    m.drawingRegion = region;
    return m;
}

static void test_viewport_before_pen()
{
    RecordingNet net;
    RegionSession session(&net);
    session.connect();
    session.setPanelSize(200, 100);
    CHECK(session.onViewport(makeVp(1, 0, 0, 1.0)));
    CHECK(session.map().seq == 1);

    CHECK(session.onViewport(makeVp(2, -120, 40, 1.5)));
    CHECK(session.map().seq == 2);
    CHECK(session.refreshQueueSize() == 1);

    PenSample s;
    s.localX = 100;
    s.localY = 50;
    s.pressure = 0.42;
    s.tiltX = 0.1;
    session.onPenSample(s);
    session.endStroke("ink_a", "op_a");
    CHECK(!session.doc().inks().empty());
    const auto &samples = session.doc().inks().at("ink_a").samples;
    CHECK(samples.size() == 1);
    // panel 200x100, region 0..40 x 0..20 → (100,50) → (20,10)
    CHECK(std::abs(samples[0].x - 20.0) < 1e-9);
    CHECK(std::abs(samples[0].y - 10.0) < 1e-9);
}

static void test_panel_to_region_not_screen_formula()
{
    RecordingNet net;
    RegionSession session(&net);
    session.connect();
    session.setPanelSize(200, 100);
    // Infini-like translate/scale that would give a different answer under old formula
    CHECK(session.onViewport(makeVp(1, -10, 5, 2.0, {0, 0, 40, 20})));
    PenSample s{100, 50, {}, {}, {}, {}};
    session.onPenSample(s);
    session.endStroke("ink_map", "op_map");
    const auto &pt = session.doc().inks().at("ink_map").samples[0];
    CHECK(std::abs(pt.x - 20.0) < 1e-9);
    CHECK(std::abs(pt.y - 10.0) < 1e-9);
    // Old formula: local/scale - translate = 100/2 - (-10) = 60 — must NOT match
    CHECK(std::abs(pt.x - 60.0) > 1.0);
}

static void test_append_ink_channels_no_smart_group()
{
    RecordingNet net;
    RegionSession session(&net);
    session.connect();
    session.setPanelSize(200, 100);
    CHECK(session.onViewport(makeVp(1, 0, 0, 1.0)));

    PenSample s;
    s.localX = 10;
    s.localY = 20;
    s.pressure = 0.42;
    s.tiltX = 0.1;
    s.extras["distance"] = 0;
    session.onPenSample(s);
    session.endStroke("ink_1", "op_ink_1");

    CHECK(session.smartGroupRanOnDevice() == false);
    session.flushNetQueue();
    CHECK(net.sent.size() == 1);
    CHECK(net.sent[0].type == "append_ink");
    CHECK(net.sent[0].source == "epaper");
    CHECK(net.sent[0].samples[0].pressure.has_value());
    CHECK(*net.sent[0].samples[0].pressure == 0.42);
    CHECK(net.sent[0].samples[0].extras.at("distance") == 0);
}

static void test_remote_op_coherent_refresh()
{
    RecordingNet net;
    RegionSession session(&net);
    session.connect();
    session.setPanelSize(200, 100);
    CHECK(session.onViewport(makeVp(5, 0, 0, 1.0)));
    (void)session.runRegionRefresh(0); // clear initial

    DocOp remote;
    remote.opId = "remote_1";
    remote.type = "append_ink";
    remote.source = "infini";
    remote.inkId = "remote_ink";
    remote.samples.push_back(InkSample{1, 2, {}, {}, {}, {}});
    auto r = session.onRemoteDocOp(remote);
    CHECK(r.applied);
    CHECK(session.doc().version() >= 1);

    auto pass = session.runRegionRefresh(300);
    CHECK(pass.has_value());
    CHECK(pass->coherent);
    CHECK(pass->mapSeq == 5);
    CHECK(pass->docVersion == session.doc().version());
    CHECK(pass->contentHash.find("remote_ink") != std::string::npos);

    auto r2 = session.onRemoteDocOp(remote);
    CHECK(!r2.applied);
    CHECK(r2.reason == "duplicate_opId");
}

static void test_refresh_coalesce_and_settle()
{
    RecordingNet net;
    RegionSession session(&net);
    session.connect();
    session.setPanelSize(200, 100);

    double t = 0;
    for (int i = 1; i <= 10; ++i) {
        CHECK(session.onViewport(makeVp(i, -i, i, 1.0 + i * 0.01)));
        t += 10; // 100 ms total spam
        auto early = session.runRegionRefresh(t);
        if (i == 1) {
            CHECK(early.has_value()); // first paint allowed
        } else {
            CHECK(!early.has_value()); // within 250 ms floor
        }
    }
    CHECK(session.map().seq == 10);
    CHECK(session.paintCount() == 1);

    auto later = session.runRegionRefresh(t + 250);
    CHECK(later.has_value());
    CHECK(later->mapSeq == 10);
    CHECK(session.paintCount() == 2);

    // More pending + settle flush before floor
    CHECK(session.onViewport(makeVp(11, 0, 0, 1.0)));
    auto settle = session.runRegionRefresh(t + 250 + 50, true);
    CHECK(settle.has_value());
    CHECK(settle->mapSeq == 11);
}

static void test_stroke_panel_width()
{
    ViewportMap map;
    map.drawingRegion = {0, 0, 40, 20};
    map.panelW = 200;
    map.panelH = 100;
    CHECK(std::abs(strokePanelWidth(2.0, map) - 10.0) < 1e-9);
    map.drawingRegion = {0, 0, 80, 40};
    CHECK(std::abs(strokePanelWidth(2.0, map) - 5.0) < 1e-9);
}

static void test_session_owns_map_over_strokesync()
{
    RecordingNet net;
    RegionSession session(&net);
    session.connect();
    session.setPanelSize(200, 100);
    CHECK(session.onViewport(makeVp(1, 0, 0, 1.0)));
    CHECK(session.ownsDrawingRegionMap() == true);
    // Legacy StrokeSync must not own map when session connected+valid
    CHECK(session.map().seq == 1);
}

static void test_send_fail_does_not_block_hot_path()
{
    RecordingNet net;
    net.failNext = true;
    RegionSession session(&net);
    session.connect();
    session.setPanelSize(200, 100);
    CHECK(session.onViewport(makeVp(1, 0, 0, 1.0)));

    PenSample s{3, 4, 0.5, {}, {}, {}};
    session.onPenSample(s);
    session.endStroke("ink_b", "op_b");
    session.flushNetQueue();
    CHECK(session.netQueueSize() == 1);
    session.onPenSample(PenSample{5, 6, {}, {}, {}, {}});
    CHECK(session.logs().size() >= 1);

    net.failNext = false;
    session.flushNetQueue();
    CHECK(session.netQueueSize() == 0);
}

static void test_hot_path_no_socket()
{
    HotPathSpyNet spy;
    RegionSession session(&spy);
    session.connect();
    session.setPanelSize(200, 100);
    CHECK(session.onViewport(makeVp(1, 0, 0, 1.0)));
    session.onPenSample(PenSample{1, 1, {}, {}, {}, {}});
    CHECK(spy.didRunOnPenCallback() == false);
    for (const auto &l : session.logs())
        CHECK(l.find("violation") == std::string::npos);
}

int main()
{
    test_viewport_before_pen();
    test_panel_to_region_not_screen_formula();
    test_append_ink_channels_no_smart_group();
    test_remote_op_coherent_refresh();
    test_refresh_coalesce_and_settle();
    test_stroke_panel_width();
    test_session_owns_map_over_strokesync();
    test_send_fail_does_not_block_hot_path();
    test_hot_path_no_socket();

    if (g_fails) {
        std::cerr << g_fails << " check(s) failed\n";
        return 1;
    }
    std::cout << "regionsync_test: all checks passed\n";
    return 0;
}
