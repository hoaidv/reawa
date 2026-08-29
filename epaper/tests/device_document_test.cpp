/**
 * Host tests for STORY-EP-014 / STORY-EP-059 / STORY-EP-060 / STORY-EP-061.
 * Maps ingest-stroke.feature, undo-ring.feature, undo-fail-safe.feature,
 * undo-queue.feature. No Qt.
 *
 * Build: ./tests/run_device_document_test.sh
 */

#include "document/device_document.hpp"
#include "document/ingest_stroke.hpp"
#include "document/connector_warp.hpp"
#include "debug/latency_probe.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
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

static JsonValue makeAppendInkOp(const std::string &id, double x = 10, double y = 20)
{
    const std::string json = std::string("{\"opId\":\"op-") + id + "\",\"type\":\"append_ink\","
                                                                  "\"source\":\"epaper\",\"payload\":{\"id\":\""
                             + id + "\",\"samples\":[{\"x\":" + std::to_string(x) + ",\"y\":"
                             + std::to_string(y) + "},{\"x\":" + std::to_string(x + 2) + ",\"y\":"
                             + std::to_string(y + 2)
                             + "}],\"style\":{\"stroke\":\"#1C2430\",\"strokeWidth\":2}}}";
    return parseJson(json);
}

static bool geomNear(double a, double b, double eps = 1.0)
{
    return std::abs(a - b) <= eps;
}

static std::string fixturePath(const char *name)
{
    return std::string("tests/fixtures/ops/") + name;
}

static void test_append_ink_fixture_needs_parent()
{
    DeviceDocument doc;
    const JsonValue op = loadJsonFile(fixturePath("append_ink.json"));
    CHECK(op.getString("type") == "append_ink");
    CHECK(op.getString("source") == "epaper");
    const ApplyResult r = doc.applyJson(op);
    CHECK(!r.applied);
    CHECK(r.reason == "bad_parent:frm_1");
    CHECK(doc.inkCount() == 0);
    CHECK(doc.nodeCount() == 0);
}

static void test_create_primitive_fixture()
{
    DeviceDocument doc;
    const JsonValue op = loadJsonFile(fixturePath("create_primitive.json"));
    CHECK(op.getString("type") == "create_primitive");
    CHECK(op.getString("source") == "infini");
    CHECK(doc.applyJson(op).applied);
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
    CHECK(doc.applyJson(parseJson(R"({
      "opId": "f1",
      "type": "create_frame",
      "payload": { "id": "frm_1", "bounds": { "minX": 0, "minY": 0, "maxX": 400, "maxY": 300 } }
    })"))
              .applied);
    CHECK(doc.applyJson(loadJsonFile(fixturePath("append_ink.json"))).applied);
    CHECK(doc.applyJson(loadJsonFile(fixturePath("create_primitive.json"))).applied);

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

    const ApplyResult dup = doc.applyJson(loadJsonFile(fixturePath("append_ink.json")));
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
    const ApplyResult r = doc.applyJson(opEnvelope("x", "insert_node", JsonValue::object({})));
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

static JsonValue makeCreateFrameOp(const std::string &opId, const std::string &id, double x, double y,
                                   double w, double h)
{
    JsonValue::Object b;
    b.emplace_back("minX", JsonValue::number(x));
    b.emplace_back("minY", JsonValue::number(y));
    b.emplace_back("maxX", JsonValue::number(x + w));
    b.emplace_back("maxY", JsonValue::number(y + h));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    return opEnvelope(opId, "create_frame", JsonValue::object(std::move(payload)));
}

static JsonValue makeSmartGroupOp(const std::string &opId, const std::string &id, double x, double y,
                                  double w, double h)
{
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
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    payload.emplace_back("transform", JsonValue::object(std::move(t)));
    payload.emplace_back("inkScaleMode", JsonValue::string("fixedInk"));
    payload.emplace_back("children", JsonValue::array({}));
    return opEnvelope(opId, "create_smart_group", JsonValue::object(std::move(payload)));
}

static JsonValue makeSetTransformOp(const std::string &opId, const std::string &id, double x, double y)
{
    JsonValue::Object t;
    t.emplace_back("x", JsonValue::number(x));
    t.emplace_back("y", JsonValue::number(y));
    t.emplace_back("rotation", JsonValue::number(0));
    t.emplace_back("scaleX", JsonValue::number(1));
    t.emplace_back("scaleY", JsonValue::number(1));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("transform", JsonValue::object(std::move(t)));
    return opEnvelope(opId, "set_smart_transform", JsonValue::object(std::move(payload)));
}

static JsonValue makeCreateConnectorOp(const std::string &opId, const std::string &id,
                                       const std::string &fromId, const std::string &toId)
{
    JsonValue::Object from;
    from.emplace_back("nodeId", JsonValue::string(fromId));
    JsonValue::Object to;
    to.emplace_back("nodeId", JsonValue::string(toId));
    JsonValue::Array spine;
    JsonValue::Object a, b;
    a.emplace_back("x", JsonValue::number(0));
    a.emplace_back("y", JsonValue::number(0));
    b.emplace_back("x", JsonValue::number(40));
    b.emplace_back("y", JsonValue::number(0));
    spine.push_back(JsonValue::object(std::move(a)));
    spine.push_back(JsonValue::object(std::move(b)));
    JsonValue::Object rest;
    rest.emplace_back("spine", JsonValue::array(std::move(spine)));
    rest.emplace_back("offsets", JsonValue::array({}));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("from", JsonValue::object(std::move(from)));
    payload.emplace_back("to", JsonValue::object(std::move(to)));
    payload.emplace_back("warpStyle", JsonValue::string("morph"));
    payload.emplace_back("restShape", JsonValue::object(std::move(rest)));
    return opEnvelope(opId, "create_connector", JsonValue::object(std::move(payload)));
}

static JsonValue makeRemoveNodeOp(const std::string &opId, const std::string &id)
{
    return opEnvelope(opId, "remove_node", JsonValue::object({{"id", JsonValue::string(id)}}));
}

static JsonValue makeInkSamplesOp(const std::string &opId, const std::string &id,
                              const std::vector<std::pair<double, double>> &xy)
{
    JsonValue::Array samples;
    for (const auto &p : xy) {
        JsonValue::Object s;
        s.emplace_back("x", JsonValue::number(p.first));
        s.emplace_back("y", JsonValue::number(p.second));
        samples.push_back(JsonValue::object(std::move(s)));
    }
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("samples", JsonValue::array(std::move(samples)));
    payload.emplace_back("style", parseJson(R"({"stroke":"#1C2430","strokeWidth":2})"));
    return opEnvelope(opId, "append_ink", JsonValue::object(std::move(payload)));
}

static bool samplesMatch(const DocNode *n, const std::vector<std::pair<double, double>> &xy,
                         double eps = 1.0)
{
    if (!n || n->samples.size() != xy.size())
        return false;
    for (size_t i = 0; i < xy.size(); ++i) {
        if (!geomNear(n->samples[i].x, xy[i].first, eps)
            || !geomNear(n->samples[i].y, xy[i].second, eps))
            return false;
    }
    return true;
}

static std::vector<std::unique_ptr<DocEdit>> gestureParts(std::initializer_list<JsonValue> js)
{
    std::vector<std::unique_ptr<DocEdit>> out;
    for (const auto &j : js)
        out.push_back(DocEdit::fromJson(j));
    return out;
}

static std::vector<std::string> compoundInnerTypes(const JsonValue &op)
{
    std::vector<std::string> types;
    const JsonValue *payload = op.get("payload");
    const JsonValue *ops = payload ? payload->get("ops") : nullptr;
    if (!ops || !ops->isArray())
        return types;
    for (const auto &j : ops->asArray())
        types.push_back(j.getString("type"));
    return types;
}

static bool compoundContains(const JsonValue &op, const std::string &innerType)
{
    for (const auto &t : compoundInnerTypes(op)) {
        if (t == innerType)
            return true;
    }
    return false;
}

static bool entryIsInverseShape(const UndoRingEntry &e)
{
    return !e.forwardOpId.empty() && !DeviceDocument::entryHasSnapshot(e);
}

/** @SRS-EP-09 Structural gesture commit pushes one inverse entry */
static void test_undo_structural_gesture_pushes_inverse_entry()
{
    DeviceDocument doc;
    CHECK(doc.undoDepth() == 0);
    CHECK(doc.commitJson(makeCreateFrameOp("op-frame-1", "frm_1", 0, 0, 100, 80)).applied);
    CHECK(doc.undoDepth() == 1);
    const UndoRingEntry *e = doc.newestEntry();
    CHECK(e);
    CHECK(e->forwardOpId == "op-frame-1");
    CHECK(e->seq == 1);
    CHECK(e->inverses.size() == 1);
    CHECK(kindEq(e->inverses[0]->kind(), edit_kind::kRemoveNode));
    const auto *rm = dynamic_cast<const RemoveNodeEdit *>(e->inverses[0].get());
    CHECK(rm && rm->nodeId() == "frm_1");
    CHECK(e->targets.size() == 1);
    CHECK(e->targets[0].nodeId == "frm_1");
    CHECK(e->targets[0].prevLastOpId.empty());
    CHECK(entryIsInverseShape(*e));
    CHECK(!DeviceDocument::entryHasSnapshot(*e));
    const DocNode *f = doc.find("frm_1");
    CHECK(f && f->lastOpId == "op-frame-1");

    DeviceDocument raw;
    CHECK(raw.applyJson(makeAppendInkOp("raw")).applied);
    CHECK(raw.undoDepth() == 0);

    DeviceDocument rej;
    CHECK(!rej.commitJson(opEnvelope("bad", "append_ink",
                                     JsonValue::object({{"id", JsonValue::string("x")}})))
               .applied);
    CHECK(rej.undoDepth() == 0);
    CHECK(rej.publishQueue().empty());
}

/** @SRS-EP-07 One completed gesture is one undo entry */
static void test_undo_one_gesture_one_entry()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeSmartGroupOp("sg-1", "B", 40, 60, 40, 40)).applied);
    const std::size_t depthBefore = doc.undoDepth();
    const DocNode *sg0 = doc.find("B");
    CHECK(sg0 && geomNear(sg0->transform.x, 40) && geomNear(sg0->transform.y, 60));

    doc.beginGesture();
    for (int i = 0; i < 12; ++i)
        doc.previewManipulationFrame();
    CHECK(doc.intermediateFrameCount() == 12);
    CHECK(doc.undoDepth() == depthBefore);

    CHECK(doc.commitJson(makeSetTransformOp("op-move-g1", "B", 88, 91)).applied);
    CHECK(doc.undoDepth() == depthBefore + 1);
    const UndoRingEntry *e = doc.newestEntry();
    CHECK(e && e->inverses.size() == 1);
    CHECK(kindEq(e->inverses[0]->kind(), edit_kind::kSetSmartTransform));
    const auto *st = dynamic_cast<const SetSmartTransformEdit *>(e->inverses[0].get());
    CHECK(st && geomNear(st->toT().x, 40) && geomNear(st->toT().y, 60));

    CHECK(doc.undo().restored);
    const DocNode *sg2 = doc.find("B");
    CHECK(sg2 && geomNear(sg2->transform.x, 40, 1) && geomNear(sg2->transform.y, 60, 1));
}

/** Live preview mutates the tree before commit; inverse must still hold origin. */
static void test_undo_after_live_preview_restores_origin()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeSmartGroupOp("sg-1", "B", 40, 60, 40, 40)).applied);
    const DocNode *sg0 = doc.find("B");
    CHECK(sg0);
    const SmartTransform originT = sg0->transform;
    const SmartBounds originB = sg0->smartBounds;
    SmartTransform destT = originT;
    destT.x = 88;
    destT.y = 91;
    SmartBounds destB = originB;
    destB.x = 88;
    destB.y = 91;
    CHECK(doc.applyLiveSmartGeometry("B", destT, destB));
    SetSmartTransformEdit edit("op-live-move", "B", originT, originB, destT, destB, true);
    CHECK(doc.commitEdit(edit).applied);
    const UndoRingEntry *e = doc.newestEntry();
    CHECK(e && e->inverses.size() == 1);
    CHECK(kindEq(e->inverses[0]->kind(), edit_kind::kSetSmartTransform));
    const auto *st = dynamic_cast<const SetSmartTransformEdit *>(e->inverses[0].get());
    CHECK(st && geomNear(st->toT().x, 40) && geomNear(st->toT().y, 60));
    CHECK(doc.undo().restored);
    const DocNode *sg2 = doc.find("B");
    CHECK(sg2 && geomNear(sg2->transform.x, 40, 1) && geomNear(sg2->transform.y, 60, 1));
}

/** Live resize mutates scale before commit; inverse must restore origin scale. */
static void test_undo_after_live_preview_resize_restores_scale()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeSmartGroupOp("sg-1", "B", 40, 60, 40, 40)).applied);
    const DocNode *sg0 = doc.find("B");
    CHECK(sg0);
    const SmartTransform originT = sg0->transform;
    const SmartBounds originB = sg0->smartBounds;
    CHECK(geomNear(originT.scaleX, 1) && geomNear(originT.scaleY, 1));
    SmartTransform destT = originT;
    destT.scaleX = 1.5;
    destT.scaleY = 1.25;
    destT.x = 32;
    destT.y = 55;
    CHECK(doc.applyLiveSmartGeometry("B", destT, originB));
    SetSmartTransformEdit edit("op-live-resize", "B", originT, originB, destT, originB, true);
    CHECK(doc.commitEdit(edit).applied);
    const DocNode *mid = doc.find("B");
    CHECK(mid && geomNear(mid->transform.scaleX, 1.5) && geomNear(mid->transform.scaleY, 1.25));
    const UndoRingEntry *e = doc.newestEntry();
    CHECK(e && e->inverses.size() == 1);
    const auto *st = dynamic_cast<const SetSmartTransformEdit *>(e->inverses[0].get());
    CHECK(st && geomNear(st->toT().scaleX, 1) && geomNear(st->toT().scaleY, 1));
    CHECK(doc.undo().restored);
    const DocNode *sg2 = doc.find("B");
    CHECK(sg2 && geomNear(sg2->transform.scaleX, 1) && geomNear(sg2->transform.scaleY, 1));
    CHECK(geomNear(sg2->transform.x, 40, 1) && geomNear(sg2->transform.y, 60, 1));
}

/** Move then resize must both commit; duplicate sst-N would leave resize un-undoable. */
static void test_undo_resize_after_move_restores_scale()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeSmartGroupOp("sg-1", "B", 40, 60, 40, 40)).applied);
    const DocNode *sg0 = doc.find("B");
    CHECK(sg0);
    const SmartTransform originT = sg0->transform;
    const SmartBounds originB = sg0->smartBounds;
    SmartTransform moved = originT;
    moved.x = 88;
    moved.y = 91;
    CHECK(doc.applyLiveSmartGeometry("B", moved, originB));
    SetSmartTransformEdit mv("sst-1", "B", originT, originB, moved, originB, true);
    CHECK(doc.commitEdit(mv).applied);
    const DocNode *afterMove = doc.find("B");
    CHECK(afterMove);
    const SmartTransform moveT = afterMove->transform;
    SmartTransform resized = moveT;
    resized.scaleX = 2;
    resized.scaleY = 2;
    CHECK(doc.applyLiveSmartGeometry("B", resized, originB));
    SetSmartTransformEdit rz("sst-1", "B", moveT, originB, resized, originB, true);
    const ApplyResult dup = doc.commitEdit(rz);
    CHECK(!dup.applied);
    CHECK(dup.reason == "duplicate_opId");
    SetSmartTransformEdit rz2("sst-2", "B", moveT, originB, resized, originB, true);
    CHECK(doc.commitEdit(rz2).applied);
    CHECK(doc.undo().restored);
    const DocNode *sg2 = doc.find("B");
    CHECK(sg2 && geomNear(sg2->transform.scaleX, 1) && geomNear(sg2->transform.scaleY, 1));
    CHECK(geomNear(sg2->transform.x, 88, 1) && geomNear(sg2->transform.y, 91, 1));
}

/** @SRS-EP-07 Undo with matching lastOpId restores stored pre-op fields */
static void test_undo_matching_lastopid_restores_pre_op_fields()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeSmartGroupOp("op-b", "B", 100, 200, 80, 60)).applied);
    CHECK(doc.commitJson(makeSetTransformOp("op-move-1", "B", 140, 228)).applied);
    const DocNode *b0 = doc.find("B");
    CHECK(b0 && b0->lastOpId == "op-move-1");
    CHECK(doc.undo().restored);
    const DocNode *b1 = doc.find("B");
    CHECK(b1 && geomNear(b1->transform.x, 100, 1) && geomNear(b1->transform.y, 200, 1));
    CHECK(geomNear(b1->smartBounds.width, 80, 1) && geomNear(b1->smartBounds.height, 60, 1));
    CHECK(b1->lastOpId == "op-b");
    CHECK(doc.restoreSnapshotQueued() == 0);
    CHECK(doc.publishQueue().back().op.getString("type") == "set_smart_transform");
}

/** @SRS-EP-07 Viewport tool selection clipboard-slot and copy do not push undo */
static void test_undo_viewport_tool_selection_copy_do_not_push()
{
    DeviceDocument doc;
    CHECK(doc.applyJson(makeAppendInkOp("I1")).applied);
    CHECK(doc.undoDepth() == 0);
    doc.applySelectionChange("I1");
    doc.applyViewportPan(40, 0);
    doc.applyToolSwitch("selection");
    doc.applySelectionChange("");
    doc.copyToClipboard({"I1"});
    CHECK(doc.undoDepth() == 0);
    CHECK(doc.clipboardSlot().size() == 1);
    CHECK(doc.clipboardSlot()[0].id == "I1");
    CHECK(doc.viewportPanX() == 40);
    CHECK(doc.uiTool() == "selection");

    DeviceDocument empty;
    CHECK(empty.undoDepth() == 0);
    empty.applyViewportPan(40, 0);
    empty.applyToolSwitch("selection");
    empty.applySelectionChange("I1");
    empty.applySelectionChange("");
    CHECK(empty.undoDepth() == 0);
}

/** @SRS-EP-07 Ring overflow drops the oldest entry */
static void test_undo_ring_overflow_drops_oldest()
{
    DeviceDocument doc;
    for (int i = 1; i <= 20; ++i) {
        const std::string n = (i < 10 ? "op-00" : "op-0") + std::to_string(i);
        const std::string id = std::string("frm_") + std::to_string(i);
        CHECK(doc.commitJson(makeCreateFrameOp(n, id, double(i), 0, 10, 10)).applied);
    }
    CHECK(doc.undoDepth() == 20);
    CHECK(doc.oldestEntry() && doc.oldestEntry()->forwardOpId == "op-001");
    CHECK(doc.commitJson(makeCreateFrameOp("op-021", "frm_21", 21, 0, 10, 10)).applied);
    CHECK(doc.undoDepth() == 20);
    CHECK(doc.oldestEntry() && doc.oldestEntry()->forwardOpId == "op-002");
    CHECK(doc.newestEntry() && doc.newestEntry()->forwardOpId == "op-021");
}

/** @SRS-EP-07 Undo requested mid-gesture is deferred */
static void test_undo_mid_gesture_is_deferred()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeSmartGroupOp("op-b", "B", 10, 20, 40, 40)).applied);
    CHECK(doc.undoDepth() == 1);
    const int nodesBefore = doc.nodeCount();

    doc.beginGesture();
    for (int i = 0; i < 4; ++i)
        doc.previewManipulationFrame();
    CHECK(doc.gestureInFlight());
    const UndoResult latched = doc.undo();
    CHECK(latched.latched);
    CHECK(!latched.restored);
    CHECK(doc.undoLatched());
    CHECK(doc.nodeCount() == nodesBefore);
    CHECK(doc.find("B"));

    CHECK(doc.commitJson(makeSetTransformOp("op-g", "B", 50, 60)).applied);
    CHECK(!doc.gestureInFlight());
    CHECK(!doc.undoLatched());
    const DocNode *b = doc.find("B");
    CHECK(b && geomNear(b->transform.x, 10, 1) && geomNear(b->transform.y, 20, 1));
    CHECK(doc.restoreSnapshotQueued() == 0);
}

/** @SRS-EP-07 Undo publishes counterpart, never restore_snapshot */
static void test_undo_publishes_counterpart_not_snapshot()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeAppendInkOp("pub")).applied);
    const std::size_t q0 = doc.publishQueue().size();

    std::vector<std::int64_t> ns;
    ns.reserve(20);
    for (int i = 0; i < 20; ++i) {
        CHECK(doc.commitJson(makeAppendInkOp(std::string("lat") + std::to_string(i), double(i), 1))
                  .applied);
        const auto t0 = std::chrono::steady_clock::now();
        CHECK(doc.undo().restored);
        const auto t1 = std::chrono::steady_clock::now();
        ns.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }
    const auto p = summarizeNs(ns);
    std::cout << "undo request→restored n=" << p.n << " p50=" << p.p50Ns << "ns p95=" << p.p95Ns
              << "ns (p95=" << nsToUs(p.p95Ns) << "us host analog, bar 500ms)\n";
    CHECK(p.p95Ns <= 500000000);

    CHECK(doc.undo().restored);
    CHECK(doc.publishQueue().size() > q0);
    CHECK(doc.publishQueue().back().op.getString("type") == "remove_node");
    CHECK(doc.restoreSnapshotQueued() == 0);
    CHECK(doc.inkCount() == 0);
}

/** @SRS-EP-07 An accepted document load empties undo and redo */
static void test_undo_accepted_doc_load_clears_ring()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeAppendInkOp("pre1")).applied);
    CHECK(doc.commitJson(makeAppendInkOp("pre2", 5, 5)).applied);
    CHECK(doc.commitJson(makeAppendInkOp("pre3", 6, 6)).applied);
    CHECK(doc.undo().restored);
    CHECK(doc.undoDepth() == 2);
    CHECK(doc.redoDepth() == 1);
    const std::string preLoad = doc.snapshotString();
    doc.clearPublishQueue();

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
    CHECK(doc.redoDepth() == 0);
    CHECK(doc.find("loaded"));
    CHECK(!doc.find("pre1"));
    const UndoResult u = doc.undo();
    CHECK(u.noop);
    CHECK(doc.find("loaded"));
    CHECK(!doc.find("pre1"));
    CHECK(doc.snapshotString() != preLoad);
}

/** @SRS-EP-09 Bound-node drag does not bump connector lastOpId */
static void test_bound_node_drag_does_not_bump_connector_lastopid()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeSmartGroupOp("op-g-create", "G", 10, 20, 80, 60)).applied);
    CHECK(doc.commitJson(makeSmartGroupOp("op-h-create", "H", 200, 20, 80, 60)).applied);
    CHECK(doc.commitJson(makeCreateConnectorOp("op-c-create", "C", "G", "H")).applied);
    const DocNode *c0 = doc.find("C");
    CHECK(c0 && c0->lastOpId == "op-c-create");
    const DocNode *g0 = doc.find("G");
    CHECK(g0 && g0->lastOpId == "op-g-create");

    CHECK(doc.commitJson(makeSetTransformOp("op-g-move", "G", 50, 60)).applied);
    refreshAllConnectorWarps(doc);
    const DocNode *g1 = doc.find("G");
    CHECK(g1 && g1->lastOpId == "op-g-move");
    const DocNode *c1 = doc.find("C");
    CHECK(c1 && c1->lastOpId == "op-c-create");
    CHECK(doc.undoDepth() >= 1);
    bool gInTargets = false;
    const UndoRingEntry *e = doc.newestEntry();
    CHECK(e);
    for (const auto &t : e->targets) {
        if (t.nodeId == "G")
            gInTargets = true;
        CHECK(t.nodeId != "C");
    }
    CHECK(gInTargets);
}

/** @SRS-EP-09 Last-live-pose cache does not bump connector lastOpId */
static void test_last_live_pose_does_not_bump_connector_lastopid()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeSmartGroupOp("op-g-create", "G", 10, 20, 80, 60)).applied);
    CHECK(doc.commitJson(makeSmartGroupOp("op-h-create", "H", 200, 20, 80, 60)).applied);
    CHECK(doc.commitJson(makeCreateConnectorOp("op-c-create", "C", "G", "H")).applied);
    const std::size_t depth = doc.undoDepth();
    CHECK(doc.writeLastLivePose("C", 10, 20));
    const DocNode *c = doc.find("C");
    CHECK(c && c->lastOpId == "op-c-create");
    CHECK(c->fromPose.valid);
    CHECK(geomNear(c->fromPose.x, 10) && geomNear(c->fromPose.y, 20));
    CHECK(doc.undoDepth() == depth);
}

/** @SRS-EP-07 Ungroup inverse does not delete children */
static void test_ungroup_inverse_does_not_delete_children()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeAppendInkOp("I1", 1, 1)).applied);
    CHECK(doc.commitJson(makeAppendInkOp("I2", 3, 3)).applied);
    CHECK(doc.rootChildren.size() >= 2);
    CHECK(doc.rootChildren[0].id == "I1");
    CHECK(doc.rootChildren[1].id == "I2");

    JsonValue kids = JsonValue::array({
        parseJson(R"({"id":"I1","kind":"ink","role":"content","samples":[{"x":1,"y":1},{"x":3,"y":3}],"style":{"stroke":"#1C2430","strokeWidth":2}})"),
        parseJson(R"({"id":"I2","kind":"ink","role":"content","samples":[{"x":3,"y":3},{"x":5,"y":5}],"style":{"stroke":"#1C2430","strokeWidth":2}})"),
    });
    JsonValue::Object b;
    b.emplace_back("x", JsonValue::number(0));
    b.emplace_back("y", JsonValue::number(0));
    b.emplace_back("width", JsonValue::number(40));
    b.emplace_back("height", JsonValue::number(40));
    JsonValue::Object t;
    t.emplace_back("x", JsonValue::number(0));
    t.emplace_back("y", JsonValue::number(0));
    t.emplace_back("rotation", JsonValue::number(0));
    t.emplace_back("scaleX", JsonValue::number(1));
    t.emplace_back("scaleY", JsonValue::number(1));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string("G"));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    payload.emplace_back("transform", JsonValue::object(std::move(t)));
    payload.emplace_back("inkScaleMode", JsonValue::string("fixedInk"));
    payload.emplace_back("captureIds", JsonValue::array({JsonValue::string("I1"), JsonValue::string("I2")}));
    payload.emplace_back("children", std::move(kids));
    CHECK(doc.commitJson(opEnvelope("op-group-1", "create_smart_group",
                                    JsonValue::object(std::move(payload))))
              .applied);
    const DocNode *g = doc.find("G");
    CHECK(g && g->lastOpId == "op-group-1");
    CHECK(doc.find("I1") && doc.find("I2"));

    CHECK(doc.undo().restored);
    CHECK(doc.find("I1"));
    CHECK(doc.find("I2"));
    CHECK(!doc.find("G"));
    CHECK(doc.rootChildren.size() >= 2);
    CHECK(doc.rootChildren[0].id == "I1");
    CHECK(doc.rootChildren[1].id == "I2");
}

/** @SRS-EP-07 Redo restores the undone forward fields */
static void test_redo_restores_forward_fields()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeSmartGroupOp("op-b", "B", 100, 200, 80, 60)).applied);
    CHECK(doc.commitJson(makeSetTransformOp("op-move-1", "B", 140, 228)).applied);
    CHECK(doc.undo().restored);
    const DocNode *b0 = doc.find("B");
    CHECK(b0);
    CHECK(b0->lastOpId == "op-b");
    CHECK(doc.redo().restored);
    const DocNode *b1 = doc.find("B");
    CHECK(b1 && geomNear(b1->transform.x, 140, 1) && geomNear(b1->transform.y, 228, 1));
    CHECK(b1->lastOpId == "op-move-1");
    CHECK(doc.restoreSnapshotQueued() == 0);
    CHECK(doc.commitJson(makeCreateFrameOp("op-later", "frm_z", 0, 0, 10, 10)).applied);
    CHECK(doc.redoDepth() == 0);
}

/** @SRS-EP-07 Empty redo is a no-op */
static void test_redo_empty_is_noop()
{
    DeviceDocument empty;
    CHECK(empty.redo().noop);
    DeviceDocument doc;
    CHECK(doc.commitJson(makeSmartGroupOp("op-b", "B", 100, 200, 40, 40)).applied);
    CHECK(doc.redo().noop);
    const DocNode *b = doc.find("B");
    CHECK(b && geomNear(b->transform.x, 100) && geomNear(b->transform.y, 200));
    CHECK(doc.restoreSnapshotQueued() == 0);
    CHECK(doc.commitJson(makeAppendInkOp("gone")).applied);
    CHECK(doc.undo().restored);
    CHECK(doc.redoDepth() == 1);
    doc.onAcceptedDocLoad();
    CHECK(doc.redoDepth() == 0);
    CHECK(doc.redo().noop);
}

/** @SRS-EP-13 F21 absences no-op and live targets apply */
static void test_f21_absences_noop_live_targets_apply()
{
    DeviceDocument doc;
    CHECK(doc.applyJson(makeSmartGroupOp("op-a", "A", 10, 20, 40, 40)).applied);
    CHECK(doc.applyJson(makeSmartGroupOp("op-b", "B", 80, 90, 40, 40)).applied);
    CHECK(doc.commitGesture("op-move-ab",
                            gestureParts({makeSetTransformOp("op-move-ab", "A", 50, 60),
                                          makeSetTransformOp("op-move-ab", "B", 120, 140)}))
              .applied);
    CHECK(doc.undoDepth() == 1);
    CHECK(doc.newestEntry() && doc.newestEntry()->forwardOpId == "op-move-ab");
    CHECK(doc.newestEntry()->inverses.size() == 2);
    CHECK(doc.applyJson(makeRemoveNodeOp("op-drop-b", "B")).applied);
    CHECK(!doc.find("B"));
    const DocNode *a0 = doc.find("A");
    CHECK(a0 && a0->lastOpId == "op-move-ab");
    CHECK(geomNear(a0->transform.x, 50) && geomNear(a0->transform.y, 60));
    CHECK(doc.redoDepth() == 0);
    const int nodesBefore = doc.nodeCount();
    const std::size_t q0 = doc.publishQueue().size();

    CHECK(doc.undo().restored);
    const DocNode *a1 = doc.find("A");
    CHECK(a1 && geomNear(a1->transform.x, 10, 1) && geomNear(a1->transform.y, 20, 1));
    CHECK(!doc.find("B"));
    CHECK(doc.nodeCount() == nodesBefore);
    CHECK(doc.undoDepth() == 0);
    CHECK(doc.redoDepth() == 1);
    CHECK(doc.publishQueue().size() == q0 + 1);
    CHECK(doc.restoreSnapshotQueued() == 0);
    CHECK(doc.errorUiShown() == 0);
}

/** @SRS-EP-13 F20 any sibling lastOpId mismatch skips the whole entry */
static void test_f20_sibling_lastopid_mismatch_skips_whole()
{
    DeviceDocument doc;
    CHECK(doc.applyJson(makeSmartGroupOp("op-a", "A", 10, 20, 40, 40)).applied);
    CHECK(doc.applyJson(makeSmartGroupOp("op-b", "B", 80, 90, 40, 40)).applied);
    CHECK(doc.commitGesture("op-move-ab",
                            gestureParts({makeSetTransformOp("op-move-ab", "A", 50, 60),
                                          makeSetTransformOp("op-move-ab", "B", 120, 140)}))
              .applied);
    CHECK(doc.applyJson(makeSetTransformOp("op-other-b", "B", 120, 140)).applied);
    const DocNode *a0 = doc.find("A");
    const DocNode *b0 = doc.find("B");
    CHECK(a0 && a0->lastOpId == "op-move-ab");
    CHECK(b0 && b0->lastOpId == "op-other-b");
    CHECK(doc.undoDepth() == 1);
    CHECK(doc.redoDepth() == 0);
    const int nodesBefore = doc.nodeCount();
    const std::size_t q0 = doc.publishQueue().size();

    const UndoResult u = doc.undo();
    CHECK(u.skipped);
    CHECK(u.noop);
    CHECK(!u.restored);
    const DocNode *a1 = doc.find("A");
    const DocNode *b1 = doc.find("B");
    CHECK(a1 && geomNear(a1->transform.x, 50) && geomNear(a1->transform.y, 60));
    CHECK(b1 && geomNear(b1->transform.x, 120) && geomNear(b1->transform.y, 140));
    CHECK(a1->lastOpId == "op-move-ab");
    CHECK(b1->lastOpId == "op-other-b");
    CHECK(doc.nodeCount() == nodesBefore);
    CHECK(doc.undoDepth() == 0);
    CHECK(doc.redoDepth() == 0);
    CHECK(doc.publishQueue().size() == q0);
    CHECK(doc.restoreSnapshotQueued() == 0);
    CHECK(doc.errorUiShown() == 0);
}

/** @SRS-EP-13 Empty undo ring is a no-op */
static void test_empty_undo_ring_is_noop()
{
    DeviceDocument doc;
    CHECK(doc.applyJson(makeSmartGroupOp("op-b", "B", 100, 200, 40, 40)).applied);
    CHECK(doc.undoDepth() == 0);
    CHECK(doc.redoDepth() == 0);
    const std::size_t q0 = doc.publishQueue().size();
    const UndoResult u = doc.undo();
    CHECK(u.noop);
    CHECK(!u.skipped);
    CHECK(!u.restored);
    const DocNode *b = doc.find("B");
    CHECK(b && geomNear(b->transform.x, 100) && geomNear(b->transform.y, 200));
    CHECK(doc.undoDepth() == 0);
    CHECK(doc.redoDepth() == 0);
    CHECK(doc.publishQueue().size() == q0);
    CHECK(doc.restoreSnapshotQueued() == 0);
    CHECK(doc.errorUiShown() == 0);
}

/** @SRS-EP-13 Pure no-op undo consumes the entry and does not push redo */
static void test_pure_noop_undo_consumes_no_redo()
{
    DeviceDocument doc;
    CHECK(doc.applyJson(makeSmartGroupOp("op-a", "A", 10, 20, 40, 40)).applied);
    CHECK(doc.applyJson(makeSmartGroupOp("op-b", "B", 80, 90, 40, 40)).applied);
    CHECK(doc.commitGesture("op-move-ab",
                            gestureParts({makeSetTransformOp("op-move-ab", "A", 50, 60),
                                          makeSetTransformOp("op-move-ab", "B", 120, 140)}))
              .applied);
    CHECK(doc.applyJson(makeRemoveNodeOp("op-drop-a", "A")).applied);
    CHECK(doc.applyJson(makeRemoveNodeOp("op-drop-b", "B")).applied);
    CHECK(!doc.find("A") && !doc.find("B"));
    CHECK(doc.undoDepth() == 1);
    CHECK(doc.redoDepth() == 0);
    const int nodesBefore = doc.nodeCount();
    const std::size_t q0 = doc.publishQueue().size();

    const UndoResult u = doc.undo();
    CHECK(u.noop);
    CHECK(!u.skipped);
    CHECK(!u.restored);
    CHECK(!doc.find("A") && !doc.find("B"));
    CHECK(doc.nodeCount() == nodesBefore);
    CHECK(doc.undoDepth() == 0);
    CHECK(doc.redoDepth() == 0);
    CHECK(doc.publishQueue().size() == q0);
    CHECK(doc.restoreSnapshotQueued() == 0);
    CHECK(doc.errorUiShown() == 0);
}

/** @SRS-EP-08 Matching undo records the counterpart op */
static void test_matching_undo_records_counterpart()
{
    DeviceDocument doc;
    CHECK(doc.applyJson(makeSmartGroupOp("op-b", "B", 100, 200, 80, 60)).applied);
    CHECK(doc.commitJson(makeSetTransformOp("op-move-1", "B", 140, 228)).applied);
    const DocNode *b0 = doc.find("B");
    CHECK(b0 && b0->lastOpId == "op-move-1");
    CHECK(doc.newestEntry() && doc.newestEntry()->forwardOpId == "op-move-1");
    const std::size_t q0 = doc.publishQueue().size();

    CHECK(doc.undo().restored);
    const DocNode *b1 = doc.find("B");
    CHECK(b1 && geomNear(b1->transform.x, 100, 1) && geomNear(b1->transform.y, 200, 1));
    CHECK(doc.publishQueue().size() == q0 + 1);
    CHECK(doc.publishQueue().back().op.getString("type") == "set_smart_transform");
    CHECK(doc.restoreSnapshotQueued() == 0);
}

/** @SRS-EP-08 Matching redo records the counterpart op */
static void test_matching_redo_records_counterpart()
{
    DeviceDocument doc;
    CHECK(doc.applyJson(makeSmartGroupOp("op-b", "B", 100, 200, 80, 60)).applied);
    CHECK(doc.commitJson(makeSetTransformOp("op-move-1", "B", 140, 228)).applied);
    CHECK(doc.undo().restored);
    const DocNode *b0 = doc.find("B");
    CHECK(b0 && b0->lastOpId == "op-b");
    CHECK(doc.redoDepth() == 1);
    const std::size_t q0 = doc.publishQueue().size();

    CHECK(doc.redo().restored);
    const DocNode *b1 = doc.find("B");
    CHECK(b1 && geomNear(b1->transform.x, 140, 1) && geomNear(b1->transform.y, 228, 1));
    CHECK(b1->lastOpId == "op-move-1");
    CHECK(doc.publishQueue().size() == q0 + 1);
    CHECK(doc.publishQueue().back().op.getString("type") == "set_smart_transform");
    CHECK(doc.restoreSnapshotQueued() == 0);
}

/** @SRS-EP-08 Multi-inverse undo records one compound not N changes */
static void test_multi_inverse_undo_records_one_compound()
{
    DeviceDocument doc;
    CHECK(doc.applyJson(makeSmartGroupOp("op-a", "A", 10, 20, 40, 40)).applied);
    CHECK(doc.applyJson(makeSmartGroupOp("op-b", "B", 80, 90, 40, 40)).applied);
    CHECK(doc.commitGesture("op-move-ab",
                            gestureParts({makeSetTransformOp("op-move-ab", "A", 50, 60),
                                          makeSetTransformOp("op-move-ab", "B", 120, 140)}))
              .applied);
    const UndoRingEntry *e = doc.newestEntry();
    CHECK(e && e->forwardOpId == "op-move-ab" && e->inverses.size() == 2);
    const std::size_t q0 = doc.publishQueue().size();

    CHECK(doc.undo().restored);
    const DocNode *a = doc.find("A");
    const DocNode *b = doc.find("B");
    CHECK(a && geomNear(a->transform.x, 10, 1) && geomNear(a->transform.y, 20, 1));
    CHECK(b && geomNear(b->transform.x, 80, 1) && geomNear(b->transform.y, 90, 1));
    CHECK(doc.publishQueue().size() == q0 + 1);
    CHECK(doc.publishQueue().back().op.getString("type") == "compound");
    CHECK(compoundInnerTypes(doc.publishQueue().back().op).size() == 2);
    CHECK(doc.restoreSnapshotQueued() == 0);
}

/** Path A sample-drop erase retired — remnant clip lives in erase_clip_test.cpp. */

/** Own later undo must unwind lastOpId to the previous forward, not stamp undo:N. */
static void seedAB(DeviceDocument &doc)
{
    CHECK(doc.commitJson(makeSmartGroupOp("createA", "A", 10, 20, 40, 40)).applied);
    CHECK(doc.commitJson(makeSetTransformOp("moveA", "A", 30, 40)).applied);
    CHECK(doc.commitJson(makeSetTransformOp("resizeA", "A", 50, 60)).applied);
    CHECK(doc.commitJson(makeAppendInkOp("ink2", 8, 8)).applied);
    CHECK(doc.commitJson(makeSmartGroupOp("createB", "B", 80, 90, 40, 40)).applied);
    CHECK(doc.commitJson(makeSetTransformOp("moveB", "B", 100, 110)).applied);
    CHECK(doc.commitJson(makeSetTransformOp("resizeA2", "A", 70, 80)).applied);
}

static void test_lastopid_unwind_allows_older_own_gestures()
{
    DeviceDocument doc;
    seedAB(doc);
    CHECK(doc.find("A") && doc.find("A")->lastOpId == "resizeA2");
    CHECK(doc.find("B") && doc.find("B")->lastOpId == "moveB");

    CHECK(doc.undo().restored);
    CHECK(doc.find("A") && geomNear(doc.find("A")->transform.x, 50, 1));
    CHECK(doc.find("A")->lastOpId == "resizeA");
    CHECK(doc.find("B")->lastOpId == "moveB");

    CHECK(doc.undo().restored);
    CHECK(doc.find("B") && geomNear(doc.find("B")->transform.x, 80, 1));
    CHECK(doc.find("B")->lastOpId == "createB");

    CHECK(doc.undo().restored);
    CHECK(!doc.find("B"));
    CHECK(doc.find("ink2"));

    CHECK(doc.undo().restored);
    CHECK(!doc.find("ink2"));

    CHECK(doc.undo().restored);
    CHECK(doc.find("A") && geomNear(doc.find("A")->transform.x, 30, 1));
    CHECK(doc.find("A")->lastOpId == "moveA");

    CHECK(doc.undo().restored);
    CHECK(doc.find("A") && geomNear(doc.find("A")->transform.x, 10, 1));
    CHECK(doc.find("A")->lastOpId == "createA");

    CHECK(doc.undo().restored);
    CHECK(!doc.find("A"));
}

static void test_lastopid_f20_still_skips_when_later_live()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeSmartGroupOp("createB", "B", 80, 90, 40, 40)).applied);
    CHECK(doc.applyJson(makeSetTransformOp("moveB", "B", 100, 110)).applied);
    CHECK(doc.find("B") && doc.find("B")->lastOpId == "moveB");
    const UndoResult u = doc.undo();
    CHECK(u.skipped);
    CHECK(!u.restored);
    CHECK(doc.find("B"));
    CHECK(geomNear(doc.find("B")->transform.x, 100, 1));
    CHECK(doc.find("B")->lastOpId == "moveB");
    CHECK(doc.undoDepth() == 0);
    CHECK(doc.redoDepth() == 0);
}

static void test_lastopid_redo_restamps_forward()
{
    DeviceDocument doc;
    seedAB(doc);
    CHECK(doc.undo().restored); // resizeA2
    CHECK(doc.find("A")->lastOpId == "resizeA");
    CHECK(doc.redo().restored);
    CHECK(doc.find("A") && geomNear(doc.find("A")->transform.x, 70, 1));
    CHECK(doc.find("A")->lastOpId == "resizeA2");
}

static void test_lastopid_redo_move_b_then_create_b_skips()
{
    DeviceDocument doc;
    seedAB(doc);
    CHECK(doc.undo().restored); // 8
    CHECK(doc.undo().restored); // 7
    CHECK(doc.find("B")->lastOpId == "createB");
    CHECK(doc.redo().restored); // redo 7 — B moved again
    CHECK(doc.find("B") && geomNear(doc.find("B")->transform.x, 100, 1));
    CHECK(doc.find("B")->lastOpId == "moveB");
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
    test_undo_structural_gesture_pushes_inverse_entry();
    test_undo_one_gesture_one_entry();
    test_undo_after_live_preview_restores_origin();
    test_undo_after_live_preview_resize_restores_scale();
    test_undo_resize_after_move_restores_scale();
    test_undo_matching_lastopid_restores_pre_op_fields();
    test_undo_viewport_tool_selection_copy_do_not_push();
    test_undo_ring_overflow_drops_oldest();
    test_undo_mid_gesture_is_deferred();
    test_undo_publishes_counterpart_not_snapshot();
    test_undo_accepted_doc_load_clears_ring();
    test_bound_node_drag_does_not_bump_connector_lastopid();
    test_last_live_pose_does_not_bump_connector_lastopid();
    test_ungroup_inverse_does_not_delete_children();
    test_redo_restores_forward_fields();
    test_redo_empty_is_noop();
    test_f21_absences_noop_live_targets_apply();
    test_f20_sibling_lastopid_mismatch_skips_whole();
    test_empty_undo_ring_is_noop();
    test_pure_noop_undo_consumes_no_redo();
    test_matching_undo_records_counterpart();
    test_matching_redo_records_counterpart();
    test_multi_inverse_undo_records_one_compound();
    test_lastopid_unwind_allows_older_own_gestures();
    test_lastopid_f20_still_skips_when_later_live();
    test_lastopid_redo_restamps_forward();
    test_lastopid_redo_move_b_then_create_b_skips();

    if (g_fails) {
        std::cerr << g_fails << " check(s) failed\n";
        return 1;
    }
    std::cout << "device_document_test: checks passed\n";
    return 0;
}
