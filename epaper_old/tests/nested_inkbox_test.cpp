/**
 * Host tests for nested ink-box render/hit/flatten/reparent.
 * @implements [SRS-EP-75] [SRS-EP-76] [SRS-EP-77]
 * @implements [STORY-EP-074] [STORY-EP-075] [STORY-EP-076] [STORY-EP-077]
 */

#include "document/device_document.hpp"
#include "document/nested_inkbox.hpp"
#include "document/recognize_enclose.hpp"
#include "document/surround_create.hpp"
#include "drawing/tools/clipboard.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace epaper::document;
using epaper::tools::clipboard;
using epaper::tools::clipops::commitPaste;
using epaper::tools::clipops::copyToSlot;

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

static InkSample samp(double x, double y)
{
    InkSample s;
    s.x = x;
    s.y = y;
    return s;
}

static std::vector<InkSample> rectPoly(double x, double y, double w, double h)
{
    return {samp(x, y), samp(x + w, y), samp(x + w, y + h), samp(x, y + h), samp(x, y)};
}

static DocNode makeInk(const std::string &id, const std::vector<InkSample> &samples,
                       const std::string &role)
{
    DocNode n;
    n.id = id;
    n.kind = NodeKind::Ink;
    n.samples = samples;
    n.role = role;
    n.style.strokeWidth = 2;
    return n;
}

static DocNode makeSg(const std::string &id, double tx, double ty, double w, double h,
                      std::vector<DocNode> children)
{
    DocNode sg;
    sg.id = id;
    sg.kind = NodeKind::SmartGroup;
    sg.smartBounds = {0, 0, w, h};
    sg.transform = {tx, ty, 0, 1, 1};
    sg.inkScaleMode = "fixedInk";
    sg.children = std::move(children);
    return sg;
}

static void indexTree(DeviceDocument &doc)
{
    doc.onAcceptedDocLoad(doc.toJSON());
}

static void test_tap_selects_nested_child()
{
    DeviceDocument doc;
    DocNode child = makeSg("child", 20, 20, 40, 40,
                           {makeInk("cb", rectPoly(0, 0, 40, 40), "boundary"),
                            makeInk("cc", {samp(10, 10), samp(20, 20)}, "content")});
    DocNode parent = makeSg("parent", 0, 0, 200, 200,
                            {makeInk("pb", rectPoly(0, 0, 200, 200), "boundary"), std::move(child)});
    doc.rootChildren.push_back(std::move(parent));
    indexTree(doc);

    const DocNode *hitChild = hitTapSmartGroup(doc, 30, 30);
    CHECK(hitChild && hitChild->id == "child");
    const DocNode *hitParent = hitTapSmartGroup(doc, 180, 180);
    CHECK(hitParent && hitParent->id == "parent");

    SmartBounds cb;
    CHECK(composedBoundsOf(doc, *doc.find("child"), cb));
    CHECK(cb.x > 15 && cb.x < 25);
    CHECK(cb.y > 15 && cb.y < 25);
}

static void test_marquee_skips_nested()
{
    DeviceDocument doc;
    DocNode child = makeSg("child", 20, 20, 40, 40,
                           {makeInk("cb", rectPoly(0, 0, 40, 40), "boundary")});
    DocNode parent = makeSg("parent", 0, 0, 200, 200,
                            {makeInk("pb", rectPoly(0, 0, 200, 200), "boundary"), std::move(child)});
    doc.rootChildren.push_back(std::move(parent));
    indexTree(doc);
    SmartBounds rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 200;
    rect.height = 200;
    const auto ids = selectByRect(doc, rect);
    CHECK(std::find(ids.begin(), ids.end(), "parent") != ids.end());
    CHECK(std::find(ids.begin(), ids.end(), "child") == ids.end());
}

static void test_enclose_nests_nonempty()
{
    DeviceDocument doc;
    DocNode inner = makeSg("inner", 30, 30, 40, 40,
                           {makeInk("ib", rectPoly(0, 0, 40, 40), "boundary"),
                            makeInk("ic", {samp(10, 10), samp(12, 12)}, "content")});
    doc.rootChildren.push_back(std::move(inner));
    doc.rootChildren.push_back(makeInk("free", {samp(50, 50), samp(55, 55)}, "content"));
    indexTree(doc);

    EncloseStrokeInput stroke;
    stroke.id = "enclose_n";
    stroke.armedAtPenDown = StrokeArmedTool::InkBox;
    stroke.samples = rectPoly(0, 0, 120, 120);
    const EncloseResult r = commitStrokeWithEncloseRecognition(doc, stroke);
    CHECK(r.kind == EncloseKind::Created);
    const DocNode *sg = doc.find(r.smartGroupId);
    CHECK(sg);
    bool nested = false;
    for (const auto &c : sg->children) {
        if (c.id == "inner" && c.kind == NodeKind::SmartGroup)
            nested = true;
    }
    CHECK(nested);
    CHECK(doc.find("ic"));
}

static void test_enclose_flattens_empty()
{
    DeviceDocument doc;
    DocNode empty = makeSg("letter", 40, 40, 20, 20,
                           {makeInk("lb", rectPoly(0, 0, 20, 20), "boundary")});
    doc.rootChildren.push_back(std::move(empty));
    doc.rootChildren.push_back(makeInk("free", {samp(50, 50), samp(55, 55)}, "content"));
    indexTree(doc);

    EncloseStrokeInput stroke;
    stroke.id = "enclose_e";
    stroke.armedAtPenDown = StrokeArmedTool::InkBox;
    stroke.samples = rectPoly(0, 0, 120, 120);
    const EncloseResult r = commitStrokeWithEncloseRecognition(doc, stroke);
    CHECK(r.kind == EncloseKind::Created);
    CHECK(!doc.find("letter") || doc.find("letter")->kind != NodeKind::SmartGroup
          || doc.find(r.smartGroupId) == doc.find("letter"));
    const DocNode *sg = doc.find(r.smartGroupId);
    CHECK(sg);
    bool flattened = false;
    bool wrapper = false;
    for (const auto &c : sg->children) {
        if (c.id == "letter" && c.kind == NodeKind::SmartGroup)
            wrapper = true;
        if (c.id == "lb" && c.kind == NodeKind::Ink && c.role && *c.role == "content")
            flattened = true;
    }
    CHECK(flattened);
    CHECK(!wrapper);
}

static void test_move_reparent_80()
{
    DeviceDocument doc;
    DocNode a = makeSg("A", 0, 0, 100, 100, {makeInk("ab", rectPoly(0, 0, 100, 100), "boundary")});
    DocNode b = makeSg("B", 200, 0, 100, 100, {makeInk("bb", rectPoly(0, 0, 100, 100), "boundary")});
    DocNode child = makeSg("C", 10, 10, 40, 40, {makeInk("cb", rectPoly(0, 0, 40, 40), "boundary")});
    a.children.push_back(std::move(child));
    doc.rootChildren.push_back(std::move(a));
    doc.rootChildren.push_back(std::move(b));
    indexTree(doc);

    CHECK(chooseMoveParentId(doc, "C") == "A");

    DocNode *c = doc.mutableFind("C");
    CHECK(c);
    c->transform.x = 220;
    c->transform.y = 10;
    CHECK(chooseMoveParentId(doc, "C") == "B");

    c->transform.x = 400;
    c->transform.y = 400;
    CHECK(chooseMoveParentId(doc, "C") == "");
}

static void test_paste_nests_and_flattens()
{
    clipboard().reset();
    DeviceDocument doc;
    DocNode parent = makeSg("P", 0, 0, 200, 200,
                            {makeInk("pb", rectPoly(0, 0, 200, 200), "boundary")});
    DocNode nonempty = makeSg("N", 0, 0, 40, 40,
                              {makeInk("nb", rectPoly(0, 0, 40, 40), "boundary"),
                               makeInk("nc", {samp(5, 5), samp(8, 8)}, "content")});
    DocNode empty = makeSg("E", 0, 0, 20, 20, {makeInk("eb", rectPoly(0, 0, 20, 20), "boundary")});
    doc.rootChildren.push_back(std::move(parent));
    doc.rootChildren.push_back(std::move(nonempty));
    doc.rootChildren.push_back(std::move(empty));
    indexTree(doc);

    copyToSlot(doc, {"N"}, clipboard());
    const auto nest = commitPaste(doc, clipboard(), 40, 40, "P", false);
    CHECK(nest.result.applied);
    int nestedCount = 0;
    const DocNode *p = doc.find("P");
    CHECK(p);
    for (const auto &c : p->children) {
        if (c.kind == NodeKind::SmartGroup)
            ++nestedCount;
    }
    CHECK(nestedCount >= 1);
    const DocNode *nested = hitTapSmartGroup(doc, 50, 50);
    CHECK(nested && nested->id != "P");

    copyToSlot(doc, {"E"}, clipboard());
    const auto flat = commitPaste(doc, clipboard(), 30, 30, "P", false);
    CHECK(flat.result.applied);
    bool emptyWrapperUnderP = false;
    for (const auto &c : doc.find("P")->children) {
        if (c.kind == NodeKind::SmartGroup && isEmptySmartGroup(c))
            emptyWrapperUnderP = true;
    }
    CHECK(!emptyWrapperUnderP);
}

static void test_overflow_not_hittable()
{
    DeviceDocument doc;
    DocNode child = makeSg("child", 80, 80, 20, 20,
                           {makeInk("cb", rectPoly(0, 0, 20, 20), "boundary")});
    DocNode parent = makeSg("parent", 0, 0, 50, 50,
                            {makeInk("pb", rectPoly(0, 0, 50, 50), "boundary"), std::move(child)});
    doc.rootChildren.push_back(std::move(parent));
    indexTree(doc);

    CHECK(!hitTapSmartGroup(doc, 90, 90));
    const DocNode *inside = hitTapSmartGroup(doc, 25, 25);
    CHECK(inside && inside->id == "parent");
}

static void test_overlap_child_still_wins()
{
    DeviceDocument doc;
    DocNode child = makeSg("child", 80, 80, 40, 40,
                           {makeInk("cb", rectPoly(0, 0, 40, 40), "boundary")});
    DocNode parent = makeSg("parent", 0, 0, 100, 100,
                            {makeInk("pb", rectPoly(0, 0, 100, 100), "boundary"), std::move(child)});
    doc.rootChildren.push_back(std::move(parent));
    indexTree(doc);

    const DocNode *overlap = hitTapSmartGroup(doc, 90, 90);
    CHECK(overlap && overlap->id == "child");
    CHECK(!hitTapSmartGroup(doc, 115, 115));
}

static void test_own_transform_only()
{
    DeviceDocument doc;
    DocNode child = makeSg("child", 20, 20, 40, 40,
                           {makeInk("cb", rectPoly(0, 0, 40, 40), "boundary")});
    DocNode parent = makeSg("parent", 5, 7, 200, 200,
                            {makeInk("pb", rectPoly(0, 0, 200, 200), "boundary"), std::move(child)});
    doc.rootChildren.push_back(std::move(parent));
    indexTree(doc);
    const double px = doc.find("parent")->transform.x;
    const double py = doc.find("parent")->transform.y;
    DocNode *c = doc.mutableFind("child");
    c->transform.x += 3;
    CHECK(doc.find("parent")->transform.x == px);
    CHECK(doc.find("parent")->transform.y == py);
}

int main()
{
    test_tap_selects_nested_child();
    test_marquee_skips_nested();
    test_enclose_nests_nonempty();
    test_enclose_flattens_empty();
    test_move_reparent_80();
    test_paste_nests_and_flattens();
    test_own_transform_only();
    test_overflow_not_hittable();
    test_overlap_child_still_wins();
    if (g_fails) {
        std::cerr << "nested_inkbox_test: " << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "nested_inkbox_test OK\n";
    return 0;
}
