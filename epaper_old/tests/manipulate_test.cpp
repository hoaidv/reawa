/**
 * Host tests for STORY-EP-019 / [SRS-EP-11] [SRS-EP-14].
 * Maps smart-group-selection.feature (descriptor, move, resize, LOD, invert).
 */

#include "document/capability.hpp"
#include "document/device_document.hpp"
#include "document/manipulate.hpp"
#include "document/recognize_enclose.hpp"

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

static bool near(double a, double b, double eps = 1e-6)
{
    return std::abs(a - b) <= eps;
}

static JsonValue createSg(const std::string &id, const std::string &mode, double bw, double bh)
{
    const std::string json = std::string("{\"opId\":\"op-") + id
        + "\",\"type\":\"create_smart_group\",\"source\":\"epaper\",\"payload\":{"
          "\"id\":\""
        + id + "\",\"bounds\":{\"x\":0,\"y\":0,\"width\":" + std::to_string(bw) + ",\"height\":"
        + std::to_string(bh)
        + "},\"transform\":{\"x\":0,\"y\":0,\"rotation\":0,\"scaleX\":1,\"scaleY\":1},"
          "\"inkScaleMode\":\""
        + mode
        + "\",\"children\":["
          "{\"id\":\""
        + id
        + "_c\",\"kind\":\"ink\",\"role\":\"content\",\"layoutOffset\":{\"u\":0.25,\"v\":0.4},"
          "\"samples\":[{\"x\":10,\"y\":10},{\"x\":20,\"y\":10}]},"
          "{\"id\":\""
        + id
        + "_b\",\"kind\":\"ink\",\"role\":\"boundary\",\"samples\":[{\"x\":0,\"y\":0},{\"x\":"
        + std::to_string(bw) + ",\"y\":0},{\"x\":" + std::to_string(bw) + ",\"y\":"
        + std::to_string(bh) + "},{\"x\":0,\"y\":" + std::to_string(bh) + "}]}"
                                                                        "]}}";
    return parseJson(json);
}

static void test_descriptor()
{
    CHECK(smartGroupVerbsExact(descriptorFor(NodeKind::SmartGroup)));
    CHECK(descriptorFor(NodeKind::Ink).has(Verb::Select));
    CHECK(!descriptorFor(NodeKind::Ink).has(Verb::Move));
    CHECK(descriptorFor(NodeKind::Connector).has(Verb::Select));
    CHECK(!descriptorFor(NodeKind::Connector).has(Verb::Move));
}

static void test_router_no_kind_branch()
{
    const auto cap = descriptorFor(NodeKind::SmartGroup);
    CHECK(resolvePress(cap, true, false, false, true) == GestureKind::SelectMove);
    CHECK(resolvePress(cap, true, true, false, true) == GestureKind::Resize);
    CHECK(resolvePress(cap, true, false, true, true) == GestureKind::ToggleMode);
    CHECK(resolvePress(cap, false, false, false, true) == GestureKind::Unavailable);
    CHECK(resolvePress(cap, true, false, false, false) == GestureKind::Marquee);
    const auto ink = descriptorFor(NodeKind::Ink);
    CHECK(resolvePress(ink, true, true, false, true) == GestureKind::SelectMove);
}

static void test_move_commit_equals_preview()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(createSg("sg_m", "withBounds", 200, 200)).applied);
    const DocNode *orig = doc.find("sg_m");
    CHECK(orig);
    doc.beginGesture();
    const SmartTransform origin = orig->transform;
    SmartTransform live = origin;
    live.x = 40;
    live.y = 15;
    live.rotation = 0;
    DocNode preview = *orig;
    applyLiveGeometry(preview, live, preview.smartBounds);
    doc.previewManipulationFrame();
    CHECK(near(preview.transform.x, 40) && near(preview.transform.y, 15));
    CHECK(near(preview.transform.rotation, 0));
    SetSmartTransformEdit move("mv1", "sg_m", origin, orig->smartBounds, preview.transform,
                               orig->smartBounds, false);
    CHECK(doc.commitEdit(move).applied);
    const DocNode *done = doc.find("sg_m");
    CHECK(done && near(done->transform.x, 40) && near(done->transform.y, 15));
    CHECK(near(done->transform.rotation, 0));
}

static void test_fixed_ink_resize_keeps_uv_and_sample_size()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(createSg("sg_f", "fixedInk", 100, 80)).applied);
    const DocNode *c0 = nullptr;
    for (const auto &ch : doc.find("sg_f")->children) {
        if (ch.role && *ch.role == "content")
            c0 = &ch;
    }
    CHECK(c0 && c0->layoutOffset);
    if (!c0 || !c0->layoutOffset)
        return;
    const auto uv = *c0->layoutOffset;
    const double dx = c0->samples[1].x - c0->samples[0].x;
    const double dy = c0->samples[1].y - c0->samples[0].y;
    const WorldBox origin = originWorldAabb(doc.find("sg_f")->smartBounds, doc.find("sg_f")->transform);
    const WorldBox grown = resizeWorldAabbFromHandle(origin, ResizeHandle::Se, origin.maxX + 50,
                                                     origin.maxY + 40);
    const auto mapped = smartTransformFromWorldAabb(grown, doc.find("sg_f")->smartBounds,
                                                    doc.find("sg_f")->transform, "fixedInk");
    SetSmartTransformEdit rz("rz1", "sg_f", doc.find("sg_f")->transform, doc.find("sg_f")->smartBounds,
                             mapped.transform, mapped.bounds, true);
    CHECK(doc.commitEdit(rz).applied);
    const DocNode *c1 = nullptr;
    for (const auto &ch : doc.find("sg_f")->children) {
        if (ch.role && *ch.role == "content")
            c1 = &ch;
    }
    CHECK(c1 && c1->layoutOffset);
    CHECK(near(c1->layoutOffset->first, uv.first) && near(c1->layoutOffset->second, uv.second));
    CHECK(near(c1->samples[1].x - c1->samples[0].x, dx, 1.0));
    CHECK(near(c1->samples[1].y - c1->samples[0].y, dy, 1.0));
    CHECK(near(mapped.transform.scaleX, 1.5, 0.05));
    const DocNode *sg = doc.find("sg_f");
    const Vec2 cW = smartLocalToWorld(c1->samples[0].x, c1->samples[0].y, *sg, "content", c1->layoutOffset,
                                      nullptr);
    CHECK(near(cW.x, c1->samples[0].x + sg->transform.x, 1e-6));
    CHECK(near(cW.y, c1->samples[0].y + sg->transform.y, 1e-6));
    const DocNode *b1 = nullptr;
    for (const auto &ch : sg->children) {
        if (ch.role && *ch.role == "boundary")
            b1 = &ch;
    }
    CHECK(b1);
    const Vec2 bW0 = smartLocalToWorld(b1->samples[0].x, b1->samples[0].y, *sg, "boundary", {}, nullptr);
    const Vec2 bW1 = smartLocalToWorld(b1->samples[1].x, b1->samples[1].y, *sg, "boundary", {}, nullptr);
    CHECK(std::abs(bW1.x - bW0.x) > std::abs(b1->samples[1].x - b1->samples[0].x) + 1.0);
}

static void test_with_bounds_scales()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(createSg("sg_w", "withBounds", 100, 100)).applied);
    const WorldBox origin = originWorldAabb(doc.find("sg_w")->smartBounds, doc.find("sg_w")->transform);
    const WorldBox grown = resizeWorldAabbFromHandle(origin, ResizeHandle::Se, origin.maxX + 100,
                                                     origin.maxY + 100);
    const auto mapped = smartTransformFromWorldAabb(grown, doc.find("sg_w")->smartBounds,
                                                    doc.find("sg_w")->transform, "withBounds");
    CHECK(near(mapped.transform.scaleX, 2.0, 0.05));
    CHECK(near(mapped.transform.scaleY, 2.0, 0.05));
    CHECK(near(mapped.bounds.width, 100));
}

static void test_resize_undo_restores_scale()
{
    DeviceDocument doc;
    CHECK(doc.commitJson(createSg("sg_u", "withBounds", 100, 100)).applied);
    const DocNode *orig = doc.find("sg_u");
    CHECK(orig);
    const SmartTransform originT = orig->transform;
    const SmartBounds originB = orig->smartBounds;
    const WorldBox origin = originWorldAabb(originB, originT);
    const WorldBox grown = resizeWorldAabbFromHandle(origin, ResizeHandle::Se, origin.maxX + 100,
                                                     origin.maxY + 100);
    const auto mapped = smartTransformFromWorldAabb(grown, originB, originT, "withBounds");
    CHECK(doc.applyLiveSmartGeometry("sg_u", mapped.transform, mapped.bounds));
    SetSmartTransformEdit rz("rz-undo", "sg_u", originT, originB, mapped.transform, mapped.bounds,
                             true);
    CHECK(doc.commitEdit(rz).applied);
    const DocNode *grownN = doc.find("sg_u");
    CHECK(grownN && near(grownN->transform.scaleX, 2.0, 0.05));
    CHECK(doc.undo().restored);
    const DocNode *undone = doc.find("sg_u");
    CHECK(undone && near(undone->transform.scaleX, 1.0) && near(undone->transform.scaleY, 1.0));
    CHECK(near(undone->transform.x, originT.x) && near(undone->transform.y, originT.y));
}

static void test_inverted_resize_non_negative()
{
    WorldBox box{0, 0, 40, 30};
    const WorldBox n = resizeWorldAabbFromHandle(box, ResizeHandle::Se, -10, -8);
    CHECK(n.maxX - n.minX >= 1);
    CHECK(n.maxY - n.minY >= 1);
}

static void test_lod()
{
    CHECK(lodAllows(200, 200, 1.0));
    CHECK(!lodAllows(50, 200, 1.0));
    CHECK(lodAllowsPanel(200, 100));
    CHECK(!lodAllowsPanel(80, 200));
    CHECK(lodAllowsPanel(96, 400));
    CHECK(lodAllows(48, 48, 1.0) == false); // 48du enclose at 1:1 would fail naive LOD
}

int main()
{
    test_descriptor();
    test_router_no_kind_branch();
    test_move_commit_equals_preview();
    test_fixed_ink_resize_keeps_uv_and_sample_size();
    test_with_bounds_scales();
    test_resize_undo_restores_scale();
    test_inverted_resize_non_negative();
    test_lod();
    if (g_fails) {
        std::cerr << g_fails << " failed\n";
        return 1;
    }
    std::cout << "manipulate_test ok\n";
    return 0;
}
