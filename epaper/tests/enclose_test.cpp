/**
 * Host tests for STORY-EP-016 / [SRS-EP-10] [SRS-EP-14].
 * Maps enclose-recognition.feature. Shared fixtures under tests/fixtures/enclose/.
 *
 * Build: ./tests/run_device_document_test.sh
 */

#include "document/device_document.hpp"
#include "document/recognize_enclose.hpp"

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

static bool boundsNear(const SmartBounds &a, const SmartBounds &b, double eps = 1.0)
{
    return near(a.x, b.x, eps) && near(a.y, b.y, eps) && near(a.width, b.width, eps)
        && near(a.height, b.height, eps);
}

static std::vector<InkSample> pointsToSamples(const JsonValue &points)
{
    std::vector<InkSample> out;
    if (!points.isArray())
        return out;
    for (size_t i = 0; i < points.asArray().size(); ++i) {
        const JsonValue &p = points.asArray()[i];
        InkSample s = sampleFromJson(p);
        if (!s.t)
            s.t = static_cast<double>(i);
        out.push_back(s);
    }
    return out;
}

static StrokeArmedTool armedFromString(const std::string &s)
{
    if (s == "ink_box")
        return StrokeArmedTool::InkBox;
    if (s == "selection")
        return StrokeArmedTool::Selection;
    return StrokeArmedTool::Pen;
}

static const char *kindName(EncloseKind k)
{
    switch (k) {
    case EncloseKind::Created:
        return "created";
    case EncloseKind::OrdinaryInk:
        return "ordinary_ink";
    case EncloseKind::Skipped:
        return "skipped";
    }
    return "?";
}

static int smartGroupCount(const DeviceDocument &doc)
{
    int n = 0;
    doc.forEachPaintNode([&](const DocNode &node) {
        if (node.kind == NodeKind::SmartGroup)
            ++n;
    });
    return n;
}

static EncloseStrokeInput strokeFromFixture(const JsonValue &fix)
{
    const JsonValue *st = fix.get("stroke");
    EncloseStrokeInput in;
    if (!st || !st->isObject())
        return in;
    in.id = st->getString("id");
    in.width = st->has("width") ? st->getNumber("width") : 2;
    in.armedAtPenDown = armedFromString(st->getString("armed", "pen"));
    const JsonValue *pts = st->get("points");
    if (pts)
        in.samples = pointsToSamples(*pts);
    return in;
}

static void seedDoc(DeviceDocument &doc, const JsonValue &fix)
{
    const JsonValue *ops = fix.get("seedOps");
    if (!ops || !ops->isArray())
        return;
    for (const auto &opj : ops->asArray()) {
        const ApplyResult r = doc.applyJson(opj);
        if (!r.applied) {
            std::cerr << "seed op failed: " << r.reason << "\n";
            ++g_fails;
        }
    }
}

static EncloseResult runFixture(DeviceDocument &doc, const JsonValue &fix)
{
    seedDoc(doc, fix);
    return commitStrokeWithEncloseRecognition(doc, strokeFromFixture(fix));
}

static JsonValue loadEnclose(const char *name)
{
    return loadJsonFile(std::string("tests/fixtures/enclose/") + name);
}

static void assertFitted(const EncloseResult &r, const JsonValue &fix)
{
    const JsonValue *exp = fix.get("expected");
    CHECK(exp && exp->isObject());
    const JsonValue *fb = exp->get("fittedBounds");
    CHECK(fb && fb->isObject());
    SmartBounds want;
    want.x = fb->getNumber("x");
    want.y = fb->getNumber("y");
    want.width = fb->getNumber("width");
    want.height = fb->getNumber("height");
    CHECK(boundsNear(r.fittedWorldBounds, want, 1.0));
}

/** Shared enclose/ fixtures: verdict + fitted bounds. @SRS-EP-14 */
static void test_shared_enclose_fixtures()
{
    const char *names[] = {"successful.json", "too_small.json", "no_content.json", "pen_armed.json",
                           "already_grouped.json"};
    for (const char *name : names) {
        DeviceDocument doc;
        const JsonValue fix = loadEnclose(name);
        const EncloseResult r = runFixture(doc, fix);
        const std::string want = fix.get("expected")->getString("verdict");
        if (kindName(r.kind) != want) {
            std::cerr << "FAIL fixture " << name << " verdict got " << kindName(r.kind)
                      << " want " << want << " reason " << r.reason << "\n";
            ++g_fails;
        }
        assertFitted(r, fix);
        const JsonValue *exp = fix.get("expected");
        if (exp->has("reason") && !exp->getString("reason").empty())
            CHECK(r.reason == exp->getString("reason"));
    }
}

/** @SRS-EP-10 Successful enclose commits Smart Group immediately and locally */
static void test_successful_enclose_local()
{
    DeviceDocument doc;
    const JsonValue fix = loadEnclose("successful.json");
    const std::size_t q0 = doc.publishQueue().size();
    const auto t0 = std::chrono::steady_clock::now();
    const EncloseResult r = runFixture(doc, fix);
    const auto t1 = std::chrono::steady_clock::now();
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    CHECK(r.kind == EncloseKind::Created);
    CHECK(!r.smartGroupId.empty());
    CHECK(doc.find(r.smartGroupId));
    CHECK(doc.find(r.smartGroupId)->kind == NodeKind::SmartGroup);
    CHECK(smartGroupCount(doc) == 1);

    const DocNode *sg = doc.find(r.smartGroupId);
    CHECK(sg);
    CHECK(near(sg->smartBounds.x, 0));
    CHECK(near(sg->smartBounds.y, 0));
    CHECK(near(sg->smartBounds.width, 120));
    CHECK(near(sg->smartBounds.height, 120));
    CHECK(near(sg->transform.x, 40));
    CHECK(near(sg->transform.y, 40));
    CHECK(sg->inkScaleMode == "fixedInk");
    CHECK(sg->boundaryPolyline.size() >= 2);

    const DocNode *boundary = nullptr;
    const DocNode *content = nullptr;
    for (const auto &c : sg->children) {
        if (c.role && *c.role == "boundary")
            boundary = &c;
        if (c.role && *c.role == "content")
            content = &c;
    }
    CHECK(boundary && boundary->id == "enclose_1");
    CHECK(content && content->id == "ink_in");
    CHECK(content->layoutOffset.has_value());
    CHECK(content->layoutOffset->first > 0 && content->layoutOffset->first < 1);
    CHECK(content->layoutOffset->second > 0 && content->layoutOffset->second < 1);
    CHECK(!doc.find("enclose_1") || doc.find("enclose_1") == boundary);
    CHECK(!doc.rootChildren.empty());
    CHECK(doc.rootChildren[0].id == r.smartGroupId);

    CHECK(doc.publishQueue().size() == q0 + 1);
    CHECK(doc.publishQueue().back().op.getString("type") == "create_smart_group");
    CHECK(ns <= 500000000); // p95 ≤500 ms host analog — not device p95
}

/** @SRS-EP-10 Stroke drawn with Pen armed skips recognition */
static void test_pen_armed_skips()
{
    DeviceDocument doc;
    const EncloseResult r = runFixture(doc, loadEnclose("pen_armed.json"));
    CHECK(r.kind == EncloseKind::OrdinaryInk);
    CHECK(r.reason == "pen_armed");
    CHECK(smartGroupCount(doc) == 0);
    CHECK(doc.find("pen_1") && doc.find("pen_1")->kind == NodeKind::Ink);
    CHECK(!doc.find("pen_1")->role || *doc.find("pen_1")->role != "boundary");
    CHECK(doc.find("ink_in"));
}

/** @SRS-EP-10 Tool switched mid-stroke does not change what the stroke means */
static void test_latch_ignores_live_tool()
{
    DeviceDocument doc;
    seedDoc(doc, loadEnclose("successful.json"));
    doc.applyToolSwitch("pen");
    EncloseStrokeInput stroke = strokeFromFixture(loadEnclose("successful.json"));
    stroke.armedAtPenDown = StrokeArmedTool::InkBox;
    const EncloseResult r = commitStrokeWithEncloseRecognition(doc, stroke);
    CHECK(r.kind == EncloseKind::Created);
    CHECK(doc.uiTool() == "pen");
    CHECK(smartGroupCount(doc) == 1);
}

/** @SRS-EP-10 Failed guards leave ordinary ink (0 banner) */
static void test_failed_guards_ordinary_ink()
{
    {
        DeviceDocument doc;
        const EncloseResult r = runFixture(doc, loadEnclose("too_small.json"));
        CHECK(r.kind == EncloseKind::OrdinaryInk);
        CHECK(r.reason == "too_small");
        CHECK(smartGroupCount(doc) == 0);
        CHECK(doc.find("tiny") && doc.find("tiny")->kind == NodeKind::Ink);
    }
    {
        DeviceDocument doc;
        const EncloseResult r = runFixture(doc, loadEnclose("no_content.json"));
        CHECK(r.kind == EncloseKind::Created);
        CHECK(smartGroupCount(doc) == 1);
        CHECK(doc.find("empty_box"));
    }
}

/** @SRS-EP-10 Ink already inside a Smart Group is skipped */
static void test_already_grouped_skipped()
{
    DeviceDocument doc;
    const EncloseResult r = runFixture(doc, loadEnclose("already_grouped.json"));
    CHECK(r.kind == EncloseKind::Created);
    CHECK(doc.find("sg_old"));
    CHECK(doc.find("already"));
    const DocNode *sg = doc.find(r.smartGroupId);
    CHECK(sg);
    std::vector<std::string> ids;
    for (const auto &c : sg->children)
        ids.push_back(c.id);
    CHECK(ids.size() == 2);
    CHECK((ids[0] == "enclose_mix" && ids[1] == "free") || (ids[0] == "free" && ids[1] == "enclose_mix"));
    bool hasAlready = false;
    for (const auto &id : ids)
        if (id == "already")
            hasAlready = true;
    CHECK(!hasAlready);
}

/** @SRS-EP-10 Enclose works with the session down */
static void test_session_down_queues_create()
{
    DeviceDocument doc;
    doc.clearPublishQueue();
    const EncloseResult r = runFixture(doc, loadEnclose("successful.json"));
    CHECK(r.kind == EncloseKind::Created);
    CHECK(smartGroupCount(doc) == 1);
    CHECK(doc.publishQueue().size() == 1);
    CHECK(doc.publishQueue().back().op.getString("type") == "create_smart_group");
    CHECK(doc.publishQueue().back().op.getString("type") != "recognize_enclose");
}

/** @SRS-EP-10 Consecutive encloses stay correct (CHL-0007) */
static void test_ten_consecutive_encloses()
{
    DeviceDocument doc;
    for (int i = 0; i < 10; ++i) {
        const double ox = static_cast<double>(i) * 200.0;
        JsonValue::Object seedPayload;
        JsonValue::Array samples;
        {
            JsonValue::Object a;
            a.emplace_back("x", JsonValue::number(60 + ox));
            a.emplace_back("y", JsonValue::number(60));
            a.emplace_back("t", JsonValue::number(0));
            samples.push_back(JsonValue::object(std::move(a)));
        }
        {
            JsonValue::Object b;
            b.emplace_back("x", JsonValue::number(80 + ox));
            b.emplace_back("y", JsonValue::number(90));
            b.emplace_back("t", JsonValue::number(1));
            samples.push_back(JsonValue::object(std::move(b)));
        }
        JsonValue::Object style;
        style.emplace_back("stroke", JsonValue::string("#1C2430"));
        style.emplace_back("strokeWidth", JsonValue::number(2));
        seedPayload.emplace_back("id", JsonValue::string(std::string("ink_") + std::to_string(i)));
        seedPayload.emplace_back("samples", JsonValue::array(std::move(samples)));
        seedPayload.emplace_back("style", JsonValue::object(std::move(style)));
                CHECK(doc.applyJson(opEnvelope(std::string("seed_") + std::to_string(i), "append_ink", JsonValue::object(std::move(seedPayload)), "epaper")).applied);

        EncloseStrokeInput stroke;
        stroke.id = std::string("enclose_") + std::to_string(i);
        stroke.armedAtPenDown = StrokeArmedTool::InkBox;
        stroke.width = 2;
        const double xs[5] = {40 + ox, 160 + ox, 160 + ox, 40 + ox, 40 + ox};
        const double ys[5] = {40, 40, 160, 160, 40};
        for (int k = 0; k < 5; ++k) {
            InkSample s;
            s.x = xs[k];
            s.y = ys[k];
            s.t = static_cast<double>(k);
            stroke.samples.push_back(s);
        }
        const EncloseResult r = commitStrokeWithEncloseRecognition(doc, stroke);
        CHECK(r.kind == EncloseKind::Created);
    }
    CHECK(smartGroupCount(doc) == 10);
    CHECK(doc.undoDepth() == 10);
}

/** @SRS-EP-10 Undo restores the pre-create tree */
static void test_undo_restores_pre_create()
{
    DeviceDocument doc;
    seedDoc(doc, loadEnclose("successful.json"));
    const std::string before = doc.snapshotString();
    const EncloseResult r = commitStrokeWithEncloseRecognition(doc, strokeFromFixture(loadEnclose("successful.json")));
    CHECK(r.kind == EncloseKind::Created);
    CHECK(doc.undoDepth() == 1);
    CHECK(doc.newestEntry() && doc.newestEntry()->forwardOpId.find("create_smart_group") != std::string::npos);
    const UndoResult u = doc.undo();
    CHECK(u.restored);
    CHECK(doc.snapshotString() == before);
    CHECK(smartGroupCount(doc) == 0);
    CHECK(doc.find("ink_in"));
    CHECK(!doc.find("enclose_1") || doc.find("enclose_1")->kind != NodeKind::SmartGroup);
}

int main()
{
    test_shared_enclose_fixtures();
    test_successful_enclose_local();
    test_pen_armed_skips();
    test_latch_ignores_live_tool();
    test_failed_guards_ordinary_ink();
    test_already_grouped_skipped();
    test_session_down_queues_create();
    test_ten_consecutive_encloses();
    test_undo_restores_pre_create();

    if (g_fails) {
        std::cerr << g_fails << " check(s) failed\n";
        return 1;
    }
    std::cout << "enclose_test: checks passed\n";
    return 0;
}
