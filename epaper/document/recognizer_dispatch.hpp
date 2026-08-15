#pragma once
/**
 * Closure-first recognizer dispatch at pen-up. Exactly one verdict.
 * @implements [SRS-EP-10] ADR-0022 dispatch table
 *
 * Connector commit is STORY-EP-030 — open + recog.connector → create_connector.
 */

#include "ingest_stroke.hpp"
#include "membership.hpp"
#include "recognize_connector.hpp"
#include "recognize_enclose.hpp"

#include <chrono>
#include <cmath>
#include <string>
#include <vector>

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
    /** Compact enclose-test breakdown for the [recog] line (does not affect verdict). */
    std::string encloseWhy;
    EncloseResult enclose;
    MembershipResult membership;
    ConnectorResult connector;
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

struct ClosureMeas {
    bool closed = false;
    double gap = 0;
    double pathLen = 0;
    double limit = 0;
};

inline ClosureMeas measureClosure(const std::vector<InkSample> &s)
{
    ClosureMeas m;
    if (s.size() < 2)
        return m;
    m.pathLen = polylineLength(s);
    m.gap = std::hypot(s.front().x - s.back().x, s.front().y - s.back().y);
    m.limit = std::max(kClosureGapCapWorld, kClosureGapFrac * m.pathLen);
    m.closed = m.pathLen >= 1.0 && m.gap <= m.limit;
    return m;
}

/** @implements [SRS-EP-10] closed-ish classifier before enclose */
inline bool strokeIsClosedIsh(const std::vector<InkSample> &s)
{
    return measureClosure(s).closed;
}

inline std::string formatEncloseWhy(const std::string &strokeId, RecogLatch latch,
                                    const ClosureMeas &c, const EncloseResult &enclose,
                                    RecogOutcome outcome)
{
    auto rnd = [](double x) { return std::to_string(int(std::lround(x))); };
    std::string fail;
    std::string rest;
    if (!latch.inkBox)
        fail = "recog_off";
    else if (!c.closed)
        fail = "open";
    else if (outcome == RecogOutcome::Enclose)
        fail = "none";
    else if (!enclose.reason.empty()) {
        const auto sp = enclose.reason.find(' ');
        fail = enclose.reason.substr(0, sp);
        if (sp != std::string::npos)
            rest = enclose.reason.substr(sp + 1);
    } else
        fail = "enclose_failed";
    if (outcome == RecogOutcome::Enclose && enclose.reason.find("shape=") != std::string::npos)
        rest = enclose.reason;
    const double shorter =
        std::min(enclose.fittedWorldBounds.width, enclose.fittedWorldBounds.height);
    int minBar = int(kMinEncloseWithContent);
    if (fail == "too_small_empty" || fail == "not_primitive"
        || enclose.reason.find("shape=") != std::string::npos)
        minBar = int(kMinEncloseEmpty);
    std::string s = "id=" + (strokeId.empty() ? std::string("-") : strokeId);
    s += " fail=" + fail;
    s += " gap=" + rnd(c.gap) + " lim=" + rnd(c.limit) + " L=" + rnd(c.pathLen);
    s += " shorter=" + rnd(shorter) + " min=" + std::to_string(minBar);
    if (!rest.empty())
        s += " " + rest;
    return s;
}

inline RecogDispatchResult dispatchPenUp(DeviceDocument &doc, EncloseStrokeInput stroke,
                                         RecogLatch latch)
{
    RecogDispatchResult out;
    const auto t0 = std::chrono::steady_clock::now();

    const ClosureMeas clo = measureClosure(stroke.samples);
    const bool closed = clo.closed;
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
                out.connector = tryRecognizeConnector(doc, stroke.id);
                if (out.connector.kind == ConnectorKind::Created) {
                    out.outcome = RecogOutcome::Connector;
                    out.guard = "none";
                } else {
                    out.outcome = RecogOutcome::Ink;
                    out.guard = out.connector.reason.empty() ? "none" : out.connector.reason;
                }
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
    out.encloseWhy = formatEncloseWhy(stroke.id, latch, clo, out.enclose, out.outcome);
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
