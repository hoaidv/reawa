#pragma once

/**
 * Hand-touch classifier (no Qt): one-finger pick/move/palm/pan and two-finger pan/pinch.
 * @implements [SRS-EP-21] one-finger pick move palm pan
 * @implements [SRS-EP-23] finger exclusive-tool switch
 * @implements [SRS-EP-24] two-finger pan pinch viewport
 * @implements [SRS-EP-25] one-finger hand-touch quality
 * @implements [SRS-EP-26] two-finger map-apply quality
 */

#include <cmath>
#include <string>

namespace epaper {
namespace handtouch {

/** 20 mm Euclidean panel travel @ 226 dpi (architect bind; field 2026-08-20). */
constexpr double kPanelDpi = 226.0;
constexpr double kPalmTravelMm = 20.0;
constexpr double kPalmTravelDu = 178.0;
/** Three or more capacitive contacts = palm, not two-finger pan. */
constexpr int kPalmMinContacts = 3;
/** Finger-eligible floor = primary ToolChip tile. */
constexpr double kFingerHandleHitDu = 64.0;

enum class HitKind { Chip, Knob, Box, Empty };

enum class FollowDirection { None, InfiniToEpaper, EpaperToInfini };

enum class FingerAction { ChipTap, SelectMove, Resize, PalmRest, LocalPan, Ignore };

inline double hypot2(double dx, double dy)
{
    return std::sqrt(dx * dx + dy * dy);
}

inline double travelDu(double panelDx, double panelDy)
{
    return hypot2(panelDx, panelDy);
}

inline double travelMm(double panelDx, double panelDy)
{
    return travelDu(panelDx, panelDy) / kPanelDpi * 25.4;
}

inline bool travelPastPalm(double panelDx, double panelDy)
{
    return travelDu(panelDx, panelDy) > kPalmTravelDu;
}

/** Canvas hand-touch runs when the toggle is on and the pen is neither near nor in contact. */
inline bool handTouchEnabled(bool penNear, bool penContact, bool toggleOn = true)
{
    return toggleOn && !penNear && !penContact;
}

inline bool palmByContactCount(int n)
{
    return n >= kPalmMinContacts;
}

/** Empty-canvas lift with travel at/below palm threshold: deselect, 0 pan, 0 tool switch. */
inline bool emptyTapClearsSelection(double travelDuValue)
{
    return travelDuValue <= kPalmTravelDu;
}

/** Chip > knob > box > empty. Box/knob/chip win over empty pan. */
inline HitKind classifyHit(bool chip, bool knob, bool box)
{
    if (chip)
        return HitKind::Chip;
    if (knob)
        return HitKind::Knob;
    if (box)
        return HitKind::Box;
    return HitKind::Empty;
}

inline FingerAction actionOnDown(HitKind hit)
{
    switch (hit) {
    case HitKind::Chip:
        return FingerAction::ChipTap;
    case HitKind::Knob:
        return FingerAction::Resize;
    case HitKind::Box:
        return FingerAction::SelectMove;
    case HitKind::Empty:
        return FingerAction::PalmRest;
    }
    return FingerAction::Ignore;
}

inline FingerAction actionOnEmptyMove(double travelDuValue)
{
    return travelDuValue > kPalmTravelDu ? FingerAction::LocalPan : FingerAction::PalmRest;
}

inline bool switchesToSelFreeform(HitKind hit)
{
    return hit == HitKind::Box;
}

inline FollowDirection parseFollow(const std::string &s)
{
    if (s == "infini_to_epaper")
        return FollowDirection::InfiniToEpaper;
    if (s == "epaper_to_infini")
        return FollowDirection::EpaperToInfini;
    return FollowDirection::None;
}

inline const char *followId(FollowDirection d)
{
    switch (d) {
    case FollowDirection::InfiniToEpaper:
        return "infini_to_epaper";
    case FollowDirection::EpaperToInfini:
        return "epaper_to_infini";
    case FollowDirection::None:
    default:
        return "none";
    }
}

struct LocalNav {
    FollowDirection direction = FollowDirection::None;
    bool turnedFollowOff = false;
    bool blocked = false;
};

/**
 * Empty pan / two-finger pan-pinch while following Infini is blocked so cameras stay coupled.
 * Box pick / move / resize does not call this.
 * @implements [SRS-EP-21] no pan/zoom while following Infini
 * @implements [SRS-EP-24] no pinch while following Infini
 */
inline LocalNav onLocalNav(FollowDirection current)
{
    LocalNav out;
    out.direction = current;
    if (current == FollowDirection::InfiniToEpaper)
        out.blocked = true;
    return out;
}

inline bool shouldPublishViewport(FollowDirection d)
{
    return d == FollowDirection::EpaperToInfini;
}

inline bool shouldApplyInboundViewport(FollowDirection d)
{
    return d == FollowDirection::InfiniToEpaper;
}

struct WorldAabb {
    double minX = 0;
    double minY = 0;
    double maxX = 0;
    double maxY = 0;
};

inline WorldAabb shiftAabb(const WorldAabb &a, double dx, double dy)
{
    WorldAabb o = a;
    o.minX += dx;
    o.minY += dy;
    o.maxX += dx;
    o.maxY += dy;
    return o;
}

/**
 * Content follows the finger: keep the world point from down under the current
 * panel contact. `worldNow` is that contact mapped through the *origin* region.
 */
inline WorldAabb panKeepWorldUnderFinger(const WorldAabb &origin, double worldDownX, double worldDownY,
                                         double worldNowX, double worldNowY)
{
    return shiftAabb(origin, worldDownX - worldNowX, worldDownY - worldNowY);
}

/** Session probe used by host tests to lock follow-off-then-pan ordering. */
struct OneFingerSession {
    HitKind hit = HitKind::Empty;
    FollowDirection follow = FollowDirection::None;
    FingerAction action = FingerAction::PalmRest;
    bool panApplied = false;
    int viewportUp = 0;
    int inboundApplied = 0;
    WorldAabb region{};
};

inline void applyLocalPanStart(OneFingerSession &s)
{
    const LocalNav nav = onLocalNav(s.follow);
    if (nav.blocked) {
        s.action = FingerAction::PalmRest;
        s.panApplied = false;
        return;
    }
    s.follow = nav.direction;
    s.action = FingerAction::LocalPan;
    s.panApplied = true;
}

inline bool tryApplyInboundViewport(OneFingerSession &s)
{
    if (!shouldApplyInboundViewport(s.follow))
        return false;
    ++s.inboundApplied;
    return true;
}

inline void maybePublishViewport(OneFingerSession &s)
{
    if (shouldPublishViewport(s.follow))
        ++s.viewportUp;
}

/** p95 map-apply bar for the next pen sample after two-finger pan/pinch. */
constexpr double kTwoFingerMapApplyBudgetMs = 100.0;

enum class TwoFingerAction { None, PanPinch, Blocked };

struct TwoFingerContacts {
    double u0 = 0;
    double v0 = 0;
    double u1 = 0;
    double v1 = 0;
};

inline void mapUvToWorld(const WorldAabb &r, double u, double v, double *wx, double *wy)
{
    const double rw = r.maxX - r.minX;
    const double rh = r.maxY - r.minY;
    *wx = r.minX + u * rw;
    *wy = r.minY + v * rh;
}

inline void uniformScaleOf(const WorldAabb &origin, const WorldAabb &now, double *sx, double *sy)
{
    const double ow = origin.maxX - origin.minX;
    const double oh = origin.maxY - origin.minY;
    const double nw = now.maxX - now.minX;
    const double nh = now.maxY - now.minY;
    *sx = (nw > 1e-12) ? ow / nw : 1.0;
    *sy = (nh > 1e-12) ? oh / nh : 1.0;
}

/**
 * Two-finger pan + pinch on drawingRegion. Uniform scale only (scale_x == scale_y);
 * 0 rotation / skew — AABB stays axis-aligned.
 * @implements [SRS-EP-24] two-finger local map
 */
inline WorldAabb applyTwoFingerPanPinch(const WorldAabb &origin, const TwoFingerContacts &from,
                                        const TwoFingerContacts &to)
{
    const double originMidU = (from.u0 + from.u1) * 0.5;
    const double originMidV = (from.v0 + from.v1) * 0.5;
    const double nowMidU = (to.u0 + to.u1) * 0.5;
    const double nowMidV = (to.v0 + to.v1) * 0.5;
    const double originDist = hypot2(from.u1 - from.u0, from.v1 - from.v0);
    const double nowDist = hypot2(to.u1 - to.u0, to.v1 - to.v0);
    const double ow = origin.maxX - origin.minX;
    const double oh = origin.maxY - origin.minY;
    double ratio = 1.0;
    if (originDist > 1e-9 && nowDist > 1e-9)
        ratio = originDist / nowDist;
    const double nw = ow * ratio;
    const double nh = oh * ratio;
    double cx = 0;
    double cy = 0;
    mapUvToWorld(origin, originMidU, originMidV, &cx, &cy);
    WorldAabb out;
    out.minX = cx - nowMidU * nw;
    out.minY = cy - nowMidV * nh;
    out.maxX = out.minX + nw;
    out.maxY = out.minY + nh;
    return out;
}

/** Session probe for hand-touch-two-finger.feature. */
struct TwoFingerSession {
    FollowDirection follow = FollowDirection::None;
    bool boxMoveInFlight = false;
    bool resizeInFlight = false;
    bool sessionDown = false;
    bool twoFingerStarted = false;
    bool panApplied = false;
    bool moveContinued = false;
    bool resizeContinued = false;
    bool publishQueued = false;
    int viewportUp = 0;
    int viewportDown = 0;
    int inboundApplied = 0;
    bool settleOnLastUp = false;
    const char *lastUpSource = "";
    TwoFingerAction action = TwoFingerAction::None;
    WorldAabb origin{};
    WorldAabb region{};
    WorldAabb lastPublishedRegion{};
    TwoFingerContacts originContacts{};
    double scaleX = 1;
    double scaleY = 1;
    double rotation = 0;
    double skew = 0;
};

inline bool boxGestureInFlight(const TwoFingerSession &s)
{
    return s.boxMoveInFlight || s.resizeInFlight;
}

/** Second finger does not start pan while box-move or resize is in flight. */
inline bool canStartTwoFinger(const TwoFingerSession &s)
{
    return !boxGestureInFlight(s);
}

/**
 * Two-finger pan/pinch is blocked while following Infini; pick/move still runs.
 * @implements [SRS-EP-24] no pinch while following Infini
 */
inline bool tryStartTwoFinger(TwoFingerSession &s, const TwoFingerContacts &contacts)
{
    if (!canStartTwoFinger(s)) {
        s.action = TwoFingerAction::Blocked;
        s.moveContinued = s.boxMoveInFlight;
        s.resizeContinued = s.resizeInFlight;
        return false;
    }
    const LocalNav nav = onLocalNav(s.follow);
    if (nav.blocked) {
        s.action = TwoFingerAction::Blocked;
        return false;
    }
    s.follow = nav.direction;
    s.originContacts = contacts;
    s.origin = s.region;
    s.twoFingerStarted = true;
    s.action = TwoFingerAction::PanPinch;
    return true;
}

inline void applyTwoFingerStep(TwoFingerSession &s, const TwoFingerContacts &now)
{
    if (!s.twoFingerStarted)
        return;
    s.region = applyTwoFingerPanPinch(s.origin, s.originContacts, now);
    uniformScaleOf(s.origin, s.region, &s.scaleX, &s.scaleY);
    s.rotation = 0;
    s.skew = 0;
    s.panApplied = true;
}

/**
 * Publish viewport up only if Infini follow is on. Link down → 0 up; chip may queue.
 * @implements [SRS-EP-24] publish only if Infini follow on
 */
inline void maybePublishTwoFinger(TwoFingerSession &s, bool settle)
{
    if (s.sessionDown) {
        s.publishQueued = true;
        return;
    }
    if (!shouldPublishViewport(s.follow))
        return;
    ++s.viewportUp;
    s.lastUpSource = "epaper";
    s.lastPublishedRegion = s.region;
    s.settleOnLastUp = settle;
}

inline bool tryApplyInboundViewport(TwoFingerSession &s)
{
    if (!shouldApplyInboundViewport(s.follow))
        return false;
    ++s.inboundApplied;
    ++s.viewportDown;
    return true;
}

} // namespace handtouch
} // namespace epaper
