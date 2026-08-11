/**
 * Host tests for Epaper region sync (no Qt required).
 * @implements [SRS-EP-02]
 * @implements [SRS-EP-03]
 *
 * Build: c++ -std=c++17 -I. tests/regionsync_test.cpp -o /tmp/regionsync_test && /tmp/regionsync_test
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

static ViewportMessage makeVp(int seq, double tx, double ty, double scale)
{
    ViewportMessage m;
    m.seq = seq;
    m.translate = {tx, ty};
    m.scale = scale;
    m.drawingRegion = {-100, -100, 100, 100};
    return m;
}

static void test_viewport_before_pen()
{
    RecordingNet net;
    RegionSession session(&net);
    session.connect();
    // M0
    CHECK(session.onViewport(makeVp(1, 0, 0, 1.0)));
    CHECK(session.map().seq == 1);

    // New viewport M1
    CHECK(session.onViewport(makeVp(2, -120, 40, 1.5)));
    CHECK(session.map().seq == 2);
    CHECK(session.refreshQueueSize() == 1);

    // Next pen sample must use M1
    PenSample s;
    s.localX = 150;
    s.localY = 75;
    s.pressure = 0.42;
    s.tiltX = 0.1;
    session.onPenSample(s);
    session.endStroke("ink_a", "op_a");
    CHECK(!session.doc().inks().empty());
    const auto &samples = session.doc().inks().at("ink_a").samples;
    CHECK(samples.size() == 1);
    const Vec2 expect = panelToWorld(session.map(), 150, 75);
    CHECK(std::abs(samples[0].x - expect.x) < 1e-9);
    CHECK(std::abs(samples[0].y - expect.y) < 1e-9);
}

static void test_append_ink_channels_no_smart_group()
{
    RecordingNet net;
    RegionSession session(&net);
    session.connect();
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
    CHECK(session.onViewport(makeVp(5, 0, 0, 1.0)));
    (void)session.runRegionRefresh(); // clear initial refresh from viewport

    DocOp remote;
    remote.opId = "remote_1";
    remote.type = "append_ink";
    remote.source = "infini";
    remote.inkId = "remote_ink";
    remote.samples.push_back(InkSample{1, 2, {}, {}, {}, {}});
    auto r = session.onRemoteDocOp(remote);
    CHECK(r.applied);
    CHECK(session.doc().version() >= 1);

    auto pass = session.runRegionRefresh();
    CHECK(pass.has_value());
    CHECK(pass->coherent);
    CHECK(pass->mapSeq == 5);
    CHECK(pass->docVersion == session.doc().version());
    CHECK(pass->contentHash.find("remote_ink") != std::string::npos);

    // Idempotent
    auto r2 = session.onRemoteDocOp(remote);
    CHECK(!r2.applied);
    CHECK(r2.reason == "duplicate_opId");
}

static void test_send_fail_does_not_block_hot_path()
{
    RecordingNet net;
    net.failNext = true;
    RegionSession session(&net);
    session.connect();
    CHECK(session.onViewport(makeVp(1, 0, 0, 1.0)));

    PenSample s{3, 4, 0.5, {}, {}, {}};
    session.onPenSample(s);
    session.endStroke("ink_b", "op_b");
    // Hot path already finished; flush fails and retries
    session.flushNetQueue();
    CHECK(session.netQueueSize() == 1);
    // More pen samples still work
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
    CHECK(session.onViewport(makeVp(1, 0, 0, 1.0)));
    session.onPenSample(PenSample{1, 1, {}, {}, {}, {}});
    // onPenSample must not call sendDocOp
    CHECK(spy.didRunOnPenCallback() == false);
    for (const auto &l : session.logs())
        CHECK(l.find("violation") == std::string::npos);
}

int main()
{
    test_viewport_before_pen();
    test_append_ink_channels_no_smart_group();
    test_remote_op_coherent_refresh();
    test_send_fail_does_not_block_hot_path();
    test_hot_path_no_socket();

    if (g_fails) {
        std::cerr << g_fails << " check(s) failed\n";
        return 1;
    }
    std::cout << "regionsync_test: all checks passed\n";
    return 0;
}
