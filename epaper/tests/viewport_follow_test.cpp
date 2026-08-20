/**
 * STORY-EP-055 / viewport-follow-epaper.feature (host, no Qt).
 * @implements [SRS-EP-49] viewport-follow Infini session enum
 * @implements [SRS-EP-50] FollowToggle sibling of ToolChip
 * @implements [SRS-EP-51] follow exclusivity and map-apply quality
 */

#include "document/viewport_follow.hpp"
#include "document/hand_touch.hpp"
#include "toolchip_layout.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>

using namespace epaper::follow;
using epaper::handtouch::FollowDirection;
using epaper::handtouch::OneFingerSession;
using epaper::handtouch::applyLocalPanStart;
using epaper::handtouch::kPalmTravelDu;
using epaper::handtouch::travelPastPalm;
using epaper::toolchip::Hit;
using epaper::toolchip::chipWidth;
using epaper::toolchip::hitAtRelX;
using epaper::toolchip::hitId;
using epaper::toolchip::kHeight;
using epaper::toolchip::kPublish;
using epaper::toolchip::kTile;
using epaper::document::parseJson;

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

static int exclusiveToolCount()
{
    int n = 0;
    if (hitAtRelX(kPublish + 1) == Hit::SelRect)
        ++n;
    if (hitAtRelX(kPublish + kTile + 1) == Hit::SelFreeform)
        ++n;
    if (hitAtRelX(kPublish + kTile * 2 + 1) == Hit::Pen)
        ++n;
    return n;
}

/** Scenario: Enabling Epaper follow from both-off applies Infini viewport */
static void test_enable_from_both_off_applies_infini_viewport()
{
    FollowSession s = liveBothOff();
    CHECK(s.connected);
    CHECK(s.direction == FollowDirection::None);
    CHECK(s.exclusiveTool == "pen");
    CHECK(std::string(kFollowControlId) == "btn.viewport_follow");
    CHECK(std::string(kFollowRegion) == "FollowToggle");
    CHECK(std::string(kToolChipRegion) == "ToolChip");
    CHECK(kFollowTileDu == kTile);
    CHECK(kFollowTileDu >= 64.0);
    CHECK(exclusiveToolCount() == kToolChipExclusiveCount);
    CHECK(exclusiveToolCount() == 3);
    CHECK(std::string(hitId(Hit::Pen)) == "pen");
    CHECK(std::string(hitId(Hit::SelRect)) != kFollowControlId);

    const double panelW = 1872;
    const double panelH = 1404;
    const PanelRect chip{(panelW - chipWidth()) * 0.5, kFollowInsetDu, chipWidth(), kHeight};
    const PanelRect follow = followToggleRect(panelW, panelH, false);
    CHECK(!followIsInsideToolChip(follow, chip));
    CHECK(!follow.intersects(chip));
    CHECK(follow.contains(panelW - kFollowInsetDu - 1, kFollowInsetDu + 1));
    CHECK(!chip.contains(follow.x + 1, follow.y + 1));

    const auto t0 = std::chrono::steady_clock::now();
    const FollowTapResult r = s.tapToggle();
    const auto t1 = std::chrono::steady_clock::now();
    const double applyMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    CHECK(applyMs <= kMapApplyBudgetMs);
    CHECK(s.direction == FollowDirection::InfiniToEpaper);
    CHECK(r.direction == FollowDirection::InfiniToEpaper);
    CHECK(!s.infiniFollowOn());
    CHECK(!r.infiniFollowOn);
    CHECK(s.dualOnIntervals == 0);
    CHECK(r.dualOnIntervals == 0);
    CHECK(s.exclusiveTool == "pen");
    CHECK(s.ariaPressed());
    CHECK(!s.ariaDisabled());
    CHECK(std::string(uiStateId(s.uiState())) == "follow.following_infini");
    CHECK(r.emitted);
    CHECK(s.viewportFollowEmitted == 1);
    CHECK(s.docMessagesFromFollow == 0);
    CHECK(r.appliedInfiniViewport);
    CHECK(s.mapApplied);
    CHECK(near(s.localCamera.minX, 40));
    CHECK(near(s.localCamera.maxX, 180));
    CHECK(s.outbound.size() == 1);
    const auto msg = parseViewportFollowLine(s.outbound.front());
    CHECK(msg.ok);
    CHECK(msg.direction == FollowDirection::InfiniToEpaper);
    CHECK(!isDocType("viewport_follow"));
    CHECK(isViewportFollowType("viewport_follow"));
}

/** Scenario: Tapping Epaper follow while Infini is following takes over */
static void test_tap_while_infini_following_takes_over()
{
    FollowSession s = liveBothOff();
    s.direction = FollowDirection::EpaperToInfini;
    CHECK(!s.ariaPressed());
    CHECK(std::string(uiStateId(s.uiState())) == "follow.peer_following_you");
    CHECK(s.infiniFollowOn());
    CHECK(s.tappable());

    const auto t0 = std::chrono::steady_clock::now();
    const FollowTapResult r = s.tapToggle();
    const auto t1 = std::chrono::steady_clock::now();
    const double exclMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    CHECK(exclMs <= kExclusivityBudgetMs);
    CHECK(s.direction == FollowDirection::InfiniToEpaper);
    CHECK(!s.infiniFollowOn());
    CHECK(!r.infiniFollowOn);
    CHECK(s.dualOnIntervals == 0);
    CHECK(s.ariaPressed());
    CHECK(s.mapApplied);
    CHECK(near(s.localCamera.minX, 40));
}

/** Scenario: Connection lost while following forces follow off */
static void test_connection_lost_forces_follow_off()
{
    FollowSession s = liveBothOff();
    s.tapToggle();
    CHECK(s.direction == FollowDirection::InfiniToEpaper);
    s.onDisconnect();
    CHECK(s.direction == FollowDirection::None);
    CHECK(!s.connected);
    CHECK(s.ariaDisabled());
    CHECK(!s.ariaPressed());
    CHECK(!s.epaperFollowOn());
    CHECK(!s.infiniFollowOn());
    CHECK(std::string(uiStateId(s.uiState())) == "follow.connection_lost");
    CHECK(!s.tappable());
    const FollowTapResult r = s.tapToggle();
    CHECK(!r.emitted);
    CHECK(s.direction == FollowDirection::None);
}

/** Scenario: Reconnect does not restore Epaper follow */
static void test_reconnect_does_not_restore()
{
    FollowSession s = liveBothOff();
    s.tapToggle();
    CHECK(s.direction == FollowDirection::InfiniToEpaper);
    s.onDisconnect();
    CHECK(s.direction == FollowDirection::None);
    s.onReconnect();
    CHECK(s.connected);
    CHECK(s.direction == FollowDirection::None);
    CHECK(!s.restoredOnReconnect);
    CHECK(!s.ariaPressed());
    CHECK(!s.ariaDisabled());
    CHECK(s.tappable());
    CHECK(std::string(uiStateId(s.uiState())) == "follow.reconnect_stays_off");
    CHECK(s.viewportFollowEmitted == 1);
}

/** Scenario: Local pan while following Infini turns Epaper follow off */
static void test_local_pan_while_following_turns_off()
{
    const double thirtySixMmDu = 36.0 / 25.4 * epaper::handtouch::kPanelDpi;
    CHECK(thirtySixMmDu > kPalmTravelDu);
    CHECK(travelPastPalm(thirtySixMmDu, 0));

    FollowSession follow = liveBothOff();
    follow.tapToggle();
    CHECK(follow.direction == FollowDirection::InfiniToEpaper);
    CHECK(follow.tryApplyInboundViewport());
    CHECK(follow.inboundApplied >= 1);
    const int appliedAtFollow = follow.inboundApplied;

    OneFingerSession s;
    s.hit = epaper::handtouch::HitKind::Empty;
    s.follow = follow.direction;
    applyLocalPanStart(s);
    CHECK(s.follow == FollowDirection::None);
    CHECK(s.panApplied);
    follow.direction = s.follow;
    CHECK(!follow.tryApplyInboundViewport());
    CHECK(follow.inboundApplied == appliedAtFollow);
    CHECK(follow.exclusiveTool == "pen");
}

static void test_hello_does_not_carry_follow()
{
    FollowSession s = liveBothOff();
    s.tapToggle();
    s.onDisconnect();
    s.onReconnect();
    CHECK(s.direction == FollowDirection::None);
    for (const std::string &line : s.outbound)
        CHECK(parseJson(line).getString("type") != "hello");
}

int main()
{
    test_enable_from_both_off_applies_infini_viewport();
    test_tap_while_infini_following_takes_over();
    test_connection_lost_forces_follow_off();
    test_reconnect_does_not_restore();
    test_local_pan_while_following_turns_off();
    test_hello_does_not_carry_follow();
    if (g_fails) {
        std::cerr << g_fails << " failed\n";
        return 1;
    }
    std::cout << "viewport_follow_test ok\n";
    return 0;
}
