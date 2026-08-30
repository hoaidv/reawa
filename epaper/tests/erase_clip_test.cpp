/**
 * STORY-EP-063 / STORY-EP-064 — geometric clip, remnant split, brush capsule.
 * Host test, no Qt / HID.
 * @implements [SRS-EP-55] clip remnants
 * @implements [SRS-EP-56] brush capsule
 */
#include "document/device_document.hpp"
#include "document/erase_clip.hpp"
#include "document/erase_commit.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace epaper::document;

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

static JsonValue makeInk(const std::string &opId, const std::string &id,
                         const std::vector<std::pair<double, double>> &xy,
                         const std::string &parent = {}, const std::string &role = {})
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
    if (!parent.empty())
        payload.emplace_back("parentId", JsonValue::string(parent));
    payload.emplace_back("samples", JsonValue::array(std::move(samples)));
    payload.emplace_back("style", parseJson(R"({"stroke":"#1C2430","strokeWidth":2})"));
    if (!role.empty())
        payload.emplace_back("role", JsonValue::string(role));
    return opEnvelope(opId, "append_ink", JsonValue::object(std::move(payload)));
}

static JsonValue makePrimitive(const std::string &opId, const std::string &id)
{
    JsonValue::Object geom;
    geom.emplace_back("kind", JsonValue::string("rect"));
    geom.emplace_back("x", JsonValue::number(0));
    geom.emplace_back("y", JsonValue::number(0));
    geom.emplace_back("w", JsonValue::number(10));
    geom.emplace_back("h", JsonValue::number(10));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("geom", JsonValue::object(std::move(geom)));
    payload.emplace_back("style", parseJson(R"({"stroke":"#1C2430","strokeWidth":2})"));
    return opEnvelope(opId, "create_primitive", JsonValue::object(std::move(payload)));
}

static JsonValue makeFrame(const std::string &opId, const std::string &id)
{
    JsonValue::Object b;
    b.emplace_back("minX", JsonValue::number(0));
    b.emplace_back("minY", JsonValue::number(0));
    b.emplace_back("maxX", JsonValue::number(40));
    b.emplace_back("maxY", JsonValue::number(40));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    return opEnvelope(opId, "create_frame", JsonValue::object(std::move(payload)));
}

static JsonValue makeConnector(const std::string &opId, const std::string &id,
                               const std::string &fromId, const std::string &toId)
{
    JsonValue::Object from;
    from.emplace_back("nodeId", JsonValue::string(fromId));
    JsonValue::Object to;
    to.emplace_back("nodeId", JsonValue::string(toId));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("from", JsonValue::object(std::move(from)));
    payload.emplace_back("to", JsonValue::object(std::move(to)));
    return opEnvelope(opId, "create_connector", JsonValue::object(std::move(payload)));
}

static bool remnantOutside(const DocNode *n, const ClipRegion &region)
{
    if (!n || n->samples.size() < 2)
        return true;
    for (size_t i = 1; i < n->samples.size(); ++i) {
        const InkSample &a = n->samples[i - 1];
        const InkSample &b = n->samples[i];
        for (int k = 1; k < 8; ++k) {
            const double t = k / 8.0;
            const double x = a.x + (b.x - a.x) * t;
            const double y = a.y + (b.y - a.y) * t;
            if (clipRegionContains(region, x, y))
                return false;
        }
    }
    return true;
}

static void test_split_two_remnants_longest_keeps_id()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeInk("ink-1", "I1", {{0, 0}, {100, 0}})).applied);
    const ClipRegion cap = capsuleRegion({{50, 0}}, 4.0);
    CHECK(commitEraseRegion(doc, "erase-1", cap).applied);
    const DocNode *keep = doc.find("I1");
    CHECK(keep && keep->kind == NodeKind::Ink);
    CHECK(doc.inkCount() == 2);
    CHECK(remnantOutside(keep, cap));
    const DocNode *extra = nullptr;
    doc.forEachPaintNode([&](const DocNode &n) {
        if (n.kind == NodeKind::Ink && n.id != "I1")
            extra = &n;
    });
    CHECK(extra);
    CHECK(remnantOutside(extra, cap));
    CHECK(polylineArcLength(keep->samples) + 1e-6 >= polylineArcLength(extra->samples));
    DeviceDocument::NodePlace a, b;
    CHECK(doc.findPlace("I1", &a));
    CHECK(doc.findPlace(extra->id, &b));
    CHECK(a.parentId == b.parentId);
    CHECK(b.index == a.index + 1);
    CHECK(doc.publishQueue().back().op.getString("type") == "compound");
    CHECK(doc.restoreSnapshotQueued() == 0);
    CHECK(doc.undo().restored);
    CHECK(doc.find("I1"));
    CHECK(doc.inkCount() == 1);
}

static void test_remnant_below_floor_dropped()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeInk("ink-1", "I1", {{0, 0}, {10, 0}})).applied);
    const ClipRegion cap = capsuleRegion({{5, 0}}, 4.6);
    CHECK(commitEraseRegion(doc, "erase-floor", cap).applied);
    CHECK(!doc.find("I1"));
    CHECK(doc.inkCount() == 0);
}

static void test_empty_removes_ink()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeInk("ink-1", "I1", {{0, 0}, {4, 0}})).applied);
    const ClipRegion cap = capsuleRegion({{2, 0}}, 8.0);
    CHECK(commitEraseRegion(doc, "erase-empty", cap).applied);
    CHECK(!doc.find("I1"));
    const std::string t = doc.publishQueue().back().op.getString("type");
    CHECK(t == "remove_node" || t == "compound");
    CHECK(doc.restoreSnapshotQueued() == 0);
}

static void test_miss_is_noop()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeInk("ink-1", "I1", {{0, 0}, {10, 0}})).applied);
    const std::size_t q0 = doc.publishQueue().size();
    const ClipRegion cap = capsuleRegion({{50, 50}}, 4.0);
    const ApplyResult r = commitEraseRegion(doc, "erase-miss", cap);
    CHECK(r.applied);
    CHECK(r.reason == "noop");
    CHECK(doc.publishQueue().size() == q0);
    CHECK(doc.find("I1") && doc.find("I1")->samples.size() == 2);
}

static void test_smartgroup_last_ink_removes_group()
{
    DeviceDocument doc;
    JsonValue::Object b;
    b.emplace_back("x", JsonValue::number(0));
    b.emplace_back("y", JsonValue::number(0));
    b.emplace_back("width", JsonValue::number(40));
    b.emplace_back("height", JsonValue::number(40));
    JsonValue::Object xf;
    xf.emplace_back("x", JsonValue::number(0));
    xf.emplace_back("y", JsonValue::number(0));
    xf.emplace_back("rotation", JsonValue::number(0));
    xf.emplace_back("scaleX", JsonValue::number(1));
    xf.emplace_back("scaleY", JsonValue::number(1));
    JsonValue::Object child;
    child.emplace_back("id", JsonValue::string("B1"));
    child.emplace_back("kind", JsonValue::string("ink"));
    child.emplace_back("role", JsonValue::string("boundary"));
    JsonValue::Array samples;
    JsonValue::Object s0, s1, s2, s3;
    s0.emplace_back("x", JsonValue::number(0));
    s0.emplace_back("y", JsonValue::number(0));
    s1.emplace_back("x", JsonValue::number(20));
    s1.emplace_back("y", JsonValue::number(0));
    s2.emplace_back("x", JsonValue::number(20));
    s2.emplace_back("y", JsonValue::number(20));
    s3.emplace_back("x", JsonValue::number(0));
    s3.emplace_back("y", JsonValue::number(0));
    samples.push_back(JsonValue::object(std::move(s0)));
    samples.push_back(JsonValue::object(std::move(s1)));
    samples.push_back(JsonValue::object(std::move(s2)));
    samples.push_back(JsonValue::object(std::move(s3)));
    child.emplace_back("samples", JsonValue::array(std::move(samples)));
    child.emplace_back("style", parseJson(R"({"stroke":"#1C2430","strokeWidth":2})"));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string("SG1"));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    payload.emplace_back("transform", JsonValue::object(std::move(xf)));
    payload.emplace_back("inkScaleMode", JsonValue::string("fixedInk"));
    payload.emplace_back("children", JsonValue::array({JsonValue::object(std::move(child))}));
    JsonValue::Array bp;
    JsonValue::Object p0, p1;
    p0.emplace_back("x", JsonValue::number(0));
    p0.emplace_back("y", JsonValue::number(0));
    p1.emplace_back("x", JsonValue::number(20));
    p1.emplace_back("y", JsonValue::number(0));
    bp.push_back(JsonValue::object(std::move(p0)));
    bp.push_back(JsonValue::object(std::move(p1)));
    payload.emplace_back("boundaryPolyline", JsonValue::array(std::move(bp)));
    CHECK(doc.commitJson(opEnvelope("sg-1", "create_smart_group", JsonValue::object(std::move(payload))))
              .applied);
    const DocNode *sg0 = doc.find("SG1");
    CHECK(sg0 && sg0->boundaryPolyline.size() >= 2);
    const size_t polyN = sg0->boundaryPolyline.size();
    const ClipRegion cap = capsuleRegion({{10, 0}}, 80.0);
    CHECK(commitEraseRegion(doc, "erase-sg", cap).applied);
    CHECK(!doc.find("B1"));
    CHECK(!doc.find("SG1"));
    (void)polyN;
}

static void test_boundary_polyline_unchanged_when_boundary_ink_clipped()
{
    DeviceDocument doc;
    JsonValue::Object b;
    b.emplace_back("x", JsonValue::number(0));
    b.emplace_back("y", JsonValue::number(0));
    b.emplace_back("width", JsonValue::number(100));
    b.emplace_back("height", JsonValue::number(40));
    JsonValue::Object xf;
    xf.emplace_back("x", JsonValue::number(0));
    xf.emplace_back("y", JsonValue::number(0));
    xf.emplace_back("rotation", JsonValue::number(0));
    xf.emplace_back("scaleX", JsonValue::number(1));
    xf.emplace_back("scaleY", JsonValue::number(1));
    JsonValue::Object child;
    child.emplace_back("id", JsonValue::string("B1"));
    child.emplace_back("kind", JsonValue::string("ink"));
    child.emplace_back("role", JsonValue::string("boundary"));
    JsonValue::Array samples;
    JsonValue::Object a, z;
    a.emplace_back("x", JsonValue::number(0));
    a.emplace_back("y", JsonValue::number(0));
    z.emplace_back("x", JsonValue::number(100));
    z.emplace_back("y", JsonValue::number(0));
    samples.push_back(JsonValue::object(std::move(a)));
    samples.push_back(JsonValue::object(std::move(z)));
    child.emplace_back("samples", JsonValue::array(std::move(samples)));
    child.emplace_back("style", parseJson(R"({"stroke":"#1C2430","strokeWidth":2})"));
    JsonValue::Array bp;
    JsonValue::Object p0, p1, p2;
    p0.emplace_back("x", JsonValue::number(0));
    p0.emplace_back("y", JsonValue::number(0));
    p1.emplace_back("x", JsonValue::number(100));
    p1.emplace_back("y", JsonValue::number(0));
    p2.emplace_back("x", JsonValue::number(0));
    p2.emplace_back("y", JsonValue::number(0));
    bp.push_back(JsonValue::object(std::move(p0)));
    bp.push_back(JsonValue::object(std::move(p1)));
    bp.push_back(JsonValue::object(std::move(p2)));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string("SG1"));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    payload.emplace_back("transform", JsonValue::object(std::move(xf)));
    payload.emplace_back("inkScaleMode", JsonValue::string("fixedInk"));
    payload.emplace_back("children", JsonValue::array({JsonValue::object(std::move(child))}));
    payload.emplace_back("boundaryPolyline", JsonValue::array(std::move(bp)));
    CHECK(doc.commitJson(opEnvelope("sg-1", "create_smart_group", JsonValue::object(std::move(payload))))
              .applied);
    const DocNode *sg0 = doc.find("SG1");
    CHECK(sg0 && sg0->boundaryPolyline.size() == 3);
    const ClipRegion cap = capsuleRegion({{50, 0}}, 4.0);
    CHECK(commitEraseRegion(doc, "erase-b", cap).applied);
    const DocNode *sg1 = doc.find("SG1");
    CHECK(sg1 && sg1->boundaryPolyline.size() == 3);
    CHECK(std::abs(sg1->boundaryPolyline[1].x - 100) < 1e-9);
    CHECK(doc.find("B1"));
    CHECK(doc.inkCount() >= 2);
}

static void test_json_roundtrip_boundary_polyline()
{
    DeviceDocument doc;
    JsonValue::Object b;
    b.emplace_back("x", JsonValue::number(0));
    b.emplace_back("y", JsonValue::number(0));
    b.emplace_back("width", JsonValue::number(10));
    b.emplace_back("height", JsonValue::number(10));
    JsonValue::Object xf;
    xf.emplace_back("x", JsonValue::number(1));
    xf.emplace_back("y", JsonValue::number(2));
    xf.emplace_back("rotation", JsonValue::number(0));
    xf.emplace_back("scaleX", JsonValue::number(1));
    xf.emplace_back("scaleY", JsonValue::number(1));
    JsonValue::Array bp;
    JsonValue::Object p0;
    p0.emplace_back("x", JsonValue::number(3));
    p0.emplace_back("y", JsonValue::number(4));
    bp.push_back(JsonValue::object(std::move(p0)));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string("SG1"));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    payload.emplace_back("transform", JsonValue::object(std::move(xf)));
    payload.emplace_back("inkScaleMode", JsonValue::string("fixedInk"));
    payload.emplace_back("children", JsonValue::array({}));
    payload.emplace_back("boundaryPolyline", JsonValue::array(std::move(bp)));
    CHECK(doc.commitJson(opEnvelope("sg-1", "create_smart_group", JsonValue::object(std::move(payload))))
              .applied);
    const std::string snap = doc.snapshotString();
    DeviceDocument doc2;
    const JsonValue parsed = parseJson(snap);
    const JsonValue *kids = parsed.get("rootChildren");
    CHECK(kids && kids->isArray() && !kids->asArray().empty());
    const DocNode n = DeviceDocument::nodeFromJson(kids->asArray()[0]);
    CHECK(n.kind == NodeKind::SmartGroup);
    CHECK(n.boundaryPolyline.size() == 1);
    CHECK(std::abs(n.boundaryPolyline[0].x - 3) < 1e-9);
}

static void test_brush_skips_primitive_frame_connector()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeInk("ink-1", "I1", {{0, 0}, {80, 0}})).applied);
    CHECK(doc.commitJson(makePrimitive("prim-1", "P1")).applied);
    CHECK(doc.commitJson(makeFrame("fr-1", "F1")).applied);
    CHECK(doc.commitJson(makeConnector("c-1", "C1", "P1", "F1")).applied);
    const ClipRegion cap = capsuleRegion({{40, 0}}, 4.0);
    CHECK(commitEraseRegion(doc, "erase-kinds", cap).applied);
    CHECK(doc.find("P1") && doc.find("P1")->kind == NodeKind::Primitive);
    CHECK(doc.find("F1") && doc.find("F1")->kind == NodeKind::Frame);
    CHECK(doc.find("C1") && doc.find("C1")->kind == NodeKind::Connector);
    CHECK(doc.find("I1"));
}

static void test_clip_geometry_not_sample_drop()
{
    std::vector<InkSample> s(2);
    s[0].x = 0;
    s[0].y = 0;
    s[1].x = 100;
    s[1].y = 0;
    const ClipResult r = clipInkPolyline(s, capsuleRegion({{50, 0}}, 4.0));
    CHECK(r.hit);
    CHECK(r.remnants.size() == 2);
    CHECK(r.remnants[0].back().x < 50);
    CHECK(r.remnants[1].front().x > 50);
    CHECK(std::abs(r.remnants[0].back().x - 46) < 1.0);
}

static void test_second_split_skips_taken_remnant_id()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeInk("ink-1", "s-3", {{0, 0}, {200, 0}})).applied);
    CHECK(doc.commitJson(makeInk("ink-r1", "s-3_r1", {{0, 10}, {10, 10}})).applied);
    CHECK(commitEraseRegion(doc, "erase-1", capsuleRegion({{50, 0}}, 4.0)).applied);
    CHECK(doc.find("s-3"));
    CHECK(doc.find("s-3_r1"));
    CHECK(doc.inkCount() == 3);
    const ApplyResult r = commitEraseRegion(doc, "erase-2", capsuleRegion({{150, 0}}, 4.0));
    CHECK(r.applied);
    CHECK(r.reason.find("duplicate_id") == std::string::npos);
    CHECK(doc.find("s-3"));
    CHECK(doc.find("s-3_r1"));
    CHECK(doc.inkCount() == 4);
    std::vector<std::string> extras;
    doc.forEachPaintNode([&](const DocNode &n) {
        if (n.kind == NodeKind::Ink && n.id != "s-3" && n.id != "s-3_r1")
            extras.push_back(n.id);
    });
    CHECK(extras.size() == 2);
    CHECK(extras[0] != extras[1]);
}

static void test_generate_node_id_skips_tree_and_reserved()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeInk("ink-n1", "n-1", {{0, 0}, {1, 0}})).applied);
    const std::string a = doc.generateNodeId();
    const std::string b = doc.generateNodeId();
    CHECK(a != "n-1");
    CHECK(b != "n-1");
    CHECK(a != b);
    CHECK(!doc.find(a) && !doc.find(b));
    CHECK(doc.commitJson(makeInk("ink-a", a, {{2, 0}, {3, 0}})).applied);
    CHECK(doc.find(a));
    const ApplyResult dup = doc.commitJson(makeInk("ink-dup", a, {{4, 0}, {5, 0}}));
    CHECK(!dup.applied);
    CHECK(dup.reason.find("duplicate_id") != std::string::npos);
}

static void test_two_callers_one_gesture_distinct_ids()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(makeInk("ink-1", "I1", {{0, 0}, {100, 0}})).applied);
    const std::string remnant = doc.generateNodeId();
    const std::string conn = doc.generateNodeId();
    CHECK(remnant != conn);
    CHECK(remnant != "I1");
    CHECK(conn != "I1");
    CHECK(doc.commitJson(makeInk("ink-r", remnant, {{0, 1}, {1, 1}})).applied);
    CHECK(doc.commitJson(makeInk("ink-c", conn, {{0, 2}, {1, 2}})).applied);
    CHECK(doc.find(remnant) && doc.find(conn) && doc.find("I1"));
}

static void test_mm_to_world_is_226dpi_du()
{
    const double eight = eraseMmToWorld(8.0);
    CHECK(eight > 70.0 && eight < 72.0);
    CHECK(std::abs(eight - 8.0 / 25.4 * epaper::handtouch::kPanelDpi) < 1e-9);
    CHECK(std::abs(eraseMmToWorld(1.0) - epaper::handtouch::kPanelDpi / 25.4) < 1e-9);
}

static void bench_clip_hotspot()
{
    std::vector<InkSample> ink(200);
    for (size_t i = 0; i < ink.size(); ++i) {
        ink[i].x = static_cast<double>(i) * 2.0;
        ink[i].y = 0;
    }
    std::vector<ErasePt> dense;
    dense.reserve(400);
    for (int i = 0; i < 400; ++i)
        dense.push_back({200.0 + i * 0.4, 0});
    const auto t0 = std::chrono::steady_clock::now();
    int hits = 0;
    for (int n = 0; n < 40; ++n) {
        const ClipResult r = clipInkPolyline(ink, capsuleRegion(dense, 36.0));
        if (r.hit)
            ++hits;
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();
    const auto resampled = capsuleRegion(dense, 36.0);
    std::cout << "erase_clip_bench: loops=40 inksamples=200 densePath=400 resampled="
              << resampled.path.size() << " hits=" << hits << " total_ms=" << ms
              << " per_ms=" << (ms / 40.0) << "\n";
    CHECK(resampled.path.size() < dense.size());
    CHECK(ms / 40.0 < 50.0);
}

int main()
{
    test_mm_to_world_is_226dpi_du();
    test_clip_geometry_not_sample_drop();
    test_second_split_skips_taken_remnant_id();
    test_generate_node_id_skips_tree_and_reserved();
    test_two_callers_one_gesture_distinct_ids();
    test_split_two_remnants_longest_keeps_id();
    test_remnant_below_floor_dropped();
    test_empty_removes_ink();
    test_miss_is_noop();
    test_smartgroup_last_ink_removes_group();
    test_boundary_polyline_unchanged_when_boundary_ink_clipped();
    test_json_roundtrip_boundary_polyline();
    test_brush_skips_primitive_frame_connector();
    bench_clip_hotspot();

    if (g_fails) {
        std::cerr << g_fails << " check(s) failed\n";
        return 1;
    }
    std::cout << "erase_clip_test: checks passed\n";
    return 0;
}
