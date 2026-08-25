/**
 * Phase 2 — StrokeCapture host tests (no Qt).
 * begin/append/end intents, origin stale skip, finished-stroke build.
 */

#include "drawing/stroke_capture.hpp"

#include <cmath>
#include <iostream>
#include <string>

using namespace epaper::strokecapture;
using epaper::ingest::OriginGuardAction;

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

static Channels ch(double p = 0.5)
{
    Channels c;
    c.pressure = p;
    return c;
}

static void test_world_stroke_width()
{
    CHECK(near(worldStrokeWidth(0.0), 2.5 * 0.7));
    CHECK(near(worldStrokeWidth(1.0), 2.5));
    CHECK(near(worldStrokeWidth(0.5), 2.5 * 0.85));
}

static void test_begin_emits_and_flushes()
{
    StrokeCapture s;
    s.setPanelHeight(1872);
    s.noteContact(100, 200, 400, 300);
    const StrokeResult r = s.begin(100, 200, ch());
    CHECK(s.active);
    CHECK(s.current.size() == 1);
    CHECK(has(r.intent, StrokeIntent::CancelSettle));
    CHECK(has(r.intent, StrokeIntent::BeginGesture));
    CHECK(has(r.intent, StrokeIntent::LatchChip));
    CHECK(has(r.intent, StrokeIntent::PreviewBegin));
    CHECK(has(r.intent, StrokeIntent::EmitSegment));
    CHECK(has(r.intent, StrokeIntent::FlushInk));
    CHECK(r.hasSegment && near(r.segmentFrom.panelX, 100));
}

static void test_begin_stale_origin_skips_paint()
{
    StrokeCapture s;
    s.setPanelHeight(1872);
    // Raw origin + mapped bottom-left.
    s.noteContact(0, 1872, 0, 0);
    const StrokeResult r = s.begin(0, 1872, ch());
    CHECK(s.active);
    CHECK(has(r.intent, StrokeIntent::BeginGesture));
    CHECK(!has(r.intent, StrokeIntent::EmitSegment));
    CHECK(!has(r.intent, StrokeIntent::PreviewBegin));
    CHECK(!r.hasSegment);
}

static void test_append_and_end_ingest()
{
    StrokeCapture s;
    s.setPanelHeight(1872);
    s.noteContact(10, 20, 100, 100);
    s.begin(10, 20, ch());
    s.noteContact(30, 40, 120, 110);
    const StrokeResult a = s.append(30, 40, ch(), false);
    CHECK(has(a.intent, StrokeIntent::EmitSegment));
    CHECK(near(a.segmentFrom.panelX, 10) && near(a.segmentTo.panelX, 30));
    CHECK(s.current.size() == 2);

    const StrokeResult e = s.end();
    CHECK(!s.active);
    CHECK(has(e.intent, StrokeIntent::IngestReady));
    CHECK(e.hasFinished);
    CHECK(e.finished.samples.size() == 2);
    CHECK(near(e.finished.samples[0].panelX, 10));
    CHECK(near(e.finished.samples[1].panelY, 40));
    CHECK(has(e.intent, StrokeIntent::FlushInk));
    CHECK(has(e.intent, StrokeIntent::ChipPenUp));
    CHECK(s.strokeCount == 1);
}

static void test_end_short_stroke_no_ingest()
{
    StrokeCapture s;
    s.setPanelHeight(1872);
    s.noteContact(10, 20, 100, 100);
    s.begin(10, 20, ch());
    const StrokeResult e = s.end();
    CHECK(!has(e.intent, StrokeIntent::IngestReady));
    CHECK(!e.hasFinished);
    CHECK(has(e.intent, StrokeIntent::AbortGesture));
    CHECK(s.strokeCount == 0);
}

static void test_origin_guard_promote()
{
    StrokeCapture s;
    s.setPanelHeight(1872);
    CHECK(s.guardContact(true, false, false, true) == OriginGuardAction::Discard);
    CHECK(s.awaitingPlausiblePress);
    CHECK(s.guardContact(false, true, false, false) == OriginGuardAction::PromoteToPress);
    CHECK(!s.awaitingPlausiblePress);
}

static void test_flush_due_on_append()
{
    StrokeCapture s;
    s.setPanelHeight(1872);
    s.noteContact(10, 20, 100, 100);
    s.begin(10, 20, ch());
    s.noteContact(12, 22, 101, 101);
    const StrokeResult a = s.append(12, 22, ch(), true);
    CHECK(has(a.intent, StrokeIntent::FlushInk));
    s.noteContact(14, 24, 102, 102);
    const StrokeResult b = s.append(14, 24, ch(), false);
    CHECK(has(b.intent, StrokeIntent::EmitSegment));
    CHECK(!has(b.intent, StrokeIntent::FlushInk));
}

int main()
{
    test_world_stroke_width();
    test_begin_emits_and_flushes();
    test_begin_stale_origin_skips_paint();
    test_append_and_end_ingest();
    test_end_short_stroke_no_ingest();
    test_origin_guard_promote();
    test_flush_due_on_append();
    if (g_fails) {
        std::cerr << "stroke_capture_test: " << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "stroke_capture_test ok\n";
    return 0;
}
