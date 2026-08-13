/**
 * Host tests for STORY-EP-014 / [SRS-EP-07] [SRS-EP-09] — device tree + stroke ingest.
 * Maps ingest-stroke.feature. No Qt. Shared ops/ fixtures vs Infini apply semantics.
 *
 * Build: ./tests/run_device_document_test.sh
 */

#include "document/device_document.hpp"
#include "document/ingest_stroke.hpp"
#include "latencyprobe/stub_document.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace epaper::document;
using epaper::latencyprobe::nsToUs;
using epaper::latencyprobe::summarizeNs;

static bool near(double a, double b, double eps = 1e-9)
{
    return std::abs(a - b) <= eps;
}

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

static DigitizerSample samp(double x, double y, double pressure)
{
    DigitizerSample s;
    s.panelX = x;
    s.panelY = y;
    s.pressure = pressure;
    return s;
}

static std::string fixturePath(const char *name)
{
    return std::string("tests/fixtures/ops/") + name;
}

static void test_append_ink_fixture_needs_parent()
{
    DeviceDocument doc;
    const DocOp op = opFromJson(loadJsonFile(fixturePath("append_ink.json")));
    CHECK(op.type == "append_ink");
    CHECK(op.source == "epaper");
    const ApplyResult r = doc.applyOp(op);
    CHECK(!r.applied);
    CHECK(r.reason == "bad_parent:frm_1");
    CHECK(doc.inkCount() == 0);
    CHECK(doc.nodeCount() == 0);
}

static void test_create_primitive_fixture()
{
    DeviceDocument doc;
    const DocOp op = opFromJson(loadJsonFile(fixturePath("create_primitive.json")));
    CHECK(op.type == "create_primitive");
    CHECK(op.source == "infini");
    CHECK(doc.applyOp(op).applied);
    CHECK(doc.nodeCount() == 1);
    const DocNode *n = doc.find("rect_1");
    CHECK(n);
    CHECK(n->kind == NodeKind::Primitive);
    CHECK(n->geomKind == PrimitiveKind::Rect);
    CHECK(n->gx == 0 && n->gy == 0 && n->gw == 100 && n->gh == 40);
    CHECK(n->style.stroke == "#1C2430");
    CHECK(n->style.strokeWidth == 2);
}

static void test_shared_ops_sequence_matches_infini_shape()
{
    DeviceDocument doc;
    CHECK(doc.applyOp(opFromJson(parseJson(R"({
      "opId": "f1",
      "type": "create_frame",
      "payload": { "id": "frm_1", "bounds": { "minX": 0, "minY": 0, "maxX": 400, "maxY": 300 } }
    })")))
              .applied);
    CHECK(doc.applyOp(opFromJson(loadJsonFile(fixturePath("append_ink.json")))).applied);
    CHECK(doc.applyOp(opFromJson(loadJsonFile(fixturePath("create_primitive.json")))).applied);

    CHECK(doc.inkCount() == 1);
    CHECK(doc.nodeCount() == 3);
    const DocNode *frame = doc.find("frm_1");
    CHECK(frame && frame->kind == NodeKind::Frame);
    CHECK(frame->children.size() == 1);
    CHECK(frame->children[0].id == "ink_1");

    const DocNode *ink = doc.find("ink_1");
    CHECK(ink && ink->kind == NodeKind::Ink);
    CHECK(ink->samples.size() == 2);
    CHECK(ink->samples[0].x == 10 && ink->samples[0].y == 20);
    CHECK(ink->samples[0].pressure && near(*ink->samples[0].pressure, 0.42));
    CHECK(ink->samples[0].tiltX && near(*ink->samples[0].tiltX, 0.1));
    CHECK(ink->samples[0].tiltY && near(*ink->samples[0].tiltY, -0.05));
    CHECK(ink->samples[0].t && *ink->samples[0].t == 0);
    CHECK(ink->samples[1].x == 12 && ink->samples[1].y == 24);
    CHECK(ink->samples[1].pressure && near(*ink->samples[1].pressure, 0.55));
    CHECK(ink->samples[1].tiltX && near(*ink->samples[1].tiltX, 0.12));
    CHECK(ink->samples[1].tiltY && near(*ink->samples[1].tiltY, -0.04));
    CHECK(ink->samples[1].t && *ink->samples[1].t == 16);
    CHECK(ink->samples[1].extras.count("distance") == 1);
    CHECK(ink->samples[1].extras.at("distance").isNumber());
    CHECK(ink->samples[1].extras.at("distance").asNumber() == 0);
    CHECK(ink->style.strokeWidth == 2);

    const DocNode *rect = doc.find("rect_1");
    CHECK(rect && rect->kind == NodeKind::Primitive);
    CHECK(doc.rootChildren.size() == 2);
    CHECK(doc.rootChildren[0].id == "frm_1");
    CHECK(doc.rootChildren[1].id == "rect_1");

    const ApplyResult dup = doc.applyOp(opFromJson(loadJsonFile(fixturePath("append_ink.json"))));
    CHECK(!dup.applied);
    CHECK(dup.reason == "duplicate_opId");
    CHECK(doc.inkCount() == 1);
}

static void test_ingest_20_strokes_no_session()
{
    DeviceDocument doc;
    const auto map = identityWorldMap();
    for (int i = 0; i < 20; ++i) {
        FinishedStroke s;
        s.id = std::string("s-") + std::to_string(i + 1);
        s.samples.push_back(samp(10.0 + i, 20.0, 0.4));
        s.samples.push_back(samp(12.0 + i, 24.0, 0.5));
        s.strokeWidthWorld = 2.5;
        CHECK(ingestFinishedStroke(doc, s, map).applied);
    }
    CHECK(doc.inkCount() == 20);
    CHECK(doc.find("s-1"));
    CHECK(doc.find("s-20"));
}

static void test_ingest_retains_digitizer_channels()
{
    DeviceDocument doc;
    FinishedStroke s;
    s.id = "ch";
    DigitizerSample a;
    a.panelX = 1;
    a.panelY = 2;
    a.pressure = 0.3;
    a.tiltX = 8;
    a.tiltY = -3;
    a.distance = 0.01;
    a.timestamp = 12345;
    a.extras.emplace("button", JsonValue::boolean(true));
    DigitizerSample b = a;
    b.panelX = 4;
    b.panelY = 6;
    b.pressure = 0.9;
    s.samples = {a, b};
    CHECK(ingestFinishedStroke(doc, s, identityWorldMap()).applied);
    const DocNode *ink = doc.find("ch");
    CHECK(ink && ink->samples.size() == 2);
    CHECK(ink->samples[0].pressure && *ink->samples[0].pressure == 0.3);
    CHECK(ink->samples[0].tiltX && *ink->samples[0].tiltX == 8);
    CHECK(ink->samples[0].tiltY && *ink->samples[0].tiltY == -3);
    CHECK(ink->samples[0].distance && *ink->samples[0].distance == 0.01);
    CHECK(ink->samples[0].timestamp && *ink->samples[0].timestamp == 12345);
    CHECK(ink->samples[0].extras.count("button") == 1);
    CHECK(ink->samples[0].extras.at("button").isBool());
    CHECK(ink->samples[0].extras.at("button").asBool());
    CHECK(ink->samples[1].x == 4 && ink->samples[1].y == 6);
}

static void test_ingest_failure_leaves_tree()
{
    DeviceDocument doc;
    FinishedStroke s;
    s.id = "short";
    s.samples.push_back(samp(0, 0, 0.5));
    CHECK(!ingestFinishedStroke(doc, s, identityWorldMap()).applied);
    CHECK(doc.inkCount() == 0);
    CHECK(!doc.find("short"));
}

static void test_unknown_op_rejected()
{
    DeviceDocument doc;
    DocOp op;
    op.opId = "x";
    op.type = "insert_node";
    op.payload = JsonValue::object({});
    const ApplyResult r = doc.applyOp(op);
    CHECK(!r.applied);
    CHECK(r.reason.find("unknown_type:") == 0);
    CHECK(doc.nodeCount() == 0);
}

static void test_ingest_after_pixels_timing()
{
    DeviceDocument doc;
    std::vector<std::int64_t> ns;
    ns.reserve(40);
    for (int i = 0; i < 40; ++i) {
        FinishedStroke s;
        s.id = std::string("t-") + std::to_string(i);
        for (int k = 0; k < 20; ++k)
            s.samples.push_back(samp(double(k), double(i), 0.5));
        const IngestTiming t = ingestFinishedStrokeTimed(doc, s, identityWorldMap());
        CHECK(t.result.applied);
        ns.push_back(t.ns);
    }
    CHECK(doc.inkCount() == 40);
    const auto p = summarizeNs(ns);
    std::cout << "ingest pen-up→node n=" << p.n << " p50=" << p.p50Ns << "ns p95=" << p.p95Ns
              << "ns p99=" << p.p99Ns << "ns (p95=" << nsToUs(p.p95Ns) << "us)\n";
    CHECK(p.p95Ns >= 0);
    CHECK(p.p95Ns <= 50000000); // 50 ms host analog — miss ⇒ CHL, do not relax
}

static void test_ingest_not_on_press_path()
{
    using namespace epaper::latencyprobe;
    DeviceDocument doc;
    Harness probe;
    probe.enable();
    const IngestSimResult withProbe = simulateIngest(40, 20, &probe);
    // Ingest at pen-up only — after samples have been "painted".
    for (int i = 0; i < 40; ++i) {
        FinishedStroke s;
        s.id = std::string("p-") + std::to_string(i);
        s.samples.push_back(samp(1, 1, 0.5));
        s.samples.push_back(samp(2, 2, 0.5));
        CHECK(ingestFinishedStroke(doc, s, identityWorldMap()).applied);
    }
    CHECK(withProbe.samplesDropped == 0);
    CHECK(withProbe.samplesEmitted == 40 * 20);
    CHECK(probe.paintLoopHits() == 0);
    CHECK(doc.inkCount() == 40);
}

int main()
{
    test_append_ink_fixture_needs_parent();
    test_create_primitive_fixture();
    test_shared_ops_sequence_matches_infini_shape();
    test_ingest_20_strokes_no_session();
    test_ingest_retains_digitizer_channels();
    test_ingest_failure_leaves_tree();
    test_unknown_op_rejected();
    test_ingest_after_pixels_timing();
    test_ingest_not_on_press_path();

    if (g_fails) {
        std::cerr << g_fails << " check(s) failed\n";
        return 1;
    }
    std::cout << "device_document_test: checks passed\n";
    return 0;
}
