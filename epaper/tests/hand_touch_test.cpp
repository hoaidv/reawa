/**
 * STORY-EP-038 / hand-touch-one-finger.feature and
 * STORY-EP-039 / hand-touch-two-finger.feature (host, no Qt).
 * @implements [SRS-EP-21] one-finger pick move palm pan
 * @implements [SRS-EP-23] finger exclusive-tool switch
 * @implements [SRS-EP-24] two-finger pan pinch viewport
 * @implements [SRS-EP-25] one-finger hand-touch quality
 * @implements [SRS-EP-26] two-finger map-apply quality
 */

#include "document/hand_touch.hpp"
#include "document/manipulate.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>

using namespace epaper::handtouch;

static int g_fails = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond << "\n";       \
            ++g_fails;                                                                         \
        }                                                                                      \
    } while (0)

static bool near(double a, double b, double eps = 1e-4)
{
    return std::abs(a - b) <= eps;
}

/** Scenario: Finger-down inside a box arms sel_freeform and selects */
static void test_finger_down_box_select_freeform()
{
    CHECK(classifyHit(false, false, true) == HitKind::Box);
    CHECK(actionOnDown(HitKind::Box) == FingerAction::SelectMove);
    CHECK(switchesToSelFreeform(HitKind::Box));
    CHECK(!switchesToSelFreeform(HitKind::Knob));
    CHECK(!switchesToSelFreeform(HitKind::Empty));
    CHECK(!switchesToSelFreeform(HitKind::Chip));
}

/** Scenario: Finger drag inside the selected box moves live-direct with 0 pan */
static void test_finger_drag_box_zero_pan()
{
    OneFingerSession s;
    s.hit = HitKind::Box;
    s.follow = FollowDirection::None;
    s.action = actionOnDown(s.hit);
    CHECK(s.action == FingerAction::SelectMove);
    CHECK(!travelPastPalm(40, 30));
    maybePublishViewport(s);
    CHECK(s.viewportUp == 0);
    CHECK(!s.panApplied);
    CHECK(s.follow == FollowDirection::None);
}

/** Scenario: Finger on a resize knob resizes live-direct with 0 pan */
static void test_finger_knob_resize_zero_pan()
{
    CHECK(classifyHit(false, true, true) == HitKind::Knob);
    CHECK(actionOnDown(HitKind::Knob) == FingerAction::Resize);
    CHECK(!switchesToSelFreeform(HitKind::Knob));
    CHECK(kFingerHandleHitDu >= 64.0);
    CHECK(kFingerHandleHitDu >= epaper::document::kHandleHitDu);
    OneFingerSession s;
    s.hit = HitKind::Knob;
    s.action = FingerAction::Resize;
    s.follow = FollowDirection::InfiniToEpaper;
    maybePublishViewport(s);
    CHECK(s.viewportUp == 0);
    CHECK(s.follow == FollowDirection::InfiniToEpaper);
}

/** Scenario: One-finger empty travel at or below 10 mm is palm-rest */
static void test_empty_palm_rest()
{
    const double eightMmDu = 8.0 / 25.4 * kPanelDpi;
    CHECK(eightMmDu <= kPalmTravelDu);
    CHECK(actionOnEmptyMove(eightMmDu) == FingerAction::PalmRest);
    CHECK(actionOnEmptyMove(kPalmTravelDu) == FingerAction::PalmRest);
    CHECK(!travelPastPalm(eightMmDu, 0));
    CHECK(classifyHit(false, false, false) == HitKind::Empty);
    CHECK(actionOnDown(HitKind::Empty) == FingerAction::PalmRest);
    OneFingerSession s;
    s.hit = HitKind::Empty;
    s.action = FingerAction::PalmRest;
    maybePublishViewport(s);
    CHECK(s.viewportUp == 0);
    CHECK(!s.panApplied);
}

/** Scenario: One-finger empty tap deselects */
static void test_empty_tap_clears_selection()
{
    CHECK(emptyTapClearsSelection(0));
    CHECK(emptyTapClearsSelection(kPalmTravelDu));
    CHECK(!emptyTapClearsSelection(kPalmTravelDu + 1));
}

/** Scenario: Pen proximity or contact disables canvas hand-touch */
static void test_pen_proximity_disables_hand_touch()
{
    CHECK(handTouchEnabled(false, false));
    CHECK(!handTouchEnabled(true, false));
    CHECK(!handTouchEnabled(false, true));
    CHECK(!handTouchEnabled(true, true));
}

/** Scenario: One-finger empty travel past 10 mm pans locally with Infini unchanged */
static void test_empty_pan_local_infini_unchanged()
{
    const double thirtySixMmDu = 36.0 / 25.4 * kPanelDpi;
    CHECK(thirtySixMmDu > kPalmTravelDu);
    CHECK(actionOnEmptyMove(thirtySixMmDu) == FingerAction::LocalPan);
    CHECK(travelPastPalm(thirtySixMmDu, 0));

    OneFingerSession s;
    s.hit = HitKind::Empty;
    s.follow = FollowDirection::None;
    s.region = {0, 0, 100, 80};
    applyLocalPanStart(s);
    s.region = panKeepWorldUnderFinger(s.region, 50, 40, 70, 40);
    maybePublishViewport(s);
    CHECK(s.action == FingerAction::LocalPan);
    CHECK(s.panApplied);
    CHECK(s.follow == FollowDirection::None);
    CHECK(s.viewportUp == 0);
    CHECK(near(s.region.minX, -20));
    CHECK(near(s.region.maxX, 80));
}

/** Scenario: ToolChip 64 du tap still holds REQ-03 */
static void test_chip_wins_over_empty_pan()
{
    CHECK(classifyHit(true, true, true) == HitKind::Chip);
    CHECK(actionOnDown(HitKind::Chip) == FingerAction::ChipTap);
    CHECK(classifyHit(true, false, false) == HitKind::Chip);
    OneFingerSession s;
    s.hit = HitKind::Chip;
    s.action = FingerAction::ChipTap;
    CHECK(!s.panApplied);
}

/** Scenario: One-finger empty pan while following Infini turns Epaper follow off */
static void test_follower_local_nav_turns_follow_off_then_pans()
{
    OneFingerSession s;
    s.hit = HitKind::Empty;
    s.follow = FollowDirection::InfiniToEpaper;
    CHECK(tryApplyInboundViewport(s));
    CHECK(s.inboundApplied == 1);

    applyLocalPanStart(s);
    CHECK(s.follow == FollowDirection::None);
    CHECK(s.panApplied);
    CHECK(!tryApplyInboundViewport(s));
    CHECK(s.inboundApplied == 1);
    maybePublishViewport(s);
    CHECK(s.viewportUp == 0);
}

static void test_publish_only_if_infini_follow_on()
{
    OneFingerSession s;
    s.follow = FollowDirection::EpaperToInfini;
    applyLocalPanStart(s);
    CHECK(s.follow == FollowDirection::EpaperToInfini);
    maybePublishViewport(s);
    CHECK(s.viewportUp == 1);
}

static void test_hit_priority_box_knob_chip()
{
    CHECK(classifyHit(false, false, true) == HitKind::Box);
    CHECK(classifyHit(false, true, true) == HitKind::Knob);
    CHECK(classifyHit(true, false, true) == HitKind::Chip);
}

static void test_follow_parse()
{
    CHECK(parseFollow("none") == FollowDirection::None);
    CHECK(parseFollow("infini_to_epaper") == FollowDirection::InfiniToEpaper);
    CHECK(parseFollow("epaper_to_infini") == FollowDirection::EpaperToInfini);
    CHECK(std::string(followId(FollowDirection::None)) == "none");
}

static TwoFingerSession makeTwoFingerIdle()
{
    TwoFingerSession s;
    s.follow = FollowDirection::None;
    s.region = {0, 0, 100, 80};
    s.origin = s.region;
    return s;
}

static TwoFingerContacts pinchClosed()
{
    return {0.4, 0.5, 0.6, 0.5};
}

static TwoFingerContacts pinchSpread()
{
    return {0.3, 0.5, 0.7, 0.5};
}

static TwoFingerContacts panLeft(const TwoFingerContacts &from, double du)
{
    TwoFingerContacts t = from;
    t.u0 -= du;
    t.u1 -= du;
    return t;
}

/** Scenario: Two-finger pan for 5 s applies the local map before the next pen sample */
static void test_two_finger_pan_map_before_pen()
{
    TwoFingerSession s = makeTwoFingerIdle();
    const TwoFingerContacts from = pinchClosed();
    CHECK(tryStartTwoFinger(s, from));
    const auto t0 = std::chrono::steady_clock::now();
    applyTwoFingerStep(s, panLeft(from, 0.1));
    const auto t1 = std::chrono::steady_clock::now();
    const double applyMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    CHECK(applyMs <= kTwoFingerMapApplyBudgetMs);
    CHECK(s.panApplied);
    CHECK(near(s.scaleX, s.scaleY));
    CHECK(s.rotation == 0);
    CHECK(s.skew == 0);
    CHECK(s.region.minX > s.origin.minX);
    double originWx = 0, originWy = 0, nowWx = 0, nowWy = 0;
    mapUvToWorld(s.origin, 0.5, 0.5, &originWx, &originWy);
    mapUvToWorld(s.region, 0.5, 0.5, &nowWx, &nowWy);
    CHECK(!near(nowWx, originWx));
    maybePublishTwoFinger(s, true);
    CHECK(s.viewportUp == 0);
    CHECK(s.follow == FollowDirection::None);
}

/** Scenario: Two-finger pinch scales uniformly with the same map-apply bar */
static void test_two_finger_pinch_uniform_scale()
{
    TwoFingerSession s = makeTwoFingerIdle();
    CHECK(tryStartTwoFinger(s, pinchClosed()));
    const auto t0 = std::chrono::steady_clock::now();
    applyTwoFingerStep(s, pinchSpread());
    const auto t1 = std::chrono::steady_clock::now();
    const double applyMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    CHECK(applyMs <= kTwoFingerMapApplyBudgetMs);
    CHECK(near(s.scaleX, s.scaleY));
    CHECK(s.scaleX > 1.0);
    CHECK(s.rotation == 0);
    CHECK(s.skew == 0);
    const double ow = s.origin.maxX - s.origin.minX;
    const double nw = s.region.maxX - s.region.minX;
    CHECK(nw < ow);
    maybePublishTwoFinger(s, true);
    CHECK(s.viewportUp == 0);
}

/** Scenario: Infini follow off leaves Infini's camera unchanged */
static void test_two_finger_follow_none_zero_viewport_up()
{
    TwoFingerSession s = makeTwoFingerIdle();
    CHECK(tryStartTwoFinger(s, pinchClosed()));
    applyTwoFingerStep(s, panLeft(pinchClosed(), 0.1));
    maybePublishTwoFinger(s, false);
    maybePublishTwoFinger(s, true);
    CHECK(s.viewportUp == 0);
    CHECK(s.viewportDown == 0);
    CHECK(s.follow == FollowDirection::None);
}

/** Scenario: Infini follow on publishes the local viewport from the tablet */
static void test_two_finger_follow_on_publishes_viewport_up()
{
    TwoFingerSession s = makeTwoFingerIdle();
    s.follow = FollowDirection::EpaperToInfini;
    CHECK(tryStartTwoFinger(s, pinchClosed()));
    CHECK(s.follow == FollowDirection::EpaperToInfini);
    applyTwoFingerStep(s, panLeft(pinchClosed(), 0.1));
    maybePublishTwoFinger(s, false);
    CHECK(s.viewportUp == 1);
    CHECK(std::string(s.lastUpSource) == "epaper");
    CHECK(near(s.lastPublishedRegion.minX, s.region.minX));
    CHECK(near(s.lastPublishedRegion.maxX, s.region.maxX));
    CHECK(!s.settleOnLastUp);
    maybePublishTwoFinger(s, true);
    CHECK(s.viewportUp == 2);
    CHECK(s.settleOnLastUp);
    CHECK(s.viewportDown == 0);
    CHECK(!tryApplyInboundViewport(s));
}

/** Scenario: Second finger does not start pan while box-move is in flight */
static void test_second_finger_blocked_during_box_move()
{
    TwoFingerSession s = makeTwoFingerIdle();
    s.boxMoveInFlight = true;
    CHECK(!canStartTwoFinger(s));
    CHECK(!tryStartTwoFinger(s, pinchClosed()));
    CHECK(s.action == TwoFingerAction::Blocked);
    CHECK(!s.twoFingerStarted);
    CHECK(!s.panApplied);
    CHECK(s.moveContinued);
    CHECK(near(s.region.minX, 0));
    CHECK(near(s.region.maxX, 100));
}

/** Scenario: Second finger does not start pan while resize is in flight */
static void test_second_finger_blocked_during_resize()
{
    TwoFingerSession s = makeTwoFingerIdle();
    s.resizeInFlight = true;
    CHECK(!tryStartTwoFinger(s, pinchClosed()));
    CHECK(s.action == TwoFingerAction::Blocked);
    CHECK(!s.panApplied);
    CHECK(s.resizeContinued);
    CHECK(near(s.region.minX, 0));
}

/** Scenario: Link down still changes the local viewport */
static void test_two_finger_link_down_local_view()
{
    TwoFingerSession s = makeTwoFingerIdle();
    s.sessionDown = true;
    CHECK(tryStartTwoFinger(s, pinchClosed()));
    applyTwoFingerStep(s, panLeft(pinchClosed(), 0.1));
    CHECK(s.panApplied);
    CHECK(s.region.minX > s.origin.minX);
    maybePublishTwoFinger(s, true);
    CHECK(s.viewportUp == 0);
    CHECK(s.publishQueued);
}

/** Scenario: Two-finger pan while following Infini turns Epaper follow off */
static void test_two_finger_follower_local_nav_turns_follow_off()
{
    TwoFingerSession s = makeTwoFingerIdle();
    s.follow = FollowDirection::InfiniToEpaper;
    CHECK(tryApplyInboundViewport(s));
    CHECK(s.inboundApplied == 1);
    CHECK(tryStartTwoFinger(s, pinchClosed()));
    CHECK(s.follow == FollowDirection::None);
    applyTwoFingerStep(s, panLeft(pinchClosed(), 0.1));
    CHECK(s.panApplied);
    CHECK(!tryApplyInboundViewport(s));
    CHECK(s.inboundApplied == 1);
    CHECK(s.viewportDown == 1);
    maybePublishTwoFinger(s, true);
    CHECK(s.viewportUp == 0);
}

int main()
{
    test_finger_down_box_select_freeform();
    test_finger_drag_box_zero_pan();
    test_finger_knob_resize_zero_pan();
    test_empty_palm_rest();
    test_empty_tap_clears_selection();
    test_pen_proximity_disables_hand_touch();
    test_empty_pan_local_infini_unchanged();
    test_chip_wins_over_empty_pan();
    test_follower_local_nav_turns_follow_off_then_pans();
    test_publish_only_if_infini_follow_on();
    test_hit_priority_box_knob_chip();
    test_follow_parse();
    test_two_finger_pan_map_before_pen();
    test_two_finger_pinch_uniform_scale();
    test_two_finger_follow_none_zero_viewport_up();
    test_two_finger_follow_on_publishes_viewport_up();
    test_second_finger_blocked_during_box_move();
    test_second_finger_blocked_during_resize();
    test_two_finger_link_down_local_view();
    test_two_finger_follower_local_nav_turns_follow_off();
    if (g_fails) {
        std::cerr << g_fails << " failed\n";
        return 1;
    }
    std::cout << "hand_touch_test ok\n";
    return 0;
}
