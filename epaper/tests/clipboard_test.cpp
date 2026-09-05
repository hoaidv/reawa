/**
 * Host tests for STORY-EP-044 / [SRS-EP-31] [SRS-EP-33] [SRS-EP-11] tap vs travel.
 * Maps clipboard.feature. No Qt — InputHub travel defer is exercised on device.
 * @implements [SRS-EP-32] paste chrome hidden while Transforming
 *
 * Build: ./tests/run_device_document_test.sh
 */

#include "document/device_document.hpp"
#include "drawing/tools/clipboard.hpp"
#include "drawing/tools/contexts/selection_context.hpp"
#include "drawing/tools/hold_still.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace epaper::document;
using epaper::tools::clipboard;
using epaper::tools::clipops::commitCut;
using epaper::tools::clipops::commitPaste;
using epaper::tools::clipops::copyToSlot;
using epaper::tools::clipops::PasteRefuse;
using epaper::tools::clipops::slotRoots;
using epaper::tools::clipops::unionAabb;
using epaper::tools::holdTravelExceeded;
using epaper::tools::SelectionContext;
using epaper::tools::SelectionPhase;

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

static bool near(double a, double b, double eps = 1.0)
{
    return std::abs(a - b) <= eps;
}

static JsonValue inkSamplesJson(double x, double y, double w, double h)
{
    JsonValue::Array samples;
    auto pt = [](double px, double py) {
        JsonValue::Object o;
        o.emplace_back("x", JsonValue::number(px));
        o.emplace_back("y", JsonValue::number(py));
        return JsonValue::object(std::move(o));
    };
    samples.push_back(pt(x, y));
    samples.push_back(pt(x + w, y));
    samples.push_back(pt(x + w, y + h));
    samples.push_back(pt(x, y + h));
    return JsonValue::array(std::move(samples));
}

static JsonValue appendInkOp(const std::string &id, double x, double y, double w, double h)
{
    JsonValue::Object style;
    style.emplace_back("stroke", JsonValue::string("#1C2430"));
    style.emplace_back("strokeWidth", JsonValue::number(2));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("samples", inkSamplesJson(x, y, w, h));
    payload.emplace_back("style", JsonValue::object(std::move(style)));
    return opEnvelope("append_ink:" + id, "append_ink", JsonValue::object(std::move(payload)));
}

static JsonValue inkChild(const std::string &id, double x, double y, double w, double h)
{
    JsonValue::Object style;
    style.emplace_back("stroke", JsonValue::string("#1C2430"));
    style.emplace_back("strokeWidth", JsonValue::number(2));
    JsonValue::Object o;
    o.emplace_back("id", JsonValue::string(id));
    o.emplace_back("kind", JsonValue::string("ink"));
    o.emplace_back("samples", inkSamplesJson(x, y, w, h));
    o.emplace_back("style", JsonValue::object(std::move(style)));
    return JsonValue::object(std::move(o));
}

static JsonValue createSmartGroupOp(const std::string &id, double x, double y, double w, double h,
                                    JsonValue children)
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
    payload.emplace_back("children", std::move(children));
    return opEnvelope("create_smart_group:" + id, "create_smart_group",
                      JsonValue::object(std::move(payload)));
}

static JsonValue createFrameOp(const std::string &id, double x, double y, double w, double h)
{
    JsonValue::Object b;
    b.emplace_back("minX", JsonValue::number(x));
    b.emplace_back("minY", JsonValue::number(y));
    b.emplace_back("maxX", JsonValue::number(x + w));
    b.emplace_back("maxY", JsonValue::number(y + h));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    return opEnvelope("create_frame:" + id, "create_frame", JsonValue::object(std::move(payload)));
}

static int countNodes(const DeviceDocument &doc)
{
    int n = 0;
    doc.forEachPaintNode([&](const DocNode &) { ++n; });
    return n;
}

static void test_copy_does_not_push_undo()
{
    clipboard().reset();
    DeviceDocument doc;
    CHECK(doc.commitJson(appendInkOp("I1", 0, 0, 40, 30)).applied);
    const std::size_t depth = doc.undoDepth();
    const int n = countNodes(doc);
    copyToSlot(doc, {"I1"}, clipboard());
    CHECK(doc.find("I1"));
    CHECK(!clipboard().empty());
    CHECK(clipboard().nodes[0].id == "I1");
    CHECK(doc.undoDepth() == depth);
    CHECK(countNodes(doc) == n);
}

static void test_cut_removes_roots_leaves_empty_group()
{
    clipboard().reset();
    DeviceDocument doc;
    JsonValue::Array kids;
    kids.push_back(inkChild("C", 0, 0, 40, 30));
    CHECK(doc.commitJson(createSmartGroupOp("G", 0, 0, 80, 60, JsonValue::array(std::move(kids))))
              .applied);
    CHECK(doc.find("G"));
    CHECK(doc.find("C"));
    const std::size_t depth = doc.undoDepth();
    doc.clearPublishQueue();
    const auto r = commitCut(doc, clipboard(), {"C"}, /*enqueue=*/false);
    CHECK(r.applied);
    CHECK(!doc.find("C"));
    CHECK(doc.find("G"));
    CHECK(!clipboard().empty());
    CHECK(clipboard().nodes.size() == 1);
    CHECK(clipboard().nodes[0].id == "C");
    CHECK(doc.undoDepth() == depth + 1);
    CHECK(doc.publishQueue().empty());
}

static void test_paste_press_origin_no_wire()
{
    clipboard().reset();
    DeviceDocument doc;
    CHECK(doc.commitJson(appendInkOp("I1", 0, 0, 40, 30)).applied);
    copyToSlot(doc, {"I1"}, clipboard());
    SmartBounds before;
    CHECK(unionAabb(clipboard().nodes, before));
    CHECK(near(before.width, 40));
    CHECK(near(before.height, 30));
    const std::size_t depth = doc.undoDepth();
    doc.clearPublishQueue();
    const auto out = commitPaste(doc, clipboard(), 100, 200, "", /*enqueue=*/false);
    CHECK(out.result.applied);
    CHECK(out.refuse == PasteRefuse::None);
    CHECK(doc.find("I1"));
    int pasted = 0;
    SmartBounds u;
    bool any = false;
    doc.forEachPaintNode([&](const DocNode &n) {
        if (n.id == "I1")
            return;
        if (n.kind != NodeKind::Ink)
            return;
        ++pasted;
        SmartBounds b;
        if (!epaper::document::nodeWorldAabb(n, b))
            return;
        if (!any) {
            u = b;
            any = true;
            return;
        }
        const double minX = std::min(u.x, b.x);
        const double minY = std::min(u.y, b.y);
        const double maxX = std::max(u.x + u.width, b.x + b.width);
        const double maxY = std::max(u.y + u.height, b.y + b.height);
        u.x = minX;
        u.y = minY;
        u.width = maxX - minX;
        u.height = maxY - minY;
    });
    CHECK(pasted == 1);
    CHECK(any);
    CHECK(near(u.x, 100));
    CHECK(near(u.y, 200));
    CHECK(!clipboard().empty());
    CHECK(clipboard().nodes[0].id == "I1");
    CHECK(doc.undoDepth() == depth + 1);
    CHECK(doc.publishQueue().empty());
}

static void test_empty_slot_paste_noop()
{
    clipboard().reset();
    DeviceDocument doc;
    CHECK(doc.commitJson(appendInkOp("I1", 0, 0, 40, 30)).applied);
    const std::size_t depth = doc.undoDepth();
    const int n = countNodes(doc);
    const auto out = commitPaste(doc, clipboard(), 10, 10, "", false);
    CHECK(!out.result.applied);
    CHECK(out.refuse == PasteRefuse::Empty);
    CHECK(doc.undoDepth() == depth);
    CHECK(countNodes(doc) == n);
}

static void test_paste_onto_live_originals_refused()
{
    clipboard().reset();
    DeviceDocument doc;
    CHECK(doc.commitJson(appendInkOp("S", 0, 0, 40, 30)).applied);
    copyToSlot(doc, {"S"}, clipboard());
    const int n = countNodes(doc);
    const std::size_t depth = doc.undoDepth();
    const auto out = commitPaste(doc, clipboard(), 10, 10, "S", false);
    CHECK(!out.result.applied);
    CHECK(out.refuse == PasteRefuse::LiveOriginal);
    CHECK(countNodes(doc) == n);
    CHECK(doc.undoDepth() == depth);
    CHECK(!clipboard().empty());
}

static void test_cut_then_paste_undo_two_entries()
{
    clipboard().reset();
    DeviceDocument doc;
    CHECK(doc.commitJson(appendInkOp("I1", 0, 0, 40, 30)).applied);
    CHECK(commitCut(doc, clipboard(), {"I1"}, false).applied);
    CHECK(!doc.find("I1"));
    const auto out = commitPaste(doc, clipboard(), 100, 200, "", false);
    CHECK(out.result.applied);
    CHECK(!doc.find("I1"));
    int copies = 0;
    doc.forEachPaintNode([&](const DocNode &n) {
        if (n.kind == NodeKind::Ink)
            ++copies;
    });
    CHECK(copies == 1);
    CHECK(doc.undo().restored);
    copies = 0;
    doc.forEachPaintNode([&](const DocNode &n) {
        if (n.kind == NodeKind::Ink)
            ++copies;
    });
    CHECK(copies == 0);
    CHECK(!doc.find("I1"));
    CHECK(doc.undo().restored);
    CHECK(doc.find("I1"));
    CHECK(doc.restoreSnapshotQueued() == 0);
}

static void test_clone_grain_parent_plus_descendant()
{
    clipboard().reset();
    DeviceDocument doc;
    JsonValue::Array kids;
    kids.push_back(inkChild("C1", 0, 0, 10, 10));
    kids.push_back(inkChild("C2", 20, 0, 10, 10));
    CHECK(doc.commitJson(createSmartGroupOp("G", 0, 0, 80, 60, JsonValue::array(std::move(kids))))
              .applied);
    copyToSlot(doc, {"G", "C1"}, clipboard());
    const auto roots = slotRoots(doc, {"G", "C1"});
    CHECK(roots.size() == 1);
    CHECK(roots[0] == "G");
    CHECK(clipboard().nodes.size() == 1);
    CHECK(clipboard().nodes[0].id == "G");
    CHECK(clipboard().nodes[0].children.size() == 1);
    CHECK(clipboard().nodes[0].children[0].id == "C1");
}

static void test_paste_parent_20_percent_frame()
{
    clipboard().reset();
    DeviceDocument doc;
    CHECK(doc.commitJson(appendInkOp("I1", 0, 0, 40, 30)).applied);
    CHECK(doc.commitJson(createFrameOp("F", 90, 190, 80, 80)).applied);
    copyToSlot(doc, {"I1"}, clipboard());
    const auto out = commitPaste(doc, clipboard(), 100, 200, "F", false);
    CHECK(out.result.applied);
    std::string pastedId;
    doc.forEachPaintNode([&](const DocNode &n) {
        if (n.kind == NodeKind::Ink && n.id != "I1")
            pastedId = n.id;
    });
    CHECK(!pastedId.empty());
    DeviceDocument::NodePlace pl;
    CHECK(doc.findPlace(pastedId, &pl));
    CHECK(pl.parentId == "F");
}

static void test_copy_tap_selected_ink_box_keeps_children()
{
    clipboard().reset();
    DeviceDocument doc;
    JsonValue::Array kids;
    kids.push_back(inkChild("C", 0, 0, 40, 30));
    CHECK(doc.commitJson(createSmartGroupOp("G", 0, 0, 80, 60, JsonValue::array(std::move(kids))))
              .applied);
    copyToSlot(doc, {"G"}, clipboard());
    CHECK(clipboard().nodes.size() == 1);
    CHECK(clipboard().nodes[0].id == "G");
    CHECK(clipboard().nodes[0].children.size() == 1);
    CHECK(clipboard().nodes[0].children[0].id == "C");
}

static void test_paste_free_ink_into_ink_box()
{
    clipboard().reset();
    DeviceDocument doc;
    CHECK(doc.commitJson(appendInkOp("I1", 0, 0, 40, 30)).applied);
    JsonValue::Array kids;
    CHECK(doc.commitJson(createSmartGroupOp("G", 80, 180, 80, 80, JsonValue::array(std::move(kids))))
              .applied);
    copyToSlot(doc, {"I1"}, clipboard());
    const auto out = commitPaste(doc, clipboard(), 100, 200, "G", false);
    CHECK(out.result.applied);
    CHECK(out.refuse == PasteRefuse::None);
    std::string pastedId;
    doc.forEachPaintNode([&](const DocNode &n) {
        if (n.kind == NodeKind::Ink && n.id != "I1")
            pastedId = n.id;
    });
    CHECK(!pastedId.empty());
    DeviceDocument::NodePlace pl;
    CHECK(doc.findPlace(pastedId, &pl));
    CHECK(pl.parentId == "G");
    SmartBounds childLocal;
    CHECK(epaper::document::nodeWorldAabb(*doc.find(pastedId), childLocal));
    SmartBounds boxWorld;
    CHECK(epaper::document::nodeWorldAabb(*doc.find("G"), boxWorld));
    SmartBounds dirty;
    CHECK(epaper::document::nodeInvalidateAabb(doc, pastedId, dirty));
    CHECK(near(dirty.x, boxWorld.x));
    CHECK(near(dirty.y, boxWorld.y));
    CHECK(near(dirty.width, boxWorld.width));
    CHECK(near(dirty.height, boxWorld.height));
    CHECK(!near(childLocal.x, boxWorld.x) || !near(childLocal.y, boxWorld.y));
}

static void test_paste_copied_ink_box_onto_canvas()
{
    clipboard().reset();
    DeviceDocument doc;
    JsonValue::Array kids;
    kids.push_back(inkChild("C", 0, 0, 40, 30));
    CHECK(doc.commitJson(createSmartGroupOp("G", 0, 0, 80, 60, JsonValue::array(std::move(kids))))
              .applied);
    copyToSlot(doc, {"G"}, clipboard());
    const std::size_t depth = doc.undoDepth();
    const auto out = commitPaste(doc, clipboard(), 200, 100, "", false);
    CHECK(out.result.applied);
    CHECK(doc.find("G"));
    CHECK(doc.find("C"));
    int groups = 0;
    doc.forEachPaintNode([&](const DocNode &n) {
        if (n.kind == NodeKind::SmartGroup)
            ++groups;
    });
    CHECK(groups == 2);
    CHECK(doc.undoDepth() == depth + 1);
}

static void test_hold_still_travel_threshold()
{
    CHECK(!holdTravelExceeded(0, 0));
    CHECK(!holdTravelExceeded(8.0, 0));
    CHECK(holdTravelExceeded(10.0, 0));
}

static void test_paste_chrome_hidden_while_transforming()
{
    SelectionContext sel;
    sel.setPasteOrigin(10, 20, 100, 200);
    CHECK(sel.pasteChromeVisible(true));

    sel.setIds({"N1"});
    CHECK(sel.phase() == SelectionPhase::Selected);
    CHECK(sel.pasteChromeVisible(true));

    sel.setPhase(SelectionPhase::Transforming);
    CHECK(!sel.pasteChromeVisible(true));

    sel.setPhase(SelectionPhase::Selecting);
    CHECK(!sel.pasteChromeVisible(true));

    sel.setPhase(SelectionPhase::Selected);
    CHECK(sel.pasteChromeVisible(true));
    CHECK(!sel.pasteChromeVisible(false));

    sel.clear();
    sel.setPasteOrigin(10, 20, 100, 200);
    CHECK(sel.phase() == SelectionPhase::Idle);
    CHECK(sel.pasteChromeVisible(true));

    sel.setIds({"N1"});
    sel.setPhase(SelectionPhase::Transforming);
    CHECK(!sel.pasteChromeVisible(true));
}

int main()
{
    test_copy_does_not_push_undo();
    test_cut_removes_roots_leaves_empty_group();
    test_paste_press_origin_no_wire();
    test_empty_slot_paste_noop();
    test_paste_onto_live_originals_refused();
    test_cut_then_paste_undo_two_entries();
    test_clone_grain_parent_plus_descendant();
    test_paste_parent_20_percent_frame();
    test_copy_tap_selected_ink_box_keeps_children();
    test_paste_free_ink_into_ink_box();
    test_paste_copied_ink_box_onto_canvas();
    test_hold_still_travel_threshold();
    test_paste_chrome_hidden_while_transforming();
    if (g_fails) {
        std::cerr << g_fails << " failed\n";
        return 1;
    }
    std::cout << "clipboard_test ok\n";
    return 0;
}
