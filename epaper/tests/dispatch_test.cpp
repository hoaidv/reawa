/**
 * STORY-EP-029 / @SRS-EP-10 ADR-0022 dispatch.
 * Maps recognizer-dispatch.feature. Does not retune enclose/membership thresholds.
 */
#include "debuglog/debug_log_format.hpp"
#include "document/device_document.hpp"
#include "document/recognizer_dispatch.hpp"

#include <algorithm>
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
    JsonValue::Object style;
    style.emplace_back("stroke", JsonValue::string("#1C2430"));
    style.emplace_back("strokeWidth", JsonValue::number(2));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(id));
    payload.emplace_back("samples", inkSamplesToJson(samples));
    payload.emplace_back("style", JsonValue::object(std::move(style)));
    DocOp op;
    op.opId = "append_ink:" + id;
    op.type = "append_ink";
    op.payload = JsonValue::object(std::move(payload));
    CHECK(doc.commitOp(op).applied);
}

static EncloseStrokeInput strokeFromFixture(const JsonValue &fix)
{
    const JsonValue *st = fix.get("stroke");
    EncloseStrokeInput in;
    if (!st || !st->isObject())
        return in;
    in.id = st->getString("id");
    in.width = st->has("width") ? st->getNumber("width") : 2;
    const JsonValue *p = st->get("points");
    if (p && p->isArray()) {
        for (size_t i = 0; i < p->asArray().size(); ++i) {
            const JsonValue &pt = p->asArray()[i];
            InkSample s = sampleFromJson(pt);
            if (!s.t)
                s.t = static_cast<double>(i);
            in.samples.push_back(s);
        }
    }
    return in;
}

static void seedDoc(DeviceDocument &doc, const JsonValue &fix)
{
    const JsonValue *ops = fix.get("seedOps");
    if (!ops || !ops->isArray())
        return;
    for (const auto &opj : ops->asArray())
        CHECK(doc.applyOp(opFromJson(opj)).applied);
}

static RecogOutcome expectedDispatch(const char *name, RecogLatch latch)
{
    if (std::string(name) == "pen_armed.json" || !latch.inkBox)
        return RecogOutcome::Ink;
    if (std::string(name) == "successful.json" || std::string(name) == "already_grouped.json"
        || std::string(name) == "no_content.json")
        return RecogOutcome::Enclose;
    return RecogOutcome::Ink;
}

static void test_one_verdict_one_log_line()
{
    DeviceDocument doc;
    appendInk(doc, "ink_in", pts({{60, 60}, {80, 70}, {70, 90}}));
    EncloseStrokeInput stroke;
    stroke.id = "enclose_1";
    stroke.samples = pts({{40, 40}, {160, 40}, {160, 160}, {40, 160}, {40, 40}});
    RecogLatch latch;
    const RecogDispatchResult d = dispatchPenUp(doc, stroke, latch);
    CHECK(d.outcome == RecogOutcome::Enclose);
    const std::string line = epaper::debuglog::formatRecogLog(d.outcomeName(), d.guard, d.encloseWhy);
    CHECK(line.find("[recog] outcome=enclose guard=none") == 0);
    CHECK(line.find("fail=none") != std::string::npos);
    CHECK(line.find("id=enclose_1") != std::string::npos);
}

static void test_d21_fall_through()
{
    DeviceDocument doc;
    JsonValue::Object b;
    b.emplace_back("x", JsonValue::number(0));
    b.emplace_back("y", JsonValue::number(0));
    b.emplace_back("width", JsonValue::number(120));
    b.emplace_back("height", JsonValue::number(120));
    JsonValue::Object t;
    t.emplace_back("x", JsonValue::number(40));
    t.emplace_back("y", JsonValue::number(40));
    t.emplace_back("rotation", JsonValue::number(0));
    t.emplace_back("scaleX", JsonValue::number(1));
    t.emplace_back("scaleY", JsonValue::number(1));
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string("sg_1"));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    payload.emplace_back("transform", JsonValue::object(std::move(t)));
    payload.emplace_back("inkScaleMode", JsonValue::string("fixedInk"));
    payload.emplace_back("children", JsonValue::array({}));
    DocOp op;
    op.opId = "create_smart_group:sg_1";
    op.type = "create_smart_group";
    op.payload = JsonValue::object(std::move(payload));
    CHECK(doc.commitOp(op).applied);

    EncloseStrokeInput stroke;
    stroke.id = "fail_in_box";
    stroke.samples = pts({{50, 50}, {150, 50}, {150, 150}, {50, 150}, {50, 50}});
    RecogLatch latch;
    const RecogDispatchResult d = dispatchPenUp(doc, stroke, latch);
    CHECK(d.outcome == RecogOutcome::Membership);
    CHECK(d.enclose.kind != EncloseKind::Created);
    const DocNode *sg = doc.find("sg_1");
    CHECK(sg);
    bool joined = false;
    for (const auto &c : sg->children)
        if (c.id == "fail_in_box")
            joined = true;
    CHECK(joined);
}

static void test_g4_fixture_replay()
{
    CHECK(kMinEncloseWorld == 28);
    const char *names[] = {"successful.json", "too_small.json", "no_content.json", "pen_armed.json",
                           "already_grouped.json"};
    for (const char *name : names) {
        DeviceDocument doc;
        const JsonValue fix = loadJsonFile(std::string("tests/fixtures/enclose/") + name);
        seedDoc(doc, fix);
        EncloseStrokeInput stroke = strokeFromFixture(fix);
        RecogLatch latch;
        latch.inkBox = std::string(name) != "pen_armed.json";
        const RecogDispatchResult d = dispatchPenUp(doc, stroke, latch);
        const RecogOutcome want = expectedDispatch(name, latch);
        if (d.outcome != want) {
            std::cerr << "FAIL G4 " << name << " got " << d.outcomeName() << "\n";
            ++g_fails;
        }
    }
}

static void test_dispatch_host_latency()
{
    DeviceDocument doc;
    appendInk(doc, "ink_in", pts({{60, 60}, {80, 70}, {70, 90}}));
    std::vector<std::int64_t> ns;
    RecogLatch latch;
    for (int i = 0; i < 40; ++i) {
        DeviceDocument d;
        appendInk(d, "ink_in", pts({{60, 60}, {80, 70}, {70, 90}}));
        EncloseStrokeInput stroke;
        stroke.id = "s" + std::to_string(i);
        stroke.samples = pts({{40, 40}, {160, 40}, {160, 160}, {40, 160}, {40, 40}});
        ns.push_back(dispatchPenUp(d, stroke, latch).ns);
    }
    std::sort(ns.begin(), ns.end());
    const std::int64_t p95 = ns[size_t(0.95 * (ns.size() - 1))];
    CHECK(p95 < 30'000'000); // 30 ms host analog; REQ-01 is paint path
    std::cout << "dispatch host p95=" << p95 << "ns\n";
}

static void test_near_close_large_box_is_closed()
{
    // Human 2026-08-15: start near end on a large box must count as closed.
    // Old AND (gap≤48 ∧ gap/L≤0.15) rejected an 80u gap on a ~1600u path.
    const auto closed = pts({{0, 0}, {400, 0}, {400, 400}, {0, 400}, {0, 80}});
    CHECK(strokeIsClosedIsh(closed));
    const auto open = pts({{0, 0}, {400, 0}});
    CHECK(!strokeIsClosedIsh(open));
}

static void test_empty_closed_box_creates()
{
    DeviceDocument doc;
    EncloseStrokeInput stroke;
    stroke.id = "empty_box";
    stroke.samples = pts({{200, 200}, {300, 200}, {300, 300}, {200, 300}, {200, 200}});
    RecogLatch latch;
    const RecogDispatchResult d = dispatchPenUp(doc, stroke, latch);
    CHECK(d.outcome == RecogOutcome::Enclose);
    CHECK(d.enclose.kind == EncloseKind::Created);
}

int main()
{
    test_one_verdict_one_log_line();
    test_d21_fall_through();
    test_near_close_large_box_is_closed();
    test_empty_closed_box_creates();
    test_g4_fixture_replay();
    test_dispatch_host_latency();
    if (g_fails) {
        std::cerr << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "dispatch_test: OK\n";
    return 0;
}
