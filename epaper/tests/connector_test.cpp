/**
 * STORY-EP-030 / @SRS-EP-17 connector recognition.
 */
#include "document/device_document.hpp"
#include "document/recognizer_dispatch.hpp"
#include "document/connector_warp.hpp"

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

static std::vector<InkSample> lineX(double x0, double x1, double y, int n)
{
    return lineXY(x0, y, x1, y, n);
}

static JsonValue inkChildToJsonLocal(const std::string &id, const std::string &role,
                                     const std::vector<InkSample> &samples)
{
    JsonValue::Object o;
    o.emplace_back("id", JsonValue::string(id));
    o.emplace_back("kind", JsonValue::string("ink"));
    o.emplace_back("role", JsonValue::string(role));
    JsonValue::Object style;
    style.emplace_back("stroke", JsonValue::string("#1C2430"));
    style.emplace_back("strokeWidth", JsonValue::number(2));
    o.emplace_back("style", JsonValue::object(std::move(style)));
    o.emplace_back("samples", inkSamplesToJson(samples));
    return JsonValue::object(std::move(o));
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
        CHECK(doc.commitJson(opEnvelope("append_ink:" + id, "append_ink", JsonValue::object(std::move(payload)))).applied);
}

static void addSg(DeviceDocument &doc, const std::string &id, double x, double y, double w, double h,
                  const std::vector<InkSample> *boundaryLocal = nullptr)
{
    std::vector<InkSample> bpoly;
    if (boundaryLocal && boundaryLocal->size() >= 2) {
        bpoly = *boundaryLocal;
    } else {
        const double xs[5] = {0, w, w, 0, 0};
        const double ys[5] = {0, 0, h, h, 0};
        for (int i = 0; i < 5; ++i) {
            InkSample s;
            s.x = xs[i];
            s.y = ys[i];
            s.t = double(i);
            bpoly.push_back(s);
        }
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
    JsonValue kids = JsonValue::array({inkChildToJsonLocal(id + "_b", "boundary", bpoly)});
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    payload.emplace_back("transform", JsonValue::object(std::move(t)));
    payload.emplace_back("inkScaleMode", JsonValue::string("fixedInk"));
    payload.emplace_back("children", std::move(kids));
        CHECK(doc.commitJson(opEnvelope("create_smart_group:" + id, "create_smart_group", JsonValue::object(std::move(payload)))).applied);
}

static RecogDispatchResult penUp(DeviceDocument &doc, const std::string &id,
                                 const std::vector<InkSample> &samples, RecogLatch latch)
{
    EncloseStrokeInput stroke;
    stroke.id = id;
    stroke.samples = samples;
    return dispatchPenUp(doc, stroke, latch);
}

static void test_ux1_create()
{
    DeviceDocument doc;
    addSg(doc, "A", 0, 0, 80, 80);
    addSg(doc, "C", 300, 0, 80, 80);
    RecogLatch latch;
    const auto samples = lineX(78, 302, 40, 24);
    const RecogDispatchResult d = penUp(doc, "ink_ac", samples, latch);
    if (d.outcome != RecogOutcome::Connector)
        std::cerr << "ux1 outcome=" << d.outcomeName() << " guard=" << d.guard << "\n";
    CHECK(d.outcome == RecogOutcome::Connector);
    CHECK(d.guard == "none");
    CHECK(d.connector.kind == ConnectorKind::Created);
    CHECK(!d.connector.warpStyle.empty());
    const DocNode *conn = doc.find(d.connector.connectorId);
    CHECK(conn);
    if (!conn)
        return;
    CHECK(conn->kind == NodeKind::Connector);
    CHECK(conn->fromNodeId == "A");
    CHECK(conn->toNodeId == "C");
    CHECK(!conn->restSpine.empty());
    CHECK(conn->children.size() == 1);
    CHECK(doc.find("ink_ac") == &conn->children[0]);
    CHECK(doc.publishQueue().size() >= 1);
    CHECK(doc.undo().restored);
    CHECK(doc.find("ink_ac"));
    CHECK(!doc.find(d.connector.connectorId));
}

static void test_ux2_chain()
{
    DeviceDocument doc;
    addSg(doc, "A", 0, 0, 80, 80);
    addSg(doc, "C", 300, 0, 80, 80);
    appendInk(doc, "s1", lineX(78, 140, 40, 8));
    appendInk(doc, "s2", lineX(140, 220, 40, 8));
    RecogLatch latch;
    const RecogDispatchResult d = penUp(doc, "s3", lineX(220, 302, 40, 8), latch);
    CHECK(d.outcome == RecogOutcome::Connector);
    CHECK(d.connector.bodyIds.size() == 3);
    const DocNode *conn = doc.find(d.connector.connectorId);
    CHECK(conn);
    CHECK(conn->children.size() == 3);
    CHECK(conn->warpStyle == "cubic" || conn->warpStyle == "morph");
    CHECK(doc.undo().restored);
    CHECK(doc.find("s1") && doc.find("s2") && doc.find("s3"));
    CHECK(!doc.find(d.connector.connectorId));
}

static void test_toggle_off_and_guard()
{
    DeviceDocument doc;
    addSg(doc, "A", 0, 0, 80, 80);
    addSg(doc, "C", 300, 0, 80, 80);
    RecogLatch off;
    off.connector = false;
    const RecogDispatchResult d0 =
        penUp(doc, "stay0", lineX(78, 302, 40, 24), off);
    CHECK(d0.outcome == RecogOutcome::Ink);
    CHECK(doc.find("stay0"));
    CHECK(d0.connector.kind == ConnectorKind::None);

    RecogLatch on;
    const RecogDispatchResult d1 = penUp(doc, "miss", lineX(0, 200, 400, 20), on);
    CHECK(d1.outcome == RecogOutcome::Ink);
    CHECK(doc.find("miss"));
}

static void test_short_tips_inside()
{
    DeviceDocument doc;
    addSg(doc, "A", 0, 0, 80, 80);
    addSg(doc, "C", 100, 0, 80, 80);
    RecogLatch latch;
    const RecogDispatchResult d = penUp(doc, "tips", lineX(50, 130, 40, 16), latch);
    if (d.outcome != RecogOutcome::Connector)
        std::cerr << "tips outcome=" << d.outcomeName() << " guard=" << d.guard << "\n";
    CHECK(d.outcome == RecogOutcome::Connector);
}

static void test_diagonal_and_wiggle()
{
    DeviceDocument doc;
    addSg(doc, "A", 0, 0, 80, 80);
    addSg(doc, "C", 300, 220, 80, 80);
    RecogLatch latch;
    std::vector<InkSample> diag;
    for (int i = 0; i < 20; ++i) {
        InkSample s;
        const double t = double(i) / 19.0;
        s.x = 78 + t * (302 - 78);
        s.y = 40 + t * (260 - 40);
        s.t = double(i);
        diag.push_back(s);
    }
    const RecogDispatchResult d0 = penUp(doc, "diag", diag, latch);
    if (d0.outcome != RecogOutcome::Connector)
        std::cerr << "diag outcome=" << d0.outcomeName() << " guard=" << d0.guard << "\n";
    CHECK(d0.outcome == RecogOutcome::Connector);

    DeviceDocument doc2;
    addSg(doc2, "A", 0, 0, 80, 80);
    addSg(doc2, "C", 300, 0, 80, 80);
    std::vector<InkSample> wig;
    for (int i = 0; i < 40; ++i) {
        InkSample s;
        const double t = double(i) / 39.0;
        s.x = 78 + t * (302 - 78);
        s.y = 40 + 28.0 * std::sin(t * 6.28318530718 * 2.0);
        s.t = double(i);
        wig.push_back(s);
    }
    const RecogDispatchResult d1 = penUp(doc2, "wig", wig, latch);
    if (d1.outcome != RecogOutcome::Connector)
        std::cerr << "wiggle outcome=" << d1.outcomeName() << " guard=" << d1.guard << "\n";
    CHECK(d1.outcome == RecogOutcome::Connector);
}

static std::vector<InkSample> diamondLocal(double w, double h)
{
    const double xs[5] = {w * 0.5, w, w * 0.5, 0, w * 0.5};
    const double ys[5] = {0, h * 0.5, h, h * 0.5, 0};
    std::vector<InkSample> out;
    for (int i = 0; i < 5; ++i) {
        InkSample s;
        s.x = xs[i];
        s.y = ys[i];
        s.t = double(i);
        out.push_back(s);
    }
    return out;
}

static void test_snap_uses_boundary_not_aabb()
{
    DeviceDocument doc;
    auto dia = diamondLocal(80, 80);
    addSg(doc, "A", 0, 0, 80, 80, &dia);
    addSg(doc, "C", 300, 0, 80, 80);
    RecogLatch latch;
    // AABB corner is inside the fitted rect but outside the diamond and > R_SNAP from it.
    const RecogDispatchResult miss = penUp(doc, "corner", lineX(2, 340, 2, 24), latch);
    CHECK(miss.outcome == RecogOutcome::Ink);
    DeviceDocument doc2;
    addSg(doc2, "A", 0, 0, 80, 80, &dia);
    addSg(doc2, "C", 300, 0, 80, 80);
    const RecogDispatchResult hit = penUp(doc2, "tip", lineX(40, 340, 3, 24), latch);
    if (hit.outcome != RecogOutcome::Connector)
        std::cerr << "diamond-tip outcome=" << hit.outcomeName() << " guard=" << hit.guard << "\n";
    CHECK(hit.outcome == RecogOutcome::Connector);
}

static void test_last_end_prefers_other_group()
{
    DeviceDocument doc;
    addSg(doc, "A", 0, 0, 80, 80);
    addSg(doc, "C", 120, 0, 80, 80);
    RecogLatch latch;
    // Last is in the gap, equally near both; must bind C not A.
    const RecogDispatchResult d = penUp(doc, "gap", lineX(40, 100, 40, 20), latch);
    if (d.outcome != RecogOutcome::Connector)
        std::cerr << "gap outcome=" << d.outcomeName() << " guard=" << d.guard << "\n";
    CHECK(d.outcome == RecogOutcome::Connector);
    CHECK(d.connector.fromId == "A");
    CHECK(d.connector.toId == "C");
}

static void test_ux2_gap_between_strokes()
{
    DeviceDocument doc;
    addSg(doc, "A", 0, 0, 80, 80);
    addSg(doc, "C", 220, 0, 80, 80);
    appendInk(doc, "s1", lineX(40, 140, 40, 10));
    RecogLatch latch;
    const RecogDispatchResult d = penUp(doc, "s2", lineX(144, 260, 40, 10), latch);
    if (d.outcome != RecogOutcome::Connector)
        std::cerr << "ux2-gap outcome=" << d.outcomeName() << " guard=" << d.guard << "\n";
    CHECK(d.outcome == RecogOutcome::Connector);
    CHECK(d.connector.bodyIds.size() == 2);
}

static void test_ux2_any_order_and_z()
{
    DeviceDocument doc;
    addSg(doc, "A", 0, 0, 80, 80);
    addSg(doc, "C", 300, 0, 80, 80);
    appendInk(doc, "nearC", lineXY(302, 40, 200, 40, 8));
    RecogLatch latch;
    const RecogDispatchResult d = penUp(doc, "nearA", lineXY(78, 40, 196, 40, 8), latch);
    if (d.outcome != RecogOutcome::Connector)
        std::cerr << "any-order outcome=" << d.outcomeName() << " guard=" << d.guard << "\n";
    CHECK(d.outcome == RecogOutcome::Connector);
    CHECK(d.connector.bodyIds.size() == 2);

    DeviceDocument z;
    addSg(z, "A", 0, 0, 80, 80);
    addSg(z, "C", 200, 100, 80, 80);
    appendInk(z, "z1", lineXY(70, 40, 140, 40, 8));
    appendInk(z, "z3", lineXY(220, 140, 140, 140, 8));
    const RecogDispatchResult dz = penUp(z, "z2", lineXY(140, 40, 140, 140, 8), latch);
    if (dz.outcome != RecogOutcome::Connector)
        std::cerr << "z outcome=" << dz.outcomeName() << " guard=" << dz.guard << "\n";
    CHECK(dz.outcome == RecogOutcome::Connector);
    CHECK(dz.connector.bodyIds.size() == 3);
}

static void test_ux2_crossing_splice()
{
    DeviceDocument doc;
    addSg(doc, "A", 0, 0, 80, 80);
    addSg(doc, "C", 100, 200, 80, 80);
    appendInk(doc, "h", lineXY(70, 40, 200, 40, 10));
    RecogLatch latch;
    const RecogDispatchResult d = penUp(doc, "v", lineXY(140, 10, 140, 240, 12), latch);
    if (d.outcome != RecogOutcome::Connector)
        std::cerr << "cross outcome=" << d.outcomeName() << " guard=" << d.guard << "\n";
    CHECK(d.outcome == RecogOutcome::Connector);
    CHECK(d.connector.bodyIds.size() == 2);
    const DocNode *conn = doc.find(d.connector.connectorId);
    CHECK(conn);
    if (!conn)
        return;
    bool sawJoin = false;
    for (const auto &p : conn->restSpine) {
        if (std::hypot(p.x - 140, p.y - 40) < 4)
            sawJoin = true;
    }
    CHECK(sawJoin);
}

static void test_consecutive_stops_at_box()
{
    DeviceDocument doc;
    appendInk(doc, "old", lineX(40, 140, 40, 8));
    addSg(doc, "A", 0, 0, 80, 80);
    addSg(doc, "C", 220, 0, 80, 80);
    appendInk(doc, "s1", lineX(70, 140, 40, 8));
    RecogLatch latch;
    const RecogDispatchResult d = penUp(doc, "s2", lineX(144, 260, 40, 8), latch);
    if (d.outcome != RecogOutcome::Connector)
        std::cerr << "consec outcome=" << d.outcomeName() << " guard=" << d.guard
                  << " " << d.connector.diag << "\n";
    CHECK(d.outcome == RecogOutcome::Connector);
    CHECK(d.connector.bodyIds.size() == 2);
    bool hasOld = false;
    for (const auto &id : d.connector.bodyIds)
        if (id == "old")
            hasOld = true;
    CHECK(!hasOld);
    CHECK(d.connector.diag.find("stop=sg:") != std::string::npos);
}

/** Infini reconnect snapshot uses restSpine, not restShape — paint needs warp spine. */
static void test_infini_snapshot_rest_spine_warps()
{
    DeviceDocument doc;
    doc.onAcceptedDocLoad(parseJson(R"({
      "version": 1,
      "status": "open",
      "rootChildren": [
        {
          "id": "A",
          "kind": "smart_group",
          "bounds": { "x": 0, "y": 0, "width": 80, "height": 80 },
          "transform": { "x": 0, "y": 0, "rotation": 0, "scaleX": 1, "scaleY": 1 },
          "inkScaleMode": "fixedInk",
          "children": []
        },
        {
          "id": "B",
          "kind": "smart_group",
          "bounds": { "x": 0, "y": 0, "width": 80, "height": 80 },
          "transform": { "x": 200, "y": 0, "rotation": 0, "scaleX": 1, "scaleY": 1 },
          "inkScaleMode": "fixedInk",
          "children": []
        },
        {
          "id": "c1",
          "kind": "connector",
          "from": { "nodeId": "A", "kind": "centre" },
          "to": { "nodeId": "B", "kind": "centre" },
          "warpStyle": "morph",
          "restSpine": [
            { "x": 40, "y": 40 },
            { "x": 120, "y": 40 },
            { "x": 240, "y": 40 }
          ],
          "restOffsets": [
            { "s": 0, "d": 0 },
            { "s": 0.5, "d": 0 },
            { "s": 1, "d": 0 }
          ],
          "children": []
        }
      ]
    })"));
    const DocNode *c = doc.find("c1");
    CHECK(c);
    CHECK(c && c->kind == NodeKind::Connector);
    CHECK(c && c->restSpine.size() >= 2);
    refreshAllConnectorWarps(doc);
    c = doc.find("c1");
    CHECK(c && c->warpedSamples.size() >= 2);
}

int main()
{
    test_ux1_create();
    test_ux2_chain();
    test_toggle_off_and_guard();
    test_short_tips_inside();
    test_diagonal_and_wiggle();
    test_snap_uses_boundary_not_aabb();
    test_last_end_prefers_other_group();
    test_ux2_gap_between_strokes();
    test_ux2_any_order_and_z();
    test_ux2_crossing_splice();
    test_consecutive_stops_at_box();
    test_infini_snapshot_rest_spine_warps();
    if (g_fails) {
        std::cerr << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "connector_test: OK\n";
    return 0;
}
