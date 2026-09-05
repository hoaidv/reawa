/**
 * STORY-EP-031 / @SRS-EP-18 Morph + Cubic warp, I1/I3, D39 last-live pose.
 */
#include "document/connector_warp.hpp"
#include "document/device_document.hpp"
#include "document/recognizer_dispatch.hpp"
#include "document/surround_create.hpp"

#include <algorithm>
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

static void test_i3_morph_identity()
{
    const RestShape rs = buildRestShape({wiggleXY(0, 0, 400, 0, 80)});
    CHECK(rs.spine.size() >= 2);
    CHECK(rs.warpStyle == "morph");
    WarpEnd e0, e1;
    endsFromSpine(rs, &e0, &e1);
    const WarpResult w = warpConnector(rs, e0, e1, "morph");
    CHECK(!(w.mixM > 0.0));
    const auto rec = restShapeReconstruction(rs);
    CHECK(w.samples.size() == rec.size());
    for (size_t i = 0; i < rec.size(); ++i) {
        CHECK(w.samples[i].x == rec[i].x);
        CHECK(w.samples[i].y == rec[i].y);
    }
}

static void test_i1_never_rebake()
{
    const RestShape rs = buildRestShape({wiggleXY(0, 0, 400, 40, 80)});
    WarpEnd e0, e1;
    endsFromSpine(rs, &e0, &e1);
    const WarpResult first = warpConnector(rs, e0, e1, "morph");
    CHECK(!first.samples.empty());
    for (int i = 0; i < 200; ++i) {
        WarpEnd a = e0;
        WarpEnd b = e1;
        a.p.x += 12.0 * std::sin(i * 0.31);
        a.p.y += 9.0 * std::cos(i * 0.17);
        b.p.x += 7.0 * std::cos(i * 0.23);
        b.p.y += 11.0 * std::sin(i * 0.19);
        (void)warpConnector(rs, a, b, "morph");
    }
    const WarpResult last = warpConnector(rs, e0, e1, "morph");
    CHECK(last.samples.size() == first.samples.size());
    double maxd = 0;
    for (size_t i = 0; i < first.samples.size(); ++i) {
        maxd = std::max(maxd, std::hypot(last.samples[i].x - first.samples[i].x,
                                         last.samples[i].y - first.samples[i].y));
    }
    CHECK(maxd == 0.0);
}

static void test_cubic_differs_at_rest()
{
    const RestShape rs = buildRestShape({wiggleXY(0, 0, 400, 0, 80)});
    WarpEnd e0, e1;
    endsFromSpine(rs, &e0, &e1);
    const WarpResult morph = warpConnector(rs, e0, e1, "morph");
    const WarpResult cubic = warpConnector(rs, e0, e1, "cubic");
    CHECK(morph.samples.size() == cubic.samples.size());
    double maxd = 0;
    for (size_t i = 0; i < morph.samples.size(); ++i) {
        maxd = std::max(maxd, std::hypot(cubic.samples[i].x - morph.samples[i].x,
                                         cubic.samples[i].y - morph.samples[i].y));
    }
    CHECK(maxd > 0.5);
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
                                 const std::vector<InkSample> &samples)
{
    EncloseStrokeInput stroke;
    stroke.id = id;
    stroke.samples = samples;
    RecogLatch latch;
    return dispatchPenUp(doc, stroke, latch);
}

static void test_d39_delete_keeps_connector()
{
    DeviceDocument doc;
    addSg(doc, "A", 0, 0, 80, 80);
    addSg(doc, "C", 300, 0, 80, 80);
    const RecogDispatchResult d = penUp(doc, "ink_ac", lineXY(78, 40, 302, 40, 24));
    CHECK(d.outcome == RecogOutcome::Connector);
    const std::string cid = d.connector.connectorId;
    const DocNode *conn0 = doc.find(cid);
    CHECK(conn0);
    CHECK(!conn0->connectorInvalid);
    CHECK(conn0->fromPose.valid);
    CHECK(conn0->toPose.valid);
    CHECK(!conn0->warpedSamples.empty());
    const size_t nSamp = conn0->warpedSamples.size();

    CHECK(doc.commitJson(opEnvelope("remove_node:A", "remove_node",
                                    JsonValue::object({{"id", JsonValue::string("A")}})))
              .applied);
    CHECK(!doc.find("A"));
    CHECK(doc.find(cid));
    refreshAllConnectorWarps(doc);
    const DocNode *orphan = doc.find(cid);
    CHECK(orphan);
    CHECK(!orphan->connectorInvalid);
    CHECK(orphan->fromNodeId == "A");
    CHECK(orphan->warpedSamples.size() == nSamp);
    CHECK(doc.find("C"));

    CHECK(doc.undo().restored);
    CHECK(doc.find("A"));
    CHECK(doc.find(cid));
    refreshAllConnectorWarps(doc);
    const DocNode *glued = doc.find(cid);
    CHECK(glued);
    CHECK(glued->fromNodeId == "A");
    CHECK(!glued->connectorInvalid);
    CHECK(!glued->warpedSamples.empty());
}

static void test_live_move_rewarp()
{
    DeviceDocument doc;
    addSg(doc, "A", 0, 0, 80, 80);
    addSg(doc, "C", 300, 0, 80, 80);
    const RecogDispatchResult d = penUp(doc, "ink_ac", lineXY(78, 40, 302, 40, 24));
    CHECK(d.outcome == RecogOutcome::Connector);
    const DocNode *conn = doc.find(d.connector.connectorId);
    CHECK(conn && !conn->warpedSamples.empty());
    const double x0 = conn->warpedSamples.front().x;
    DocNode *a = nullptr;
    for (auto &n : doc.rootChildren) {
        if (n.id == "A")
            a = &n;
    }
    CHECK(a);
    a->transform.x += 40;
    refreshConnectorsBoundTo(doc, "A");
    conn = doc.find(d.connector.connectorId);
    CHECK(conn);
    CHECK(std::abs(conn->warpedSamples.front().x - x0) > 1.0);
}

static void test_attach_stays_on_pen_not_aabb()
{
    DeviceDocument doc;
    std::vector<InkSample> tri;
    const double xs[4] = {0, 50, 100, 0};
    const double ys[4] = {100, 0, 100, 100};
    for (int i = 0; i < 4; ++i) {
        InkSample s;
        s.x = xs[i];
        s.y = ys[i];
        s.t = double(i);
        tri.push_back(s);
    }
    addSg(doc, "A", 0, 0, 100, 100, &tri);
    addSg(doc, "C", 300, 0, 80, 80);
    const RecogDispatchResult d = penUp(doc, "ink_ac", lineXY(25, 50, 302, 40, 32));
    CHECK(d.outcome == RecogOutcome::Connector);
    const DocNode *conn = doc.find(d.connector.connectorId);
    CHECK(conn);
    CHECK(conn->fromAnchor.hasLocal);
    CHECK(conn->fromAnchor.kind == "edge");
    CHECK(conn->fromPose.valid);
    CHECK(conn->fromPose.x > 12.0);
    CHECK(std::abs(conn->fromPose.x - 25.0) < 8.0);
    CHECK(std::abs(conn->fromPose.y - 50.0) < 8.0);
    CHECK(!conn->restSpine.empty());
    CHECK(std::hypot(conn->fromPose.x - conn->restSpine.front().x,
                     conn->fromPose.y - conn->restSpine.front().y) < 0.5);
}

static void test_centre_keeps_pen_point()
{
    DeviceDocument doc;
    addSg(doc, "A", 0, 0, 80, 80);
    addSg(doc, "C", 300, 0, 80, 80);
    const RecogDispatchResult d = penUp(doc, "ink_ac", lineXY(40, 40, 302, 40, 32));
    CHECK(d.outcome == RecogOutcome::Connector);
    const DocNode *conn = doc.find(d.connector.connectorId);
    CHECK(conn);
    CHECK(conn->fromAnchor.kind == "centre");
    CHECK(conn->fromAnchor.hasLocal);
    CHECK(std::hypot(conn->fromPose.x - conn->restSpine.front().x,
                     conn->fromPose.y - conn->restSpine.front().y) < 0.5);
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

static void test_undo_move_restores_connector_pose()
{
    DeviceDocument doc;
    addSg(doc, "A", 0, 0, 80, 80);
    addSg(doc, "C", 300, 0, 80, 80);
    const RecogDispatchResult d = penUp(doc, "ink_ac", lineXY(78, 40, 302, 40, 24));
    CHECK(d.outcome == RecogOutcome::Connector);
    refreshAllConnectorWarps(doc);
    const DocNode *conn0 = doc.find(d.connector.connectorId);
    CHECK(conn0 && !conn0->warpedSamples.empty());
    const double x0 = conn0->warpedSamples.front().x;
    const double y0 = conn0->warpedSamples.front().y;
    const DocNode *a = doc.find("A");
    CHECK(a);
    CHECK(doc.commitJson(makeSetTransformOp("sst-a", "A", a->transform.x + 80, a->transform.y))
              .applied);
    refreshAllConnectorWarps(doc);
    const DocNode *moved = doc.find(d.connector.connectorId);
    CHECK(moved && !moved->warpedSamples.empty());
    CHECK(std::abs(moved->warpedSamples.front().x - x0) > 1.0);
    SmartBounds boxOnly;
    CHECK(nodeInvalidateAabb(doc, "A", boxOnly));
    SmartBounds hist;
    CHECK(unionHistoryRestoreAabb(doc, {"A"}, hist));
    const double spineEnd = moved->warpedSamples.back().x;
    CHECK(hist.x + hist.width + 1.0 >= spineEnd);
    CHECK(boxOnly.x + boxOnly.width + 1.0 < spineEnd || boxOnly.x > moved->warpedSamples.front().x);

    CHECK(doc.undo().restored);
    refreshAllConnectorWarps(doc);
    const DocNode *restored = doc.find(d.connector.connectorId);
    CHECK(restored && !restored->warpedSamples.empty());
    CHECK(std::abs(restored->warpedSamples.front().x - x0) < 1.0);
    CHECK(std::abs(restored->warpedSamples.front().y - y0) < 1.0);
}

int main()
{
    test_i3_morph_identity();
    test_i1_never_rebake();
    test_cubic_differs_at_rest();
    test_d39_delete_keeps_connector();
    test_live_move_rewarp();
    test_attach_stays_on_pen_not_aabb();
    test_centre_keeps_pen_point();
    test_undo_move_restores_connector_pose();
    if (g_fails) {
        std::cerr << "connector_warp_test: " << g_fails << " failed\n";
        return 1;
    }
    std::cout << "connector_warp_test: OK\n";
    return 0;
}
