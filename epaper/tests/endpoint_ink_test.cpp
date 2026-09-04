/**
 * STORY-EP-047 / @SRS-EP-35 Path B endpoint ink.
 */
#include "document/device_document.hpp"
#include "document/endpoint_ink.hpp"
#include "document/erase_commit.hpp"
#include "document/recognizer_dispatch.hpp"

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

static void addSg(DeviceDocument &doc, const std::string &id, double x, double y, double w, double h)
{
    std::vector<InkSample> bpoly;
    const double xs[5] = {0, w, w, 0, 0};
    const double ys[5] = {0, 0, h, h, 0};
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
    JsonValue kids = JsonValue::array({inkChildToJsonLocal(id + "_b", "boundary", bpoly)});
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    payload.emplace_back("transform", JsonValue::object(std::move(t)));
    payload.emplace_back("inkScaleMode", JsonValue::string("fixedInk"));
    payload.emplace_back("children", std::move(kids));
    CHECK(doc.commitJson(opEnvelope("create_smart_group:" + id, "create_smart_group",
                                    JsonValue::object(std::move(payload))))
              .applied);
}

static RecogDispatchResult penUp(DeviceDocument &doc, const std::string &id,
                                 const std::vector<InkSample> &samples, RecogLatch latch)
{
    EncloseStrokeInput stroke;
    stroke.id = id;
    stroke.samples = samples;
    return dispatchPenUp(doc, stroke, latch);
}

static std::string makeConnector(DeviceDocument &doc)
{
    addSg(doc, "A", 0, 0, 80, 80);
    addSg(doc, "C", 300, 0, 80, 80);
    RecogLatch latch;
    const RecogDispatchResult d = penUp(doc, "ink_ac", lineX(78, 302, 40, 24), latch);
    CHECK(d.outcome == RecogOutcome::Connector);
    return d.connector.connectorId;
}

static std::vector<InkSample> fromTick()
{
    return lineXY(82, 40, 100, 28, 8);
}

static const DocNode *requireConn(DeviceDocument &doc, const std::string &id)
{
    const DocNode *c = doc.find(id);
    CHECK(c && c->kind == NodeKind::Connector);
    return (c && c->kind == NodeKind::Connector) ? c : nullptr;
}

static void test_steal_binds_from_end()
{
    DeviceDocument doc;
    const std::string cid = makeConnector(doc);
    RecogLatch latch;
    const RecogDispatchResult d = penUp(doc, "tick1", fromTick(), latch);
    if (d.outcome != RecogOutcome::EndpointInk)
        std::cerr << "steal outcome=" << d.outcomeName() << " reason=" << d.endpointInk.reason
                  << "\n";
    CHECK(d.outcome == RecogOutcome::EndpointInk);
    CHECK(d.endpointInk.end == "from");
    CHECK(d.endpointInk.connectorId == cid);
    CHECK(!doc.find("tick1"));
    const DocNode *c = requireConn(doc, cid);
    if (!c)
        return;
    CHECK(c->fromAnchor.styleInk.size() == 1);
    CHECK(c->toAnchor.styleInk.empty());
    CHECK(c->fromAnchor.styleInk[0].pts.size() >= 2);
}

static void test_refuse_spine_empty_off()
{
    DeviceDocument doc;
    const std::string cid = makeConnector(doc);
    RecogLatch on;
    const RecogDispatchResult spine = penUp(doc, "spine", lineX(140, 220, 40, 12), on);
    CHECK(spine.outcome != RecogOutcome::EndpointInk);
    CHECK(doc.find("spine"));
    CHECK(requireConn(doc, cid) && requireConn(doc, cid)->fromAnchor.styleInk.empty());

    RecogLatch off;
    off.connector = false;
    const RecogDispatchResult disarmed = penUp(doc, "tick_off", fromTick(), off);
    CHECK(disarmed.outcome != RecogOutcome::EndpointInk);
    CHECK(doc.find("tick_off"));

    DeviceDocument empty;
    addSg(empty, "A", 0, 0, 80, 80);
    const RecogDispatchResult none = penUp(empty, "lonely", fromTick(), on);
    CHECK(none.outcome != RecogOutcome::EndpointInk);
    CHECK(empty.find("lonely"));
}

static void test_append_and_undo_peels_last()
{
    DeviceDocument doc;
    const std::string cid = makeConnector(doc);
    RecogLatch latch;
    CHECK(penUp(doc, "t1", fromTick(), latch).outcome == RecogOutcome::EndpointInk);
    CHECK(penUp(doc, "t2", lineXY(84, 38, 96, 22, 8), latch).outcome == RecogOutcome::EndpointInk);
    const DocNode *c = requireConn(doc, cid);
    CHECK(c && c->fromAnchor.styleInk.size() == 2);
    CHECK(doc.undo().restored);
    c = requireConn(doc, cid);
    CHECK(c && c->fromAnchor.styleInk.size() == 1);
    CHECK(doc.find("t2"));
    CHECK(!doc.find("t1"));
}

static void test_rotate_keeps_face_aim()
{
    DeviceDocument doc;
    const std::string cid = makeConnector(doc);
    RecogLatch latch;
    CHECK(penUp(doc, "tick1", fromTick(), latch).outcome == RecogOutcome::EndpointInk);
    const DocNode *c0 = requireConn(doc, cid);
    if (!c0 || c0->fromAnchor.styleInk.empty() || c0->fromAnchor.styleInk[0].pts.empty())
        return;
    const EndpointInkPt stored = c0->fromAnchor.styleInk[0].pts.front();
    const double drawnN0 = c0->fromAnchor.drawnN;
    const double drawnE0 = c0->fromAnchor.drawnE;
    const FaceFrame f0 = styleInkPaintFrame(doc, *c0, true);
    const RestVec w0 = faceToWorld(f0, stored);

    JsonValue::Object t;
    t.emplace_back("x", JsonValue::number(0));
    t.emplace_back("y", JsonValue::number(0));
    t.emplace_back("rotation", JsonValue::number(1.5707963267948966));
    t.emplace_back("scaleX", JsonValue::number(1));
    t.emplace_back("scaleY", JsonValue::number(1));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string("A"));
    payload.emplace_back("transform", JsonValue::object(std::move(t)));
    CHECK(doc.commitJson(opEnvelope("rot-A", "set_smart_transform", JsonValue::object(std::move(payload))))
              .applied);
    refreshAllConnectorWarps(doc);
    const DocNode *c1 = requireConn(doc, cid);
    if (!c1 || c1->fromAnchor.styleInk.empty())
        return;
    const EndpointInkPt after = c1->fromAnchor.styleInk[0].pts.front();
    CHECK(std::abs(after.n - stored.n) < 1e-9);
    CHECK(std::abs(after.e - stored.e) < 1e-9);
    CHECK(std::abs(c1->fromAnchor.drawnN - drawnN0) < 1e-9);
    CHECK(std::abs(c1->fromAnchor.drawnE - drawnE0) < 1e-9);
    const FaceFrame f1 = styleInkPaintFrame(doc, *c1, true);
    const RestVec w1 = faceToWorld(f1, after);
    CHECK(std::hypot(w1.x - w0.x, w1.y - w0.y) > 1.0);
    const EndpointInkPt back = worldToFace(f1, w1);
    CHECK(std::abs(back.n - stored.n) < 1e-6);
    CHECK(std::abs(back.e - stored.e) < 1e-6);
}

static RestVec warpedStyleChord(const DocNode &c)
{
    if (c.warpedStyleInk.empty() || c.warpedStyleInk[0].size() < 2)
        return {0, 0};
    const auto &a = c.warpedStyleInk[0].front();
    const auto &b = c.warpedStyleInk[0].back();
    return {b.x - a.x, b.y - a.y};
}

static void test_peer_move_rotates_style_ink_by_leave_delta()
{
    DeviceDocument doc;
    const std::string cid = makeConnector(doc);
    // Cubic Hermite pins end tangent to WarpEnd.f, so α stays tiny. Morph is
    // the warp where re-sample leave diverges from stored drawn leave.
    if (DocNode *forced = doc.mutableFind(cid)) {
        forced->warpStyle = "morph";
        refreshConnectorWarp(doc, *forced);
    }
    RecogLatch latch;
    CHECK(penUp(doc, "tick1", fromTick(), latch).outcome == RecogOutcome::EndpointInk);
    const DocNode *c0 = requireConn(doc, cid);
    if (!c0 || c0->fromAnchor.styleInk.size() != 1 || c0->fromAnchor.styleInk[0].pts.size() < 2)
        return;
    const auto storedStroke = c0->fromAnchor.styleInk[0];
    const double drawnN0 = c0->fromAnchor.drawnN;
    const double drawnE0 = c0->fromAnchor.drawnE;
    const double drawnBoxX0 = c0->fromAnchor.drawnBoxX;
    const double drawnBoxY0 = c0->fromAnchor.drawnBoxY;
    const RestVec chord0 = warpedStyleChord(*c0);
    WarpEnd e0a, e1a;
    CHECK(resolveConnectorEnds(doc, *c0, &e0a, &e1a));
    const RestVec leaveStored0 = e0a.f;
    const RestVec leaveWarp0 =
        warpedLeaveFromSamples(restVecsFromWarpedSamples(c0->warpedSamples), true);
    const double alpha0 = warpSignedDeg(leaveStored0, leaveWarp0);

    JsonValue::Object t;
    t.emplace_back("x", JsonValue::number(300));
    t.emplace_back("y", JsonValue::number(220));
    t.emplace_back("rotation", JsonValue::number(0));
    t.emplace_back("scaleX", JsonValue::number(1));
    t.emplace_back("scaleY", JsonValue::number(1));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string("C"));
    payload.emplace_back("transform", JsonValue::object(std::move(t)));
    CHECK(doc.commitJson(opEnvelope("move-C", "set_smart_transform", JsonValue::object(std::move(payload))))
              .applied);
    refreshAllConnectorWarps(doc);
    const DocNode *c1 = requireConn(doc, cid);
    if (!c1)
        return;
    CHECK(c1->fromAnchor.styleInk.size() == 1);
    CHECK(std::abs(c1->fromAnchor.styleInk[0].pts.front().n - storedStroke.pts.front().n) < 1e-9);
    CHECK(std::abs(c1->fromAnchor.styleInk[0].pts.front().e - storedStroke.pts.front().e) < 1e-9);
    CHECK(std::abs(c1->fromAnchor.drawnN - drawnN0) < 1e-9);
    CHECK(std::abs(c1->fromAnchor.drawnE - drawnE0) < 1e-9);
    CHECK(std::abs(c1->fromAnchor.drawnBoxX - drawnBoxX0) < 1e-9);
    CHECK(std::abs(c1->fromAnchor.drawnBoxY - drawnBoxY0) < 1e-9);

    WarpEnd e0b, e1b;
    CHECK(resolveConnectorEnds(doc, *c1, &e0b, &e1b));
    const RestVec leaveStored1 = e0b.f;
    const RestVec leaveWarp1 =
        warpedLeaveFromSamples(restVecsFromWarpedSamples(c1->warpedSamples), true);
    const double alpha1 = warpSignedDeg(leaveStored1, leaveWarp1);
    CHECK(std::abs(alpha1 - alpha0) > 8.0);

    const FaceFrame face = liveFaceFrame(doc, *c1, true);
    const RestVec unrot0 = faceToWorld(face, storedStroke.pts.front());
    const RestVec unrot1 = faceToWorld(face, storedStroke.pts.back());
    const RestVec unrotChord = {unrot1.x - unrot0.x, unrot1.y - unrot0.y};
    const RestVec expect = warpRot(unrotChord, alpha1 * kWarpPi / 180.0);
    const RestVec got = warpedStyleChord(*c1);
    CHECK(std::hypot(got.x - expect.x, got.y - expect.y) < 1e-4);
    CHECK(std::hypot(got.x - chord0.x, got.y - chord0.y) > 1.0);
}

static void test_brush_erase_ticks_keeps_connector()
{
    DeviceDocument doc;
    const std::string cid = makeConnector(doc);
    RecogLatch latch;
    CHECK(penUp(doc, "tick1", fromTick(), latch).outcome == RecogOutcome::EndpointInk);
    CHECK(requireConn(doc, cid)->fromAnchor.styleInk.size() == 1);
    const ClipRegion cap = capsuleRegion({{90, 34}}, 20.0);
    CHECK(commitEraseRegion(doc, "erase-ticks", cap).applied);
    const DocNode *c = requireConn(doc, cid);
    CHECK(c);
    CHECK(c->fromAnchor.styleInk.empty());
    CHECK(!c->restSpine.empty());
}

static std::vector<InkSample> boxLasso(double x0, double y0, double x1, double y1)
{
    auto pt = [](double x, double y) {
        InkSample s;
        s.x = x;
        s.y = y;
        return s;
    };
    return {pt(x0, y0), pt(x1, y0), pt(x1, y1), pt(x0, y1), pt(x0, y0)};
}

static void test_object_erase_ticks_keeps_connector()
{
    DeviceDocument doc;
    const std::string cid = makeConnector(doc);
    RecogLatch latch;
    CHECK(penUp(doc, "tick1", fromTick(), latch).outcome == RecogOutcome::EndpointInk);
    auto parts = planStyleInkObjectErase(doc, "obj-ticks", boxLasso(70, 18, 112, 52), {});
    CHECK(!parts.empty());
    CHECK(doc.commitGesture("obj-ticks", std::move(parts)).applied);
    const DocNode *c = requireConn(doc, cid);
    CHECK(c);
    CHECK(c->fromAnchor.styleInk.empty());
    CHECK(!c->restSpine.empty());
}

static void test_object_erase_connector_takes_decoration()
{
    DeviceDocument doc;
    const std::string cid = makeConnector(doc);
    RecogLatch latch;
    CHECK(penUp(doc, "tick1", fromTick(), latch).outcome == RecogOutcome::EndpointInk);
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(cid));
    CHECK(doc.commitJson(opEnvelope("rm-conn", "remove_node", JsonValue::object(std::move(payload))))
              .applied);
    CHECK(!doc.find(cid));
    CHECK(doc.undo().restored);
    const DocNode *c = requireConn(doc, cid);
    CHECK(c && c->fromAnchor.styleInk.size() == 1);
}

static void test_endpoint_beats_membership()
{
    DeviceDocument doc;
    const std::string cid = makeConnector(doc);
    RecogLatch latch;
    const RecogDispatchResult d = penUp(doc, "tick_in_box", lineXY(60, 40, 75, 38, 8), latch);
    CHECK(d.outcome == RecogOutcome::EndpointInk);
    CHECK(!doc.find("tick_in_box"));
    const DocNode *a = doc.find("A");
    CHECK(a);
    for (const auto &c : a->children)
        CHECK(c.id != "tick_in_box");
    const DocNode *conn = requireConn(doc, cid);
    CHECK(conn && !conn->fromAnchor.styleInk.empty());
}

int main()
{
    test_steal_binds_from_end();
    test_refuse_spine_empty_off();
    test_append_and_undo_peels_last();
    test_rotate_keeps_face_aim();
    test_peer_move_rotates_style_ink_by_leave_delta();
    test_brush_erase_ticks_keeps_connector();
    test_object_erase_ticks_keeps_connector();
    test_object_erase_connector_takes_decoration();
    test_endpoint_beats_membership();
    if (g_fails) {
        std::cerr << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "endpoint_ink_test: OK\n";
    return 0;
}
