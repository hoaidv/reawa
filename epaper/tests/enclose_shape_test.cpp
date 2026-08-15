/**
 * Empty-boundary primitive gate (SRS-EP-10).
 */
#include "document/enclose_shape.hpp"
#include "document/recognize_enclose.hpp"
#include "document/recognizer_dispatch.hpp"

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

int main()
{
    CHECK(kMinEncloseWithContent == 28);
    CHECK(kMinEncloseEmpty == 36);

    {
        DeviceDocument doc;
        EncloseStrokeInput stroke;
        stroke.id = "empty_rect";
        stroke.samples = pts({{200, 200}, {300, 200}, {300, 300}, {200, 300}, {200, 200}});
        RecogLatch latch;
        const RecogDispatchResult d = dispatchPenUp(doc, stroke, latch);
        CHECK(d.outcome == RecogOutcome::Enclose);
        CHECK(d.encloseWhy.find("shape=") != std::string::npos);
    }
    {
        DeviceDocument doc;
        EncloseStrokeInput stroke;
        stroke.id = "tiny_empty";
        stroke.samples = pts({{0, 0}, {32, 0}, {32, 32}, {0, 32}, {0, 0}});
        RecogLatch latch;
        const RecogDispatchResult d = dispatchPenUp(doc, stroke, latch);
        CHECK(d.outcome == RecogOutcome::Ink);
        CHECK(d.enclose.reason.find("too_small_empty") != std::string::npos);
        CHECK(doc.find("tiny_empty") && doc.find("tiny_empty")->kind == NodeKind::Ink);
    }
    {
        DeviceDocument doc;
        appendInk(doc, "ink_in", pts({{8, 8}, {20, 12}, {14, 24}}));
        EncloseStrokeInput stroke;
        stroke.id = "content_small";
        stroke.samples = pts({{0, 0}, {32, 0}, {32, 32}, {0, 32}, {0, 0}});
        RecogLatch latch;
        const RecogDispatchResult d = dispatchPenUp(doc, stroke, latch);
        CHECK(d.outcome == RecogOutcome::Enclose);
    }
    {
        DeviceDocument doc;
        EncloseStrokeInput stroke;
        stroke.id = "zigzag";
        stroke.samples = pts({{0, 0},
                              {80, 8},
                              {4, 16},
                              {76, 24},
                              {8, 32},
                              {72, 40},
                              {12, 48},
                              {68, 56},
                              {16, 64},
                              {64, 72},
                              {0, 80},
                              {0, 0}});
        RecogLatch latch;
        const RecogDispatchResult d = dispatchPenUp(doc, stroke, latch);
        CHECK(d.outcome == RecogOutcome::Ink);
        CHECK(d.enclose.reason.find("not_primitive") != std::string::npos);
        CHECK(doc.find("zigzag") && doc.find("zigzag")->kind == NodeKind::Ink);
    }
    {
        DeviceDocument doc;
        EncloseStrokeInput stroke;
        stroke.id = "cuspy_square";
        // Perfect square + one mid-edge cusp — hand-drawn boxes always have extra corners.
        stroke.samples = pts({{0, 0}, {50, 0}, {100, 0}, {100, 100}, {0, 100}, {0, 0}});
        RecogLatch latch;
        const RecogDispatchResult d = dispatchPenUp(doc, stroke, latch);
        CHECK(d.outcome == RecogOutcome::Enclose);
    }
    {
        DeviceDocument doc;
        EncloseStrokeInput stroke;
        stroke.id = "rot_diamond";
        stroke.samples = pts({{80, 0}, {160, 80}, {80, 160}, {0, 80}, {80, 0}});
        RecogLatch latch;
        const RecogDispatchResult d = dispatchPenUp(doc, stroke, latch);
        CHECK(d.outcome == RecogOutcome::Enclose);
    }
    {
        std::vector<InkSample> circ;
        for (int i = 0; i <= 24; ++i) {
            const double t = 2.0 * std::acos(-1.0) * double(i) / 24.0;
            InkSample s;
            s.x = 80 + 50 * std::cos(t);
            s.y = 80 + 50 * std::sin(t);
            s.t = double(i);
            circ.push_back(s);
        }
        const EmptyShapeVerdict v = classifyEmptyBoundaryShape(circ, 30, 30, 100, 100);
        CHECK(v.ok);
        CHECK(v.name == "circle" || v.name == "ellipse");
    }

    if (g_fails) {
        std::cerr << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "enclose_shape_test OK\n";
    return 0;
}
