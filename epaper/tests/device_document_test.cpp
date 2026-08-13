/**
 * Host tests for STORY-EP-014 / STORY-EP-015 / [SRS-EP-07] [SRS-EP-09].
 * Maps ingest-stroke.feature and undo-ring.feature. No Qt.
 *
 * Build: ./tests/run_device_document_test.sh
 */

#include "document/device_document.hpp"
#include "document/ingest_stroke.hpp"
#include "latencyprobe/stub_document.hpp"

#include <algorithm>
#include <chrono>
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

static DocOp makeAppendInkOp(const std::string &id, double x = 10, double y = 20)
{
    const std::string json = std::string("{\"opId\":\"op-") + id + "\",\"type\":\"append_ink\","
                                                                  "\"source\":\"epaper\",\"payload\":{\"id\":\""
                             + id + "\",\"samples\":[{\"x\":" + std::to_string(x) + ",\"y\":"
                             + std::to_string(y) + "},{\"x\":" + std::to_string(x + 2) + ",\"y\":"
                             + std::to_string(y + 2)
                             + "}],\"style\":{\"stroke\":\"#1C2430\",\"strokeWidth\":2}}}";
    return opFromJson(parseJson(json));
}

static bool geomNear(double a, double b, double eps = 1.0)
{
    return std::abs(a - b) <= eps;
}

static bool nodesEqualGeom(const DocNode &a, const DocNode &b)
{
    if (a.id != b.id || a.kind != b.kind || a.children.size() != b.children.size())
        return false;
    if (a.samples.size() != b.samples.size())
        return false;
    for (size_t i = 0; i < a.samples.size(); ++i) {
        if (!geomNear(a.samples[i].x, b.samples[i].x) || !geomNear(a.samples[i].y, b.samples[i].y))
            return false;
    }
    if (!geomNear(a.transform.x, b.transform.x) || !geomNear(a.transform.y, b.transform.y)
        || !geomNear(a.transform.rotation, b.transform.rotation)
        || !geomNear(a.transform.scaleX, b.transform.scaleX)
        || !geomNear(a.transform.scaleY, b.transform.scaleY))
        return false;
    if (!geomNear(a.smartBounds.x, b.smartBounds.x) || !geomNear(a.smartBounds.y, b.smartBounds.y)
        || !geomNear(a.smartBounds.width, b.smartBounds.width)
        || !geomNear(a.smartBounds.height, b.smartBounds.height))
        return false;
    for (size_t i = 0; i < a.children.size(); ++i) {
        if (!nodesEqualGeom(a.children[i], b.children[i]))
            return false;
    }
    return true;
}

static bool treesEqualGeom(const DeviceDocument &a, const std::vector<DocNode> &snap)
{
    if (a.rootChildren.size() != snap.size())
        return false;
    for (size_t i = 0; i < snap.size(); ++i) {
        if (!nodesEqualGeom(a.rootChildren[i], snap[i]))
            return false;
    }
    return true;
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

/** @SRS-EP-07 Structural op pushes a snapshot before apply */
static void test_undo_structural_op_pushes_pre_op_snapshot()
{
    DeviceDocument doc;
    CHECK(doc.undoDepth() == 0);
    const std::string pre = doc.snapshotString();
    CHECK(doc.commitOp(makeAppendInkOp("ink_a")).applied);
    CHECK(doc.undoDepth() == 1);
    CHECK(doc.newestEntry());
    CHECK(doc.newestEntry()->kind == "append_ink");
    CHECK(doc.newestEntry()->opId == "op-ink_a");
    CHECK(doc.entrySnapshotString(*doc.newestEntry()) == pre);
    CHECK(doc.publishQueue().size() == 1);
    CHECK(doc.publishQueue()[0].op.type == "append_ink");

    // Raw applyOp (fixtures) does not push.
    DeviceDocument raw;
    CHECK(raw.applyOp(makeAppendInkOp("raw")).applied);
    CHECK(raw.undoDepth() == 0);
    CHECK(raw.publishQueue().empty());

    // Rejected structural op: ring and queue unchanged.
    DeviceDocument rej;
    DocOp bad;
    bad.opId = "bad";
    bad.type = "append_ink";
    bad.payload = JsonValue::object({{"id", JsonValue::string("x")}});
    CHECK(!rej.commitOp(bad).applied);
    CHECK(rej.undoDepth() == 0);
    CHECK(rej.publishQueue().empty());
    CHECK(rej.nodeCount() == 0);
}

/** @SRS-EP-07 Undo restores the prior tree exactly */
static void test_undo_restores_prior_tree_exactly()
{
    DeviceDocument doc;
    const std::string pre = doc.snapshotString();
    CHECK(doc.commitOp(makeAppendInkOp("ink_b", 3, 4)).applied);
    CHECK(doc.find("ink_b"));
    std::vector<DocNode> preSnap;
    CHECK(doc.newestEntry());
    preSnap = doc.newestEntry()->snapshot;
    const UndoResult u = doc.undo();
    CHECK(u.restored);
    CHECK(!u.latched);
    CHECK(!u.noop);
    CHECK(doc.snapshotString() == pre);
    CHECK(treesEqualGeom(doc, preSnap));
    CHECK(!doc.find("ink_b"));
    CHECK(doc.undoDepth() == 0);

    const std::size_t q = doc.publishQueue().size();
    const UndoResult empty = doc.undo();
    CHECK(empty.noop);
    CHECK(!empty.restored);
    CHECK(doc.snapshotString() == pre);
    CHECK(doc.publishQueue().size() == q);
}

/** @SRS-EP-07 One completed gesture is one undo entry */
static void test_undo_one_gesture_one_entry()
{
    DeviceDocument doc;
    CHECK(doc.commitOp(opFromJson(parseJson(R"({
      "opId": "sg-1",
      "type": "create_smart_group",
      "source": "epaper",
      "payload": {
        "id": "sg_1",
        "bounds": { "x": 0, "y": 0, "width": 40, "height": 40 },
        "transform": { "x": 0, "y": 0, "rotation": 0, "scaleX": 1, "scaleY": 1 },
        "inkScaleMode": "withBounds",
        "children": [{
          "id": "ink_sg",
          "samples": [{"x": 1, "y": 1}, {"x": 2, "y": 2}],
          "style": { "stroke": "#1C2430", "strokeWidth": 2 }
        }]
      }
    })")))
              .applied);
    const std::size_t depthBefore = doc.undoDepth();
    const std::string preMove = doc.snapshotString();
    const DocNode *sg0 = doc.find("sg_1");
    CHECK(sg0 && geomNear(sg0->transform.x, 0));

    doc.beginGesture();
    for (int i = 0; i < 40; ++i)
        doc.previewManipulationFrame();
    CHECK(doc.intermediateFrameCount() == 40);
    CHECK(doc.undoDepth() == depthBefore);
    CHECK(doc.snapshotString() == preMove);

    CHECK(doc.commitOp(opFromJson(parseJson(R"({
      "opId": "mv-1",
      "type": "set_smart_transform",
      "source": "epaper",
      "payload": {
        "id": "sg_1",
        "transform": { "x": 50, "y": 10, "rotation": 0, "scaleX": 1, "scaleY": 1 }
      }
    })")))
              .applied);
    CHECK(doc.undoDepth() == depthBefore + 1);
    const DocNode *sg1 = doc.find("sg_1");
    CHECK(sg1 && geomNear(sg1->transform.x, 50) && geomNear(sg1->transform.y, 10));

    CHECK(doc.undo().restored);
    CHECK(doc.snapshotString() == preMove);
    const DocNode *sg2 = doc.find("sg_1");
    CHECK(sg2 && geomNear(sg2->transform.x, 0) && geomNear(sg2->transform.y, 0));
}

/** @SRS-EP-07 Viewport tool and selection do not push undo */
static void test_undo_viewport_tool_selection_do_not_push()
{
    DeviceDocument doc;
    CHECK(doc.undoDepth() == 0);
    doc.applyViewportPan(12, -4);
    doc.applyToolSwitch("ink_box");
    doc.applySelectionChange(std::string("sg_1"));
    CHECK(doc.undoDepth() == 0);
    CHECK(doc.publishQueue().empty());
    CHECK(doc.viewportPanX() == 12);
    CHECK(doc.uiTool() == "ink_box");
    CHECK(doc.selectionId() && *doc.selectionId() == "sg_1");
}

/** @SRS-EP-07 Ring overflow drops the oldest entry */
static void test_undo_ring_overflow_drops_oldest()
{
    DeviceDocument doc;
    const std::string t0 = doc.snapshotString();
    for (int i = 0; i < 20; ++i)
        CHECK(doc.commitOp(makeAppendInkOp(std::string("n") + std::to_string(i), double(i), 0))
                  .applied);
    CHECK(doc.undoDepth() == 20);
    CHECK(doc.oldestEntry());
    CHECK(doc.entrySnapshotString(*doc.oldestEntry()) == t0);

    CHECK(doc.commitOp(makeAppendInkOp("n20", 20, 0)).applied);
    CHECK(doc.undoDepth() == 20);
    CHECK(doc.oldestEntry());
    CHECK(doc.entrySnapshotString(*doc.oldestEntry()) != t0);
    CHECK(doc.find("n0"));
    CHECK(doc.find("n20"));
}

/** @SRS-EP-07 Undo requested mid-gesture is deferred */
static void test_undo_mid_gesture_is_deferred()
{
    DeviceDocument doc;
    CHECK(doc.commitOp(makeAppendInkOp("keep")).applied);
    const std::string t1 = doc.snapshotString();
    const int inkBefore = doc.inkCount();

    doc.beginGesture();
    for (int i = 0; i < 8; ++i)
        doc.previewManipulationFrame();
    CHECK(doc.gestureInFlight());
    const UndoResult latched = doc.undo();
    CHECK(latched.latched);
    CHECK(!latched.restored);
    CHECK(doc.undoLatched());
    CHECK(doc.snapshotString() == t1);
    CHECK(doc.inkCount() == inkBefore);
    CHECK(doc.find("keep"));

    CHECK(doc.commitOp(makeAppendInkOp("temp")).applied);
    CHECK(!doc.gestureInFlight());
    CHECK(!doc.undoLatched());
    CHECK(doc.snapshotString() == t1);
    CHECK(doc.find("keep"));
    CHECK(!doc.find("temp"));
    CHECK(doc.undoDepth() == 1);
    CHECK(doc.publishQueue().back().op.type == "restore_snapshot");
}

/** @SRS-EP-07 Undo publishes as a change */
static void test_undo_publishes_restore_snapshot()
{
    DeviceDocument doc;
    CHECK(doc.commitOp(makeAppendInkOp("pub")).applied);
    const std::size_t q0 = doc.publishQueue().size();
    CHECK(q0 == 1);

    std::vector<std::int64_t> ns;
    ns.reserve(20);
    for (int i = 0; i < 20; ++i) {
        CHECK(doc.commitOp(makeAppendInkOp(std::string("lat") + std::to_string(i), double(i), 1))
                  .applied);
        const auto t0 = std::chrono::steady_clock::now();
        CHECK(doc.undo().restored);
        const auto t1 = std::chrono::steady_clock::now();
        ns.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }
    const auto p = summarizeNs(ns);
    std::cout << "undo request→restored n=" << p.n << " p50=" << p.p50Ns << "ns p95=" << p.p95Ns
              << "ns (p95=" << nsToUs(p.p95Ns) << "us host analog, bar 500ms)\n";
    CHECK(p.p95Ns <= 500000000); // 500 ms host analog — not device p95

    CHECK(doc.undo().restored); // undo the original "pub"
    CHECK(doc.publishQueue().size() > q0);
    const DocChange &last = doc.publishQueue().back();
    CHECK(last.op.type == "restore_snapshot");
    const JsonValue *document = last.op.payload.get("document");
    CHECK(document);
    CHECK(stringify(*document) == doc.snapshotString());
    CHECK(doc.inkCount() == 0);
}

/** @SRS-EP-07 An accepted document load clears the ring */
static void test_undo_accepted_doc_load_clears_ring()
{
    DeviceDocument doc;
    CHECK(doc.commitOp(makeAppendInkOp("pre1")).applied);
    CHECK(doc.commitOp(makeAppendInkOp("pre2", 5, 5)).applied);
    const std::string preLoad = doc.snapshotString();
    CHECK(doc.undoDepth() == 2);
    doc.clearPublishQueue();
    CHECK(doc.publishQueue().empty());

    const JsonValue loaded = parseJson(R"({
      "version": 1,
      "status": "open",
      "rootChildren": [{
        "id": "loaded",
        "kind": "ink",
        "samples": [{"x": 9, "y": 9}, {"x": 10, "y": 10}],
        "style": {"stroke": "#1C2430", "strokeWidth": 2}
      }]
    })");
    doc.onAcceptedDocLoad(loaded);
    CHECK(doc.undoDepth() == 0);
    CHECK(doc.find("loaded"));
    CHECK(!doc.find("pre1"));
    CHECK(!doc.find("pre2"));

    const UndoResult u = doc.undo();
    CHECK(u.noop);
    CHECK(doc.find("loaded"));
    CHECK(!doc.find("pre1"));
    CHECK(doc.snapshotString() != preLoad);
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
    test_undo_structural_op_pushes_pre_op_snapshot();
    test_undo_restores_prior_tree_exactly();
    test_undo_one_gesture_one_entry();
    test_undo_viewport_tool_selection_do_not_push();
    test_undo_ring_overflow_drops_oldest();
    test_undo_mid_gesture_is_deferred();
    test_undo_publishes_restore_snapshot();
    test_undo_accepted_doc_load_clears_ring();

    if (g_fails) {
        std::cerr << g_fails << " check(s) failed\n";
        return 1;
    }
    std::cout << "device_document_test: checks passed\n";
    return 0;
}
