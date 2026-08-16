#pragma once

/**
 * First-contact origin / stale-sample predicate for local pen ingest.
 * @implements [SRS-EP-01] pen event path — no ink from digitizer origin
 * @fix [STORY-EP-033] reject origin/stale first sample on pen-down
 *
 * Host-testable, no Qt. mapInputToCanvas(0,0) is panel (0, height) bottom-left.
 */

namespace epaper {
namespace ingest {

/** Widget/digitizer units: evdev BTN_TOUCH before ABS_X/Y is ~ (0,0). */
constexpr double kOriginRawEps = 16.0;
/** Mapped panel units: bottom-left corner after the landscape→portrait map. */
constexpr double kOriginCanvasEps = 16.0;
/** First-segment length from an origin start that must not be painted. */
constexpr double kOriginJumpPx = 32.0;

inline bool isStaleOriginRaw(double rawX, double rawY)
{
    return rawX >= -kOriginRawEps && rawX <= kOriginRawEps && rawY >= -kOriginRawEps
        && rawY <= kOriginRawEps;
}

inline bool isStaleOriginMapped(double canvasX, double canvasY, double panelH)
{
    if (panelH < 1.0)
        panelH = 1.0;
    return canvasX >= -kOriginCanvasEps && canvasX <= kOriginCanvasEps
        && canvasY >= panelH - kOriginCanvasEps && canvasY <= panelH + kOriginCanvasEps;
}

inline bool isStaleOriginSample(double rawX, double rawY, double canvasX, double canvasY,
                               double panelH)
{
    return isStaleOriginRaw(rawX, rawY) || isStaleOriginMapped(canvasX, canvasY, panelH);
}

inline bool isImplausibleOriginJump(double fromCanvasX, double fromCanvasY, double toCanvasX,
                                   double toCanvasY, double panelH)
{
    if (!isStaleOriginMapped(fromCanvasX, fromCanvasY, panelH))
        return false;
    const double dx = toCanvasX - fromCanvasX;
    const double dy = toCanvasY - fromCanvasY;
    return dx * dx + dy * dy > kOriginJumpPx * kOriginJumpPx;
}

enum class OriginGuardAction {
    Proceed = 0,
    Discard,
    PromoteToPress,
    DropContact
};

/**
 * Awaiting-plausible-press state machine.
 * Stale Press is held; the first non-origin Move is the real press (chip hit-test too).
 * Plausible Press proceeds immediately — no extra sample of latency.
 */
inline OriginGuardAction decideOriginPress(bool isPress, bool isMove, bool isRelease,
                                           bool staleOrigin, bool *awaiting)
{
    if (!awaiting)
        return OriginGuardAction::Proceed;
    if (isPress) {
        if (staleOrigin) {
            *awaiting = true;
            return OriginGuardAction::Discard;
        }
        *awaiting = false;
        return OriginGuardAction::Proceed;
    }
    // Origin Move is never painted — including after a real Press (tip → bottom-left).
    if (isMove && staleOrigin)
        return OriginGuardAction::Discard;
    if (isMove && *awaiting) {
        *awaiting = false;
        return OriginGuardAction::PromoteToPress;
    }
    if (isRelease && *awaiting) {
        *awaiting = false;
        return OriginGuardAction::DropContact;
    }
    return OriginGuardAction::Proceed;
}

} // namespace ingest
} // namespace epaper
