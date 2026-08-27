/**
 * Phase 3 — ManipSession host tests (no Qt).
 * begin/apply/commit/abort intents and move/resize geometry.
 */

#include "drawing/tools/manip_session.hpp"

#include <cmath>
#include <iostream>
#include <string>

using namespace epaper::manip;
using epaper::document::ResizeHandle;
using epaper::document::SmartBounds;
using epaper::document::SmartTransform;

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

static void test_handle_from_index()
{
    CHECK(handleFromIndex(0) == ResizeHandle::Nw);
    CHECK(handleFromIndex(4) == ResizeHandle::Se);
    CHECK(handleFromIndex(-1) == ResizeHandle::None);
    CHECK(handleFromIndex(99) == ResizeHandle::None);
}

static void test_begin_intents()
{
    ManipSession m;
    SmartTransform t;
    t.x = 10;
    t.y = 20;
    SmartBounds b;
    b.width = 40;
    b.height = 30;
    const ManipResult r =
        m.begin("n1", ResizeHandle::None, {100, 200}, t, b);
    CHECK(m.active);
    CHECK(m.nodeId == "n1");
    CHECK(has(r.intent, ManipIntent::BeginGesture));
    CHECK(has(r.intent, ManipIntent::ScheduleRasterize));
    CHECK(has(r.intent, ManipIntent::Redraw));
}

static void test_apply_move()
{
    ManipSession m;
    SmartTransform t;
    t.x = 10;
    t.y = 20;
    SmartBounds b;
    b.x = 0;
    b.y = 0;
    b.width = 40;
    b.height = 30;
    m.begin("n1", ResizeHandle::None, {0, 0}, t, b);
    const ManipResult r = m.apply({15, 25}, "withBounds", /*previewDue=*/true);
    CHECK(near(m.liveT.x, 25));
    CHECK(near(m.liveT.y, 45));
    CHECK(has(r.intent, ManipIntent::ApplyLiveGeometry));
    CHECK(has(r.intent, ManipIntent::SendPreview));
    CHECK(has(r.intent, ManipIntent::Redraw));
}

static void test_apply_skips_preview_when_not_due()
{
    ManipSession m;
    SmartTransform t;
    SmartBounds b;
    b.width = 10;
    b.height = 10;
    m.begin("n1", ResizeHandle::None, {0, 0}, t, b);
    const ManipResult r = m.apply({1, 0}, "withBounds", /*previewDue=*/false);
    CHECK(has(r.intent, ManipIntent::ApplyLiveGeometry));
    CHECK(!has(r.intent, ManipIntent::SendPreview));
    CHECK(!has(r.intent, ManipIntent::Redraw));
}

static void test_commit_no_move_aborts()
{
    ManipSession m;
    SmartTransform t;
    SmartBounds b;
    b.width = 10;
    b.height = 10;
    m.begin("n1", ResizeHandle::None, {0, 0}, t, b);
    m.apply({0.5, 0}, "withBounds", false); // under 2.0 du
    const ManipResult r = m.commit();
    CHECK(!r.moved);
    CHECK(has(r.intent, ManipIntent::AbortGesture));
    CHECK(has(r.intent, ManipIntent::ScheduleRasterize));
    CHECK(m.active); // fields stay until canvas reset()
}

static void test_commit_move()
{
    ManipSession m;
    SmartTransform t;
    SmartBounds b;
    b.width = 10;
    b.height = 10;
    m.begin("n1", ResizeHandle::None, {0, 0}, t, b);
    m.apply({10, 0}, "withBounds", false);
    const ManipResult r = m.commit();
    CHECK(r.moved);
    CHECK(!r.resized);
    CHECK(has(r.intent, ManipIntent::CommitTransform));
    CHECK(has(r.intent, ManipIntent::FlushWire));
}

static void test_abort()
{
    ManipSession m;
    SmartTransform t;
    SmartBounds b;
    b.width = 10;
    b.height = 10;
    m.begin("n1", ResizeHandle::None, {0, 0}, t, b);
    const ManipResult r = m.abort();
    CHECK(has(r.intent, ManipIntent::ApplyLiveGeometry));
    CHECK(has(r.intent, ManipIntent::AbortGesture));
    CHECK(has(r.intent, ManipIntent::ScheduleRasterize));
}

int main()
{
    test_handle_from_index();
    test_begin_intents();
    test_apply_move();
    test_apply_skips_preview_when_not_due();
    test_commit_no_move_aborts();
    test_commit_move();
    test_abort();
    if (g_fails) {
        std::cerr << "manip_session_test: " << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "manip_session_test ok\n";
    return 0;
}
