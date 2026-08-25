#pragma once

/**
 * Finger / palm gesture state machine. Qt-free.
 * Owns one- and two-finger session state; returns FingerIntent for the canvas.
 *
 * Hit geometry (knob/box) and panel⇄world mapping stay at the canvas edge —
 * the machine classifies and transitions from those facts.
 *
 * @implements [SRS-EP-21] one-finger pick move palm pan
 * @implements [SRS-EP-23] finger exclusive-tool switch
 * @implements [SRS-EP-24] two-finger pan pinch viewport
 */

#include "canvas_frame.hpp"
#include "document/hand_touch.hpp"

namespace epaper {
namespace fingergesture {

enum class Kind { None, Chip, Move, Resize, EmptyPending, EmptyPan, TwoFinger };

enum class FingerIntent : int {
    None = 0,
    ArmSelFreeform = 1 << 0,
    BeginHandleResize = 1 << 1,
    BeginSelectMove = 1 << 2,
    UpdateSelection = 1 << 3,
    ApplyCameraRegion = 1 << 4,
    PublishViewportLive = 1 << 5,
    PublishViewportSettle = 1 << 6,
    ScheduleRasterizeLive = 1 << 7,
    ScheduleRasterizeSettle = 1 << 8,
    RefreshChrome = 1 << 9,
    EndSelectionGesture = 1 << 10,
    ClearSelection = 1 << 11,
    AbortManip = 1 << 12,
    LockUntilLift = 1 << 13,
};

inline FingerIntent operator|(FingerIntent a, FingerIntent b)
{
    return static_cast<FingerIntent>(static_cast<int>(a) | static_cast<int>(b));
}
inline FingerIntent &operator|=(FingerIntent &a, FingerIntent b)
{
    a = a | b;
    return a;
}
inline bool has(FingerIntent mask, FingerIntent bit)
{
    return (static_cast<int>(mask) & static_cast<int>(bit)) != 0;
}

struct FingerResult {
    FingerIntent intent = FingerIntent::None;
    /** begin / beginTwo may refuse (disarmed, blocked follow, etc.). */
    bool accepted = true;
    handtouch::WorldAabb region{};
    bool hasRegion = false;
};

inline FingerIntent liveNavIntents(bool previewDue, bool settle)
{
    FingerIntent i = FingerIntent::ApplyCameraRegion;
    if (settle) {
        i |= FingerIntent::PublishViewportSettle | FingerIntent::ScheduleRasterizeSettle
            | FingerIntent::RefreshChrome;
    } else if (previewDue) {
        i |= FingerIntent::PublishViewportLive | FingerIntent::ScheduleRasterizeLive
            | FingerIntent::RefreshChrome;
    }
    return i;
}

struct FingerGestureMachine {
    Kind gesture = Kind::None;
    bool armed = true;
    bool lockedUntilLift = false;

    canvasframe::PanelPt downPanel{};
    canvasframe::WorldPt downWorld{};
    canvasframe::WorldAabb panOrigin{};
    canvasframe::PanelPt twoA{};
    canvasframe::PanelPt twoB{};
    handtouch::TwoFingerContacts twoOrigin{};
    handtouch::TwoFingerContacts twoCurrent{};

    bool isLiveManip() const { return gesture == Kind::Move || gesture == Kind::Resize; }
    bool isTwoFinger() const { return gesture == Kind::TwoFinger; }
    bool ignoresOneFingerUpdate() const
    {
        return gesture == Kind::None || gesture == Kind::Chip || gesture == Kind::TwoFinger;
    }

    void setArmed(bool on)
    {
        armed = on;
        if (!armed)
            lockedUntilLift = false;
    }

    void contactsCleared() { lockedUntilLift = false; }

    /** Pinch start after abort: reset gesture so beginTwo can take over. */
    void clearGestureForPinch() { gesture = Kind::None; }

    /**
     * @p knob / @p box — canvas hit facts.
     * @p region / @p worldAtDown — current camera + down contact in world (empty path).
     */
    FingerResult begin(double panelX, double panelY, bool knob, bool box,
                       const canvasframe::WorldAabb &region,
                       canvasframe::WorldPt worldAtDown)
    {
        FingerResult r;
        if (!armed) {
            r.accepted = false;
            return r;
        }
        gesture = Kind::None;
        downPanel = {panelX, panelY};
        using namespace handtouch;
        const HitKind hit = classifyHit(false, knob, box);
        const FingerAction act = actionOnDown(hit);

        if (act == FingerAction::Resize) {
            gesture = Kind::Resize;
            r.intent = FingerIntent::BeginHandleResize;
            return r;
        }
        if (act == FingerAction::SelectMove) {
            gesture = Kind::Move;
            r.intent = FingerIntent::ArmSelFreeform | FingerIntent::BeginSelectMove;
            return r;
        }
        gesture = Kind::EmptyPending;
        panOrigin = region;
        downWorld = worldAtDown;
        return r;
    }

    /**
     * One-finger move. @p worldNow* — contact mapped through the *origin* region.
     * @p previewDue — canvas throttle clock for pan publish/rasterize.
     */
    FingerResult update(double panelX, double panelY, int fingerCount,
                        handtouch::FollowDirection follow, bool previewDue,
                        double worldNowX, double worldNowY)
    {
        FingerResult r;
        if (ignoresOneFingerUpdate())
            return r;
        if (isLiveManip()) {
            r.intent = FingerIntent::UpdateSelection;
            return r;
        }
        if (fingerCount >= 2)
            return r;

        using namespace handtouch;
        const double dx = panelX - downPanel.x;
        const double dy = panelY - downPanel.y;
        if (gesture == Kind::EmptyPending) {
            if (actionOnEmptyMove(travelDu(dx, dy)) != FingerAction::LocalPan)
                return r;
            if (onLocalNav(follow).blocked)
                return r;
            gesture = Kind::EmptyPan;
        }
        if (gesture != Kind::EmptyPan)
            return r;

        r.region = panKeepWorldUnderFinger(panOrigin.box(), downWorld.x, downWorld.y, worldNowX,
                                           worldNowY);
        r.hasRegion = true;
        r.intent = liveNavIntents(previewDue, /*settle=*/false);
        return r;
    }

    FingerResult end(double panelX, double panelY, double worldNowX, double worldNowY)
    {
        FingerResult r;
        const Kind g = gesture;
        gesture = Kind::None;

        if (g == Kind::Move || g == Kind::Resize) {
            r.intent = FingerIntent::EndSelectionGesture;
            return r;
        }
        if (g == Kind::EmptyPan) {
            r.region = handtouch::panKeepWorldUnderFinger(panOrigin.box(), downWorld.x, downWorld.y,
                                                          worldNowX, worldNowY);
            r.hasRegion = true;
            r.intent = liveNavIntents(/*previewDue=*/true, /*settle=*/true);
            return r;
        }
        if (g == Kind::EmptyPending) {
            using namespace handtouch;
            const double dx = panelX - downPanel.x;
            const double dy = panelY - downPanel.y;
            if (emptyTapClearsSelection(travelDu(dx, dy)))
                r.intent = FingerIntent::ClearSelection | FingerIntent::RefreshChrome;
            return r;
        }
        return r;
    }

    FingerResult beginTwo(double ax, double ay, double bx, double by,
                          const canvasframe::WorldAabb &region,
                          handtouch::TwoFingerContacts originUv,
                          handtouch::FollowDirection follow)
    {
        FingerResult r;
        if (isLiveManip() || gesture == Kind::Chip) {
            r.accepted = false;
            return r;
        }
        if (handtouch::onLocalNav(follow).blocked) {
            r.accepted = false;
            return r;
        }
        panOrigin = region;
        twoOrigin = originUv;
        twoCurrent = originUv;
        twoA = {ax, ay};
        twoB = {bx, by};
        gesture = Kind::TwoFinger;
        r.region = handtouch::applyTwoFingerPanPinch(panOrigin.box(), twoOrigin, twoCurrent);
        r.hasRegion = true;
        r.intent = liveNavIntents(/*previewDue=*/true, /*settle=*/false);
        return r;
    }

    FingerResult updateTwo(double ax, double ay, double bx, double by,
                           handtouch::TwoFingerContacts currentUv, bool previewDue)
    {
        FingerResult r;
        if (gesture != Kind::TwoFinger)
            return r;
        twoA = {ax, ay};
        twoB = {bx, by};
        twoCurrent = currentUv;
        r.region = handtouch::applyTwoFingerPanPinch(panOrigin.box(), twoOrigin, twoCurrent);
        r.hasRegion = true;
        r.intent = liveNavIntents(previewDue, /*settle=*/false);
        return r;
    }

    FingerResult endTwo()
    {
        FingerResult r;
        if (gesture != Kind::TwoFinger)
            return r;
        gesture = Kind::None;
        lockedUntilLift = true;
        r.region = handtouch::applyTwoFingerPanPinch(panOrigin.box(), twoOrigin, twoCurrent);
        r.hasRegion = true;
        r.intent = FingerIntent::LockUntilLift
            | liveNavIntents(/*previewDue=*/true, /*settle=*/true);
        return r;
    }

    FingerResult secondContact(bool manipActive)
    {
        FingerResult r;
        if (manipActive || isLiveManip())
            r.intent |= FingerIntent::AbortManip;
        if (gesture != Kind::TwoFinger)
            gesture = Kind::None;
        lockedUntilLift = true;
        r.intent |= FingerIntent::LockUntilLift;
        return r;
    }

    /**
     * Non-two-finger cancel branch. For TwoFinger, caller uses endTwo() instead
     * (settle the viewport that was already panning).
     */
    FingerResult cancel(bool manipActive)
    {
        FingerResult r;
        lockedUntilLift = false;
        if (gesture == Kind::TwoFinger)
            return r;
        const Kind g = gesture;
        gesture = Kind::None;
        if (g == Kind::Move || g == Kind::Resize || manipActive)
            r.intent = FingerIntent::EndSelectionGesture;
        return r;
    }
};

} // namespace fingergesture
} // namespace epaper
