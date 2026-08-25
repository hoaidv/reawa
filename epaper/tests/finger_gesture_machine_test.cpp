/**
 * Phase 4 — FingerGestureMachine host tests (no Qt).
 * One-finger classify/promote/tap; two-finger refuse while following.
 */

#include "drawing/finger_gesture_machine.hpp"

#include <cmath>
#include <iostream>
#include <string>

using namespace epaper::fingergesture;
using epaper::canvasframe::WorldAabb;
using epaper::canvasframe::WorldPt;
using epaper::handtouch::FollowDirection;
using epaper::handtouch::TwoFingerContacts;
using epaper::handtouch::kPalmTravelDu;

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

static WorldAabb unitRegion()
{
    WorldAabb r;
    r.setBox({0, 0, 100, 100});
    return r;
}

static void test_begin_resize_and_move()
{
    FingerGestureMachine m;
    const FingerResult resize =
        m.begin(10, 10, /*knob=*/true, /*box=*/true, unitRegion(), {10, 10});
    CHECK(m.gesture == Kind::Resize);
    CHECK(has(resize.intent, FingerIntent::BeginHandleResize));

    FingerGestureMachine m2;
    const FingerResult move =
        m2.begin(10, 10, /*knob=*/false, /*box=*/true, unitRegion(), {10, 10});
    CHECK(m2.gesture == Kind::Move);
    CHECK(has(move.intent, FingerIntent::ArmSelFreeform));
    CHECK(has(move.intent, FingerIntent::BeginSelectMove));
}

static void test_empty_promote_and_pan()
{
    FingerGestureMachine m;
    m.begin(0, 0, false, false, unitRegion(), {0, 0});
    CHECK(m.gesture == Kind::EmptyPending);

    const FingerResult stay =
        m.update(1, 0, 1, FollowDirection::None, true, 1, 0);
    CHECK(m.gesture == Kind::EmptyPending);
    CHECK(stay.intent == FingerIntent::None);

    const FingerResult pan =
        m.update(kPalmTravelDu + 1, 0, 1, FollowDirection::None, true, 50, 0);
    CHECK(m.gesture == Kind::EmptyPan);
    CHECK(has(pan.intent, FingerIntent::ApplyCameraRegion));
    CHECK(pan.hasRegion);
}

static void test_empty_tap_clears()
{
    FingerGestureMachine m;
    m.begin(5, 5, false, false, unitRegion(), {5, 5});
    const FingerResult r = m.end(5, 5, 5, 5);
    CHECK(m.gesture == Kind::None);
    CHECK(has(r.intent, FingerIntent::ClearSelection));
    CHECK(has(r.intent, FingerIntent::RefreshChrome));
}

static void test_follow_blocks_pan_and_pinch()
{
    FingerGestureMachine m;
    m.begin(0, 0, false, false, unitRegion(), {0, 0});
    const FingerResult blocked =
        m.update(kPalmTravelDu + 10, 0, 1, FollowDirection::InfiniToEpaper, true, 1, 0);
    CHECK(m.gesture == Kind::EmptyPending);
    CHECK(blocked.intent == FingerIntent::None);

    FingerGestureMachine t;
    const FingerResult pinch =
        t.beginTwo(0, 0, 10, 0, unitRegion(), TwoFingerContacts{0, 0, 0.1, 0},
                   FollowDirection::InfiniToEpaper);
    CHECK(!pinch.accepted);
    CHECK(t.gesture == Kind::None);
}

static void test_two_finger_session()
{
    FingerGestureMachine m;
    const TwoFingerContacts uv{0.2, 0.2, 0.8, 0.2};
    const FingerResult b =
        m.beginTwo(20, 20, 80, 20, unitRegion(), uv, FollowDirection::None);
    CHECK(b.accepted);
    CHECK(m.gesture == Kind::TwoFinger);
    CHECK(has(b.intent, FingerIntent::ApplyCameraRegion));

    const FingerResult e = m.endTwo();
    CHECK(m.gesture == Kind::None);
    CHECK(m.lockedUntilLift);
    CHECK(has(e.intent, FingerIntent::LockUntilLift));
    CHECK(has(e.intent, FingerIntent::PublishViewportSettle));
}

static void test_disarmed_refuses()
{
    FingerGestureMachine m;
    m.setArmed(false);
    const FingerResult r = m.begin(0, 0, false, true, unitRegion(), {0, 0});
    CHECK(!r.accepted);
}

int main()
{
    test_begin_resize_and_move();
    test_empty_promote_and_pan();
    test_empty_tap_clears();
    test_follow_blocks_pan_and_pinch();
    test_two_finger_session();
    test_disarmed_refuses();
    if (g_fails) {
        std::cerr << "finger_gesture_machine_test: " << g_fails << " failure(s)\n";
        return 1;
    }
    std::cout << "finger_gesture_machine_test ok\n";
    return 0;
}
