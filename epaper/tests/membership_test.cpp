/**
 * Host tests for STORY-EP-017 / [SRS-EP-10] draw-into membership.
 * Maps draw-into-membership.feature.
 *
 * Build: ./tests/run_device_document_test.sh
 */

#include "document/device_document.hpp"
#include "document/recognizer_dispatch.hpp"

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

static bool near(double a, double b, double eps = 1e-9)
{
    return std::abs(a - b) <= eps;
}

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

static void appendInk(DeviceDocument &doc, const std::string &id,
                      const std::vector<InkSample> &samples)
{
    JsonValue::Array arr;
    for (const auto &s : samples)
        arr.push_back(sampleToJson(s));
    JsonValue::Object style;
    style.emplace_back("stroke", JsonValue::string("#000"));
    style.emplace_back("strokeWidth", JsonValue::number(2));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("samples", JsonValue::array(std::move(arr)));
    payload.emplace_back("style", JsonValue::object(std::move(style)));
        CHECK(doc.commitJson(opEnvelope("append_ink:" + id, "append_ink", JsonValue::object(std::move(payload)))).applied);
}

static void createSg(DeviceDocument &doc, const std::string &id, double tx, double ty, double w,
                     double h, const std::vector<InkSample> &existingLocal,
                     std::pair<double, double> existingUv)
{
    JsonValue::Array children;
    if (!existingLocal.empty()) {
        JsonValue::Object lo;
        lo.emplace_back("u", JsonValue::number(existingUv.first));
        lo.emplace_back("v", JsonValue::number(existingUv.second));
        JsonValue::Object style;
        style.emplace_back("stroke", JsonValue::string("#000"));
        style.emplace_back("strokeWidth", JsonValue::number(2));
        JsonValue::Object child;
        child.emplace_back("id", JsonValue::string(id + "_old"));
        child.emplace_back("kind", JsonValue::string("ink"));
        child.emplace_back("role", JsonValue::string("content"));
        JsonValue::Array arr;
        for (const auto &s : existingLocal)
            arr.push_back(sampleToJson(s));
        child.emplace_back("samples", JsonValue::array(std::move(arr)));
        child.emplace_back("style", JsonValue::object(std::move(style)));
        child.emplace_back("layoutOffset", JsonValue::object(std::move(lo)));
        children.push_back(JsonValue::object(std::move(child)));
    }

    JsonValue::Object bstyle;
    bstyle.emplace_back("stroke", JsonValue::string("#000"));
    bstyle.emplace_back("strokeWidth", JsonValue::number(2));
    JsonValue::Object bchild;
    bchild.emplace_back("id", JsonValue::string(id + "_b"));
    bchild.emplace_back("kind", JsonValue::string("ink"));
    bchild.emplace_back("role", JsonValue::string("boundary"));
    auto bpoly = pts({{0, 0}, {w, 0}, {w, h}, {0, h}, {0, 0}});
    JsonValue::Array barr;
    for (const auto &s : bpoly)
        barr.push_back(sampleToJson(s));
    bchild.emplace_back("samples", JsonValue::array(std::move(barr)));
    bchild.emplace_back("style", JsonValue::object(std::move(bstyle)));
    JsonValue::Array kids;
    kids.push_back(JsonValue::object(std::move(bchild)));
    for (auto &c : children)
        kids.push_back(std::move(c));

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
    payload.emplace_back("children", JsonValue::array(std::move(kids)));
        CHECK(doc.commitJson(opEnvelope("create_smart_group:" + id, "create_smart_group", JsonValue::object(std::move(payload)))).applied);
}

static void test_join_with_uv_bounds_unchanged()
{
    DeviceDocument doc;
    const auto existingUv = std::make_pair(0.2, 0.3);
    createSg(doc, "sg_1", 0, 0, 200, 200, pts({{40, 60}, {50, 60}}), existingUv);
    const DocNode *sg0 = doc.find("sg_1");
    CHECK(sg0);
    const SmartBounds boundsBefore = sg0->smartBounds;

    appendInk(doc, "new_ink", pts({{100, 100}, {110, 105}, {105, 120}}));
    const auto t0 = std::chrono::steady_clock::now();
    const MembershipResult joined = tryDrawIntoMembership(doc, "new_ink");
    const auto t1 = std::chrono::steady_clock::now();
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    CHECK(joined.kind == MembershipKind::Joined);
    CHECK(joined.smartGroupId == "sg_1");
    CHECK(ns < 300000000); // p95 budget 300 ms — host should be far under

    const DocNode *sg = doc.find("sg_1");
    CHECK(sg);
    CHECK(near(sg->smartBounds.x, boundsBefore.x));
    CHECK(near(sg->smartBounds.y, boundsBefore.y));
    CHECK(near(sg->smartBounds.width, boundsBefore.width));
    CHECK(near(sg->smartBounds.height, boundsBefore.height));

    const DocNode *old = nullptr;
    const DocNode *neu = nullptr;
    for (const auto &c : sg->children) {
        if (c.id == "sg_1_old")
            old = &c;
        if (c.id == "new_ink")
            neu = &c;
    }
    CHECK(old && neu);
    CHECK(old->layoutOffset && near(old->layoutOffset->first, 0.2)
          && near(old->layoutOffset->second, 0.3));
    CHECK(neu->role && *neu->role == "content");
    CHECK(neu->layoutOffset.has_value());
    CHECK(!(near(neu->layoutOffset->first, 0.2) && near(neu->layoutOffset->second, 0.3)));
    CHECK(!doc.find("new_ink") || doc.find("new_ink") == neu);
    // Root must not still hold the free ink
    bool onRoot = false;
    for (const auto &n : doc.rootChildren)
        if (n.id == "new_ink")
            onRoot = true;
    CHECK(!onRoot);
}

static void test_later_sibling_wins()
{
    DeviceDocument doc;
    createSg(doc, "sg_a", 0, 0, 100, 100, {}, {0.5, 0.5});
    createSg(doc, "sg_b", 0, 0, 100, 100, {}, {0.5, 0.5});
    appendInk(doc, "ink", pts({{20, 20}, {30, 30}}));
    const MembershipResult r = tryDrawIntoMembership(doc, "ink");
    CHECK(r.kind == MembershipKind::Joined);
    CHECK(r.smartGroupId == "sg_b");
    bool inA = false;
    for (const auto &c : doc.find("sg_a")->children)
        if (c.id == "ink")
            inA = true;
    CHECK(!inA);
    bool inB = false;
    for (const auto &c : doc.find("sg_b")->children)
        if (c.id == "ink")
            inB = true;
    CHECK(inB);
}

static void test_no_qualifying_group()
{
    DeviceDocument doc;
    createSg(doc, "sg_1", 0, 0, 50, 50, {}, {0.5, 0.5});
    appendInk(doc, "far", pts({{400, 400}, {410, 410}}));
    const MembershipResult r = tryDrawIntoMembership(doc, "far");
    CHECK(r.kind == MembershipKind::None);
    CHECK(r.reason == "no_qualifying_group");
    bool onRoot = false;
    for (const auto &n : doc.rootChildren)
        if (n.id == "far")
            onRoot = true;
    CHECK(onRoot);
}

static void test_no_reflow_existing()
{
    DeviceDocument doc2;
    JsonValue::Array children;
    for (int i = 0; i < 3; ++i) {
        const std::string cid = "c" + std::to_string(i);
        JsonValue::Object lo;
        lo.emplace_back("u", JsonValue::number(0.1 * (i + 1)));
        lo.emplace_back("v", JsonValue::number(0.2 * (i + 1)));
        JsonValue::Object style;
        style.emplace_back("stroke", JsonValue::string("#000"));
        style.emplace_back("strokeWidth", JsonValue::number(2));
        JsonValue::Object child;
        child.emplace_back("id", JsonValue::string(cid));
        child.emplace_back("kind", JsonValue::string("ink"));
        child.emplace_back("role", JsonValue::string("content"));
        auto samples = pts({{10.0 + i * 20, 10.0}, {15.0 + i * 20, 15.0}});
        JsonValue::Array arr;
        for (const auto &s : samples)
            arr.push_back(sampleToJson(s));
        child.emplace_back("samples", JsonValue::array(std::move(arr)));
        child.emplace_back("style", JsonValue::object(std::move(style)));
        child.emplace_back("layoutOffset", JsonValue::object(std::move(lo)));
        children.push_back(JsonValue::object(std::move(child)));
    }
    JsonValue::Object bstyle;
    bstyle.emplace_back("stroke", JsonValue::string("#000"));
    bstyle.emplace_back("strokeWidth", JsonValue::number(2));
    JsonValue::Object bchild;
    bchild.emplace_back("id", JsonValue::string("sg_1_b"));
    bchild.emplace_back("kind", JsonValue::string("ink"));
    bchild.emplace_back("role", JsonValue::string("boundary"));
    auto bpoly = pts({{0, 0}, {200, 0}, {200, 200}, {0, 200}, {0, 0}});
    JsonValue::Array barr;
    for (const auto &s : bpoly)
        barr.push_back(sampleToJson(s));
    bchild.emplace_back("samples", JsonValue::array(std::move(barr)));
    bchild.emplace_back("style", JsonValue::object(std::move(bstyle)));
    JsonValue::Array kids;
    kids.push_back(JsonValue::object(std::move(bchild)));
    for (auto &c : children)
        kids.push_back(std::move(c));
    JsonValue::Object b;
    b.emplace_back("x", JsonValue::number(0));
    b.emplace_back("y", JsonValue::number(0));
    b.emplace_back("width", JsonValue::number(200));
    b.emplace_back("height", JsonValue::number(200));
    JsonValue::Object t;
    t.emplace_back("x", JsonValue::number(0));
    t.emplace_back("y", JsonValue::number(0));
    t.emplace_back("rotation", JsonValue::number(0));
    t.emplace_back("scaleX", JsonValue::number(1));
    t.emplace_back("scaleY", JsonValue::number(1));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string("sg_1"));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    payload.emplace_back("transform", JsonValue::object(std::move(t)));
    payload.emplace_back("children", JsonValue::array(std::move(kids)));
        CHECK(doc2.commitJson(opEnvelope("create_smart_group:sg_1", "create_smart_group", JsonValue::object(std::move(payload)))).applied);

    std::vector<std::pair<double, double>> beforeUv;
    std::vector<std::pair<double, double>> beforeSample0;
    for (const auto &c : doc2.find("sg_1")->children) {
        if (!c.layoutOffset)
            continue;
        beforeUv.push_back(*c.layoutOffset);
        beforeSample0.push_back({c.samples[0].x, c.samples[0].y});
    }

    appendInk(doc2, "new", pts({{80, 80}, {90, 90}}));
    CHECK(tryDrawIntoMembership(doc2, "new").kind == MembershipKind::Joined);

    int i = 0;
    for (const auto &c : doc2.find("sg_1")->children) {
        if (c.id == "new" || !c.layoutOffset)
            continue;
        CHECK(near(c.layoutOffset->first, beforeUv[static_cast<size_t>(i)].first));
        CHECK(near(c.layoutOffset->second, beforeUv[static_cast<size_t>(i)].second));
        CHECK(near(c.samples[0].x, beforeSample0[static_cast<size_t>(i)].first));
        CHECK(near(c.samples[0].y, beforeSample0[static_cast<size_t>(i)].second));
        ++i;
    }
}

static void test_membership_undo()
{
    DeviceDocument doc;
    createSg(doc, "sg_1", 0, 0, 100, 100, {}, {0.5, 0.5});
    appendInk(doc, "ink", pts({{10, 10}, {20, 20}}));
    const std::string before = doc.snapshotString();
    CHECK(tryDrawIntoMembership(doc, "ink").kind == MembershipKind::Joined);
    const UndoResult u = doc.undo();
    CHECK(u.restored);
    CHECK(doc.snapshotString() == before);
}

static void test_failed_enclose_falls_through_membership()
{
    DeviceDocument doc;
    createSg(doc, "sg_1", 40, 40, 120, 120, {}, {0.5, 0.5});
    EncloseStrokeInput stroke;
    stroke.id = "box_stroke";
    stroke.samples = pts({{50, 50}, {150, 50}, {150, 150}, {50, 150}, {50, 50}});
    RecogLatch latch;
    latch.inkBox = true;
    latch.connector = true;
    const RecogDispatchResult d = dispatchPenUp(doc, stroke, latch);
    CHECK(d.outcome == RecogOutcome::Membership);
    CHECK(d.enclose.kind == EncloseKind::OrdinaryInk);
    bool underSg = false;
    for (const auto &c : doc.find("sg_1")->children)
        if (c.id == "box_stroke")
            underSg = true;
    CHECK(underSg);
}

static void test_translated_group_local_samples()
{
    DeviceDocument doc;
    createSg(doc, "sg_1", 40, 40, 120, 120, {}, {0.5, 0.5});
    appendInk(doc, "ink", pts({{100, 100}, {110, 105}}));
    CHECK(tryDrawIntoMembership(doc, "ink").kind == MembershipKind::Joined);
    const DocNode *sg = doc.find("sg_1");
    CHECK(sg);
    const DocNode *ink = nullptr;
    for (const auto &c : sg->children)
        if (c.id == "ink")
            ink = &c;
    CHECK(ink);
    CHECK(near(ink->samples[0].x, 60)); // 100 - 40
    CHECK(near(ink->samples[0].y, 60));
    CHECK(near(sg->smartBounds.width, 120));
}

/** @fix [STORY-EP-017] fixedInk join vs resized (scale≠1) parent — not /scale */
static void test_fixed_ink_join_ignores_parent_scale()
{
    DeviceDocument doc;
    createSg(doc, "sg_1", 40, 40, 100, 100, {}, {0.5, 0.5});
    JsonValue::Object t;
    t.emplace_back("x", JsonValue::number(40));
    t.emplace_back("y", JsonValue::number(40));
    t.emplace_back("rotation", JsonValue::number(0));
    t.emplace_back("scaleX", JsonValue::number(2));
    t.emplace_back("scaleY", JsonValue::number(2));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string("sg_1"));
    payload.emplace_back("transform", JsonValue::object(std::move(t)));
        CHECK(doc.commitJson(opEnvelope("set_smart_transform:sg_1", "set_smart_transform", JsonValue::object(std::move(payload)))).applied);
    // World point 40px in from the box origin; paint is local+translate (no scale).
    appendInk(doc, "ink", pts({{80, 90}, {90, 95}}));
    CHECK(tryDrawIntoMembership(doc, "ink").kind == MembershipKind::Joined);
    const DocNode *sg = doc.find("sg_1");
    CHECK(sg);
    const DocNode *ink = nullptr;
    for (const auto &c : sg->children)
        if (c.id == "ink")
            ink = &c;
    CHECK(ink);
    CHECK(near(ink->samples[0].x, 40)); // 80 - 40, not (80-40)/2
    CHECK(near(ink->samples[0].y, 50));
    const Vec2 world = smartLocalToWorld(ink->samples[0].x, ink->samples[0].y, *sg, "content",
                                         ink->layoutOffset, nullptr);
    CHECK(near(world.x, 80));
    CHECK(near(world.y, 90));
}

static void test_aabb_inside_outside_boundary_does_not_join()
{
    DeviceDocument doc;
    JsonValue::Object bstyle;
    bstyle.emplace_back("stroke", JsonValue::string("#000"));
    bstyle.emplace_back("strokeWidth", JsonValue::number(2));
    JsonValue::Object bchild;
    bchild.emplace_back("id", JsonValue::string("dia_b"));
    bchild.emplace_back("kind", JsonValue::string("ink"));
    bchild.emplace_back("role", JsonValue::string("boundary"));
    auto diamond = pts({{100, 0}, {200, 100}, {100, 200}, {0, 100}, {100, 0}});
    JsonValue::Array barr;
    for (const auto &s : diamond)
        barr.push_back(sampleToJson(s));
    bchild.emplace_back("samples", JsonValue::array(std::move(barr)));
    bchild.emplace_back("style", JsonValue::object(std::move(bstyle)));
    JsonValue::Object b;
    b.emplace_back("x", JsonValue::number(0));
    b.emplace_back("y", JsonValue::number(0));
    b.emplace_back("width", JsonValue::number(200));
    b.emplace_back("height", JsonValue::number(200));
    JsonValue::Object t;
    t.emplace_back("x", JsonValue::number(0));
    t.emplace_back("y", JsonValue::number(0));
    t.emplace_back("rotation", JsonValue::number(0));
    t.emplace_back("scaleX", JsonValue::number(1));
    t.emplace_back("scaleY", JsonValue::number(1));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string("dia"));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    payload.emplace_back("transform", JsonValue::object(std::move(t)));
    payload.emplace_back("children", JsonValue::array({JsonValue::object(std::move(bchild))}));
    CHECK(doc.commitJson(opEnvelope("create_smart_group:dia", "create_smart_group",
                                    JsonValue::object(std::move(payload))))
              .applied);

    const auto along = pts({{10, 5}, {190, 5}});
    CHECK(fractionSamplesInside(along, smartGroupWorldBounds(*doc.find("dia"))) >= 0.8);
    appendInk(doc, "along_aabb", along);
    const MembershipResult r = tryDrawIntoMembership(doc, "along_aabb");
    CHECK(r.kind == MembershipKind::None);
    bool onRoot = false;
    for (const auto &n : doc.rootChildren)
        if (n.id == "along_aabb")
            onRoot = true;
    CHECK(onRoot);
}

int main()
{
    test_join_with_uv_bounds_unchanged();
    test_later_sibling_wins();
    test_no_qualifying_group();
    test_no_reflow_existing();
    test_membership_undo();
    test_failed_enclose_falls_through_membership();
    test_translated_group_local_samples();
    test_fixed_ink_join_ignores_parent_scale();
    test_aabb_inside_outside_boundary_does_not_join();

    if (g_fails) {
        std::cerr << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "membership_test: OK\n";
    return 0;
}
