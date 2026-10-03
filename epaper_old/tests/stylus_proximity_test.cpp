/**
 * Pen-near move detection. Qt evdevtablet drops ABS while !BTN_TOUCH.
 * @implements [SRS-EP-56] brush hover follows pen near
 * @implements [SRS-EP-21] pen near
 */

#include "input/stylus_proximity.hpp"

#include <iostream>

using epaper::input::StylusPhase;
using epaper::input::StylusProximityTracker;
using epaper::input::kAbsX;
using epaper::input::kAbsY;
using epaper::input::kBtnToolPen;
using epaper::input::kBtnToolRubber;
using epaper::input::kBtnTouch;
using epaper::input::kEvAbs;
using epaper::input::kEvKey;
using epaper::input::kEvSyn;
using epaper::input::kSynReport;

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

static void syn(StylusProximityTracker *t)
{
    t->feed(kEvSyn, kSynReport, 0);
}

static void test_near_move_without_contact()
{
    StylusProximityTracker t;
    t.setAbsRange(0, 100, 0, 200);
    t.feed(kEvKey, kBtnToolPen, 1);
    t.feed(kEvAbs, kAbsX, 50);
    t.feed(kEvAbs, kAbsY, 40);
    syn(&t);
    CHECK(t.phase() == StylusPhase::Near);
    CHECK(t.hasPosition());
    CHECK(t.shouldEmitHover(false));
    CHECK(!t.shouldEmitHover(true));
    double x = 0;
    double y = 0;
    t.windowPos(200, 400, &x, &y);
    CHECK(x == 100);
    CHECK(y == 80);
}

static void test_contact_is_not_hover()
{
    StylusProximityTracker t;
    t.setAbsRange(0, 10, 0, 10);
    t.feed(kEvKey, kBtnToolPen, 1);
    t.feed(kEvAbs, kAbsX, 3);
    t.feed(kEvAbs, kAbsY, 3);
    t.feed(kEvKey, kBtnTouch, 1);
    syn(&t);
    CHECK(t.phase() == StylusPhase::Contact);
    CHECK(!t.shouldEmitHover(false));
}

static void test_up_into_near_keeps_moving()
{
    StylusProximityTracker t;
    t.setAbsRange(0, 100, 0, 100);
    t.feed(kEvKey, kBtnToolPen, 1);
    t.feed(kEvKey, kBtnTouch, 1);
    t.feed(kEvAbs, kAbsX, 10);
    t.feed(kEvAbs, kAbsY, 10);
    syn(&t);
    CHECK(t.phase() == StylusPhase::Contact);

    t.feed(kEvKey, kBtnTouch, 0);
    t.feed(kEvAbs, kAbsX, 70);
    t.feed(kEvAbs, kAbsY, 20);
    syn(&t);
    CHECK(t.phase() == StylusPhase::Near);
    CHECK(t.shouldEmitHover(false));
    double x = 0;
    double y = 0;
    t.windowPos(100, 100, &x, &y);
    CHECK(x == 70);
    CHECK(y == 20);
}

static void test_leave_proximity()
{
    StylusProximityTracker t;
    t.feed(kEvKey, kBtnToolPen, 1);
    syn(&t);
    CHECK(t.phase() == StylusPhase::Near);
    t.feed(kEvKey, kBtnToolPen, 0);
    syn(&t);
    CHECK(t.phase() == StylusPhase::Away);
    CHECK(!t.shouldEmitHover(false));
}

static void test_eraser_nib_is_near()
{
    StylusProximityTracker t;
    t.feed(kEvKey, kBtnToolRubber, 1);
    t.feed(kEvAbs, kAbsX, 1);
    t.feed(kEvAbs, kAbsY, 1);
    syn(&t);
    CHECK(t.phase() == StylusPhase::Near);
}

int main()
{
    test_near_move_without_contact();
    test_contact_is_not_hover();
    test_up_into_near_keeps_moving();
    test_leave_proximity();
    test_eraser_nib_is_near();
    if (g_fails) {
        std::cerr << "stylus_proximity_test: " << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "stylus_proximity_test OK\n";
    return 0;
}
