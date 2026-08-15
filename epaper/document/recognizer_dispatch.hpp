#pragma once
/**
 * Closure-first recognizer dispatch at pen-up. Exactly one verdict.
 * @implements [SRS-EP-10] ADR-0022 dispatch table
 *
 * Connector commit is STORY-EP-030 — this step only reserves the slot (open +
 * recog.connector → still ink until that story).
 */

#include "ingest_stroke.hpp"
#include "membership.hpp"
#include "recognize_enclose.hpp"

#include <chrono>
#include <cmath>
#include <string>

namespace epaper {
namespace document {

/** Closure classifier (ADR-0022 / SRS-EP-10). Near-close of a large box must count.
 * Closed-ish iff first–last gap ≤ max(cap, frac × path length) — not AND. */
constexpr double kClosureGapFrac = 0.15;
constexpr double kClosureGapCapWorld = 48;

struct RecogLatch {
    bool inkBox = true;
    bool connector = true;
};

enum class RecogOutcome {
    Enclose,
    Membership,
    Connector,
    Ink,
};

struct RecogDispatchResult {
    RecogOutcome outcome = RecogOutcome::Ink;
    std::string guard = "none";
    EncloseResult enclose;
    MembershipResult membership;
    ApplyResult apply;
    std::int64_t ns = 0;

    std::string outcomeName() const
    {
        switch (outcome) {
        case RecogOutcome::Enclose:
            return "enclose";
        case RecogOutcome::Membership:
            return "membership";
        case RecogOutcome::Connector:
            return "connector";
        case RecogOutcome::Ink:
            return "ink";
        }
        return "ink";
    }
};

inline double polylineLength(const std::vector<InkSample> &s)
{
    double L = 0;
    for (size_t i = 1; i < s.size(); ++i) {
        const double dx = s[i].x - s[i - 1].x;
        const double dy = s[i].y - s[i - 1].y;
        L += std::hypot(dx, dy);
    }
    return L;
}

/** @implements [SRS-EP-10] closed-ish classifier before enclose */
inline bool strokeIsClosedIsh(const std::vector<InkSample> &s)
{
    if (s.size() < 2)
        return false;
    const double L = polylineLength(s);
    if (L < 1.0)
        return false;
    const double gap = std::hypot(s.front().x - s.back().x, s.front().y - s.back().y);
    return gap <= std::max(kClosureGapCapWorld, kClosureGapFrac * L);
}

inline RecogDispatchResult dispatchPenUp(DeviceDocument &doc, EncloseStrokeInput stroke,
                                         RecogLatch latch)
{
    RecogDispatchResult out;
    const auto t0 = std::chrono::steady_clock::now();

    const bool closed = strokeIsClosedIsh(stroke.samples);
    if (closed && latch.inkBox) {
        stroke.armedAtPenDown = StrokeArmedTool::InkBox;
        out.enclose = commitStrokeWithEncloseRecognition(doc, stroke);
        if (out.enclose.kind == EncloseKind::Created) {
            out.outcome = RecogOutcome::Enclose;
            out.guard = "none";
            out.apply.applied = true;
        } else if (out.enclose.kind == EncloseKind::Skipped) {
            out.outcome = RecogOutcome::Ink;
            out.guard = out.enclose.reason.empty() ? "skipped" : out.enclose.reason;
            out.apply.applied = false;
        } else {
            out.membership = tryDrawIntoMembership(doc, stroke.id);
            if (out.membership.kind == MembershipKind::Joined) {
                out.outcome = RecogOutcome::Membership;
                out.guard = out.enclose.reason.empty() ? "enclose_failed" : out.enclose.reason;
            } else {
                out.outcome = RecogOutcome::Ink;
                out.guard = out.enclose.reason.empty() ? "none" : out.enclose.reason;
            }
            out.apply.applied = true;
        }
    } else {
        stroke.armedAtPenDown = StrokeArmedTool::Pen;
        out.enclose = commitStrokeWithEncloseRecognition(doc, stroke);
        out.apply.applied = out.enclose.kind != EncloseKind::Skipped;
        if (out.apply.applied) {
            out.membership = tryDrawIntoMembership(doc, stroke.id);
            if (out.membership.kind == MembershipKind::Joined) {
                out.outcome = RecogOutcome::Membership;
                out.guard = "none";
            } else if (!closed && latch.connector) {
                // STORY-EP-030 owns create_connector; do not invent geometry here.
                out.outcome = RecogOutcome::Ink;
                out.guard = "connector_pending";
            } else {
                out.outcome = RecogOutcome::Ink;
                out.guard = closed ? "recog_ink_box_off" : "none";
            }
        } else {
            out.outcome = RecogOutcome::Ink;
            out.guard = out.enclose.reason.empty() ? "skipped" : out.enclose.reason;
        }
    }

    const auto t1 = std::chrono::steady_clock::now();
    out.ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    return out;
}

inline RecogDispatchResult dispatchFinishedStroke(DeviceDocument &doc, const FinishedStroke &stroke,
                                                  const PanelToWorld &map, RecogLatch latch)
{
    EncloseStrokeInput input;
    input.id = stroke.id;
    input.width = stroke.strokeWidthWorld;
    input.stroke = stroke.stroke;
    input.source = stroke.source;
    input.samples.reserve(stroke.samples.size());
    for (size_t i = 0; i < stroke.samples.size(); ++i) {
        const DigitizerSample &s = stroke.samples[i];
        InkSample w;
        map(s.panelX, s.panelY, &w.x, &w.y);
        w.pressure = s.pressure;
        w.tiltX = s.tiltX;
        w.tiltY = s.tiltY;
        w.t = s.t ? s.t : std::optional<double>(static_cast<double>(i));
        w.timestamp = s.timestamp;
        w.distance = s.distance;
        w.extras = s.extras;
        input.samples.push_back(std::move(w));
    }
    return dispatchPenUp(doc, std::move(input), latch);
}

} // namespace document
} // namespace epaper
