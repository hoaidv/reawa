/**
 * Phase 3 — SelectionSession host tests (no Qt).
 * Marquee/lasso gesture sizing, tap discard, finish intents.
 */

#include "drawing/selection_session.hpp"

#include <cmath>
#include <iostream>
#include <string>

using namespace epaper::selection;

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

static void test_begin_marquee_intents()
{
    SelectionSession s;
    const SelectionResult r = s.beginMarqueeOrLasso(10, 20, /*freeform=*/false);
    CHECK(s.gesture == Gesture::Marquee);
    CHECK(s.lasso.size() == 1);
    CHECK(has(r.intent, SelectionIntent::ResetDrag));
    CHECK(has(r.intent, SelectionIntent::StrokeWaveformOn));
    CHECK(has(r.intent, SelectionIntent::DamageLive));
    CHECK(r.hasDamage);
}

static void test_begin_lasso()
{
    SelectionSession s;
    s.beginMarqueeOrLasso(0, 0, /*freeform=*/true);
    CHECK(s.gesture == Gesture::Lasso);
}

static void test_update_marquee_and_lasso()
{
    SelectionSession s;
    s.beginMarqueeOrLasso(0, 0, false);
    const SelectionResult m = s.updateMarquee(40, 30);
    CHECK(s.marqueeEnd.x == 40 && s.marqueeEnd.y == 30);
    CHECK(has(m.intent, SelectionIntent::DamageLive));

    SelectionSession l;
    l.beginMarqueeOrLasso(0, 0, true);
    const SelectionResult u = l.updateLasso(5, 5);
    CHECK(l.lasso.size() == 2);
    CHECK(has(u.intent, SelectionIntent::DamageSegment));
}

static void test_finish_tap_discards()
{
    epaper::document::DeviceDocument doc;
    SelectionSession s;
    s.beginMarqueeOrLasso(10, 10, false);
    s.updateMarquee(12, 11); // under min gesture
    const SelectionResult r = s.finish(8.0, doc, [](double px, double py, double *wx, double *wy) {
        *wx = px;
        *wy = py;
    });
    CHECK(s.gesture == Gesture::None);
    CHECK(s.ids.empty());
    CHECK(r.debugInfo.find("tap") != std::string::npos);
    CHECK(has(r.intent, SelectionIntent::StrokeWaveformOff));
    CHECK(has(r.intent, SelectionIntent::RefreshChrome));
}

static void test_finish_rect_empty_doc()
{
    epaper::document::DeviceDocument doc;
    SelectionSession s;
    s.beginMarqueeOrLasso(0, 0, false);
    s.updateMarquee(100, 80);
    const SelectionResult r = s.finish(8.0, doc, [](double px, double py, double *wx, double *wy) {
        *wx = px;
        *wy = py;
    });
    CHECK(s.gesture == Gesture::None);
    CHECK(s.ids.empty());
    CHECK(r.debugInfo.find("no nodes") != std::string::npos);
}

static void test_set_ids_pickable()
{
    SelectionSession s;
    s.setIds({"a", "b"});
    CHECK(s.pickableId == "a");
    s.clear();
    CHECK(s.pickableId.empty());
}

int main()
{
    test_begin_marquee_intents();
    test_begin_lasso();
    test_update_marquee_and_lasso();
    test_finish_tap_discards();
    test_finish_rect_empty_doc();
    test_set_ids_pickable();
    if (g_fails) {
        std::cerr << "selection_session_test: " << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "selection_session_test ok\n";
    return 0;
}
