#pragma once
/**
 * On-device enclose recognition at pen-up. Not an op — only create_smart_group publishes.
 * @implements [SRS-EP-10] enclose recognition (tool latch, no propose/accept)
 * @implements [SRS-EP-14] 0 peer messages; fitted AABB; consecutive creates
 *
 * Port of infini/src/document/recognizeEnclose.ts. Device trigger is the tool
 * latched at pen-down (Ink-box), not wire intent. Draw-into membership is EP-017.
 */

#include "device_document.hpp"
#include "enclose_shape.hpp"
#include "affine.hpp"
#include "nested_flatten.hpp"
#include "ingest_stroke.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace epaper {
namespace document {

enum class StrokeArmedTool {
    Pen,
    InkBox,
    Selection,
};

struct EncloseStrokeInput {
    std::string id;
    std::vector<InkSample> samples; // world space
    double width = 2.5;
    std::string stroke = "#1C2430";
    StrokeArmedTool armedAtPenDown = StrokeArmedTool::Pen;
    std::string source = "epaper";
};

enum class EncloseKind {
    Created,
    OrdinaryInk,
    Skipped,
};

struct EncloseResult {
    EncloseKind kind = EncloseKind::Skipped;
    std::string reason;
    std::string smartGroupId;
    SmartBounds fittedWorldBounds;
    /** Populated on Created — boundary then content ids (for Device Log). */
    std::vector<std::string> childIds;
};

inline SmartBounds samplesAabb(const std::vector<InkSample> &samples)
{
    double minX = std::numeric_limits<double>::infinity();
    double minY = std::numeric_limits<double>::infinity();
    double maxX = -std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();
    for (const auto &s : samples) {
        minX = std::min(minX, s.x);
        minY = std::min(minY, s.y);
        maxX = std::max(maxX, s.x);
        maxY = std::max(maxY, s.y);
    }
    if (!std::isfinite(minX))
        return {};
    SmartBounds b;
    b.x = minX;
    b.y = minY;
    b.width = std::max(0.0, maxX - minX);
    b.height = std::max(0.0, maxY - minY);
    return b;
}

/** Fraction of samples inside inclusive AABB. @implements [SRS-EP-10] content guard */
inline double fractionSamplesInside(const std::vector<InkSample> &samples, const SmartBounds &bounds)
{
    if (samples.empty())
        return 0;
    const double maxX = bounds.x + bounds.width;
    const double maxY = bounds.y + bounds.height;
    int inside = 0;
    for (const auto &s : samples) {
        if (s.x >= bounds.x && s.x <= maxX && s.y >= bounds.y && s.y <= maxY)
            ++inside;
    }
    return static_cast<double>(inside) / static_cast<double>(samples.size());
}

struct Vec2 {
    double x = 0;
    double y = 0;
};

inline Vec2 inkSamplesCentroid(const std::vector<InkSample> &samples)
{
    if (samples.empty())
        return {};
    double minX = std::numeric_limits<double>::infinity();
    double minY = std::numeric_limits<double>::infinity();
    double maxX = -std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();
    for (const auto &s : samples) {
        minX = std::min(minX, s.x);
        minY = std::min(minY, s.y);
        maxX = std::max(maxX, s.x);
        maxY = std::max(maxY, s.y);
    }
    return {(minX + maxX) / 2.0, (minY + maxY) / 2.0};
}

inline Vec2 inkSamplesMin(const std::vector<InkSample> &samples)
{
    Vec2 m{0, 0};
    if (samples.empty())
        return m;
    m.x = std::numeric_limits<double>::infinity();
    m.y = std::numeric_limits<double>::infinity();
    for (const auto &s : samples) {
        m.x = std::min(m.x, s.x);
        m.y = std::min(m.y, s.y);
    }
    return m;
}

/** Seed content UV from local AABB centroid vs SmartGroup.bounds. */
inline std::pair<double, double> seedLayoutOffset(const std::vector<InkSample> &samples,
                                                  const SmartBounds &bounds)
{
    const Vec2 c = inkSamplesCentroid(samples);
    const double w = bounds.width != 0 ? bounds.width : 1;
    const double h = bounds.height != 0 ? bounds.height : 1;
    return {(c.x - bounds.x) / w, (c.y - bounds.y) / h};
}

/**
 * Local ink sample → world under SmartGroup transform.
 * @implements [SRS-EP-10] layoutOffset + fixedInk draw rule
 */
inline Vec2 smartLocalToWorld(double localX, double localY, const DocNode &sg, const std::string &role,
                              const std::optional<std::pair<double, double>> &layoutOffset,
                              const Vec2 *contentMin, const Affine &ctx)
{
    Affine used = (role == "content" && sg.inkScaleMode == "fixedInk")
        ? affineCompose(ctx, affineTR(sg.transform))
        : affineCompose(ctx, affineOwn(sg.transform));
    (void)layoutOffset;
    (void)contentMin;
    Vec2 w;
    used.apply(localX, localY, &w.x, &w.y);
    return w;
}

inline Vec2 smartLocalToWorld(double localX, double localY, const DocNode &sg, const std::string &role,
                              const std::optional<std::pair<double, double>> &layoutOffset,
                              const Vec2 *contentMin)
{
    return smartLocalToWorld(localX, localY, sg, role, layoutOffset, contentMin, affineIdentity());
}

/**
 * Inverse of smartLocalToWorld for draw-into join.
 * @implements [SRS-EP-10] world samples → group-local
 * @fix [STORY-EP-017] fixedInk join must not divide by scale
 */
inline Vec2 smartWorldToLocal(double worldX, double worldY, const DocNode &sg,
                              const std::string &role)
{
    const SmartTransform &t = sg.transform;
    if (role == "content" && sg.inkScaleMode == "fixedInk") {
        // Inverse of local + translate (no scale) — matches paint.
        return {worldX - t.x, worldY - t.y};
    }
    double x = worldX - t.x;
    double y = worldY - t.y;
    if (t.rotation != 0) {
        const double csn = std::cos(-t.rotation);
        const double sn = std::sin(-t.rotation);
        const double rx = x * csn - y * sn;
        const double ry = x * sn + y * csn;
        x = rx;
        y = ry;
    }
    const double sx = t.scaleX != 0 ? t.scaleX : 1.0;
    const double sy = t.scaleY != 0 ? t.scaleY : 1.0;
    return {x / sx, y / sy};
}

/** World AABB of SmartGroup geometric bounds after compose. */
inline SmartBounds smartGroupWorldBounds(const DocNode &sg, const Affine &ctx)
{
    SmartBounds w;
    const SmartBounds &b = sg.smartBounds;
    const Affine o = outcomeAffine(ctx, sg);
    const double xs[2] = {b.x, b.x + b.width};
    const double ys[2] = {b.y, b.y + b.height};
    double minX = std::numeric_limits<double>::infinity();
    double minY = std::numeric_limits<double>::infinity();
    double maxX = -std::numeric_limits<double>::infinity();
    double maxY = -std::numeric_limits<double>::infinity();
    for (double x : xs) {
        for (double y : ys) {
            double wx = 0;
            double wy = 0;
            o.apply(x, y, &wx, &wy);
            minX = std::min(minX, wx);
            minY = std::min(minY, wy);
            maxX = std::max(maxX, wx);
            maxY = std::max(maxY, wy);
        }
    }
    if (!std::isfinite(minX))
        return {};
    w.x = minX;
    w.y = minY;
    w.width = std::max(0.0, maxX - minX);
    w.height = std::max(0.0, maxY - minY);
    return w;
}

inline SmartBounds smartGroupWorldBounds(const DocNode &sg)
{
    return smartGroupWorldBounds(sg, affineIdentity());
}

/** First boundary-role ink of a SmartGroup, world polyline. Empty if none. */
inline std::vector<Vec2> smartGroupBoundaryWorld(const DocNode &sg, const Affine &ctx)
{
    std::vector<Vec2> poly;
    const Affine outcome = outcomeAffine(ctx, sg);
    for (const auto &c : sg.children) {
        if (c.kind != NodeKind::Ink)
            continue;
        const std::string role = c.role ? *c.role : std::string("content");
        if (role != "boundary" || c.samples.size() < 2)
            continue;
        poly.reserve(c.samples.size());
        for (const auto &s : c.samples) {
            Vec2 w;
            outcome.apply(s.x, s.y, &w.x, &w.y);
            poly.push_back(w);
        }
        break;
    }
    return poly;
}

inline std::vector<Vec2> smartGroupBoundaryWorld(const DocNode &sg)
{
    return smartGroupBoundaryWorld(sg, affineIdentity());
}

/** Even-odd fill; closes the ring if first≠last. */
inline bool pointInBoundary(double x, double y, const std::vector<Vec2> &poly)
{
    if (poly.size() < 3)
        return false;
    const Vec2 &f = poly.front();
    const Vec2 &b = poly.back();
    const bool closed = std::hypot(f.x - b.x, f.y - b.y) < 1e-6;
    const size_t n = closed ? poly.size() - 1 : poly.size();
    if (n < 3)
        return false;
    bool inside = false;
    size_t j = n - 1;
    for (size_t i = 0; i < n; ++i) {
        const double yi = poly[i].y;
        const double yj = poly[j].y;
        const double xi = poly[i].x;
        const double xj = poly[j].x;
        if ((yi > y) != (yj > y)) {
            const double xHit = (xj - xi) * (y - yi) / (yj - yi + 1e-30) + xi;
            if (x < xHit)
                inside = !inside;
        }
        j = i;
    }
    return inside;
}

inline bool segParamHits(Vec2 a, Vec2 b, Vec2 c, Vec2 d, double *tHit)
{
    const double rx = b.x - a.x;
    const double ry = b.y - a.y;
    const double sx = d.x - c.x;
    const double sy = d.y - c.y;
    const double den = rx * sy - ry * sx;
    if (std::abs(den) < 1e-18)
        return false;
    const double cx = c.x - a.x;
    const double cy = c.y - a.y;
    const double t = (cx * sy - cy * sx) / den;
    const double u = (cx * ry - cy * rx) / den;
    if (t > 1e-12 && t < 1.0 - 1e-12 && u >= 0.0 && u <= 1.0) {
        *tHit = t;
        return true;
    }
    return false;
}

/** Length of segment a→b that lies in the even-odd interior of poly. */
inline double segmentLengthInsidePoly(Vec2 a, Vec2 b, const std::vector<Vec2> &poly)
{
    const double len = std::hypot(b.x - a.x, b.y - a.y);
    if (len < 1e-12 || poly.size() < 3)
        return 0;
    const Vec2 &f = poly.front();
    const Vec2 &last = poly.back();
    const bool closed = std::hypot(f.x - last.x, f.y - last.y) < 1e-6;
    const size_t n = closed ? poly.size() - 1 : poly.size();
    if (n < 3)
        return 0;
    std::vector<double> ts;
    ts.push_back(0);
    ts.push_back(1);
    size_t j = n - 1;
    for (size_t i = 0; i < n; ++i) {
        double t = 0;
        if (segParamHits(a, b, poly[j], poly[i], &t))
            ts.push_back(t);
        j = i;
    }
    std::sort(ts.begin(), ts.end());
    double acc = 0;
    for (size_t i = 1; i < ts.size(); ++i) {
        if (ts[i] - ts[i - 1] < 1e-15)
            continue;
        const double mid = 0.5 * (ts[i - 1] + ts[i]);
        const double mx = a.x + mid * (b.x - a.x);
        const double my = a.y + mid * (b.y - a.y);
        if (pointInBoundary(mx, my, poly))
            acc += (ts[i] - ts[i - 1]) * len;
    }
    return acc;
}

/**
 * Fraction of polyline length inside a closed boundary (even-odd).
 * @implements [SRS-EP-10] draw-into membership length-in-boundary
 */
inline double fractionPolylineLengthInsidePoly(const std::vector<InkSample> &samples,
                                               const std::vector<Vec2> &poly)
{
    double total = 0;
    double inside = 0;
    for (size_t i = 1; i < samples.size(); ++i) {
        const Vec2 a{samples[i - 1].x, samples[i - 1].y};
        const Vec2 b{samples[i].x, samples[i].y};
        const double L = std::hypot(b.x - a.x, b.y - a.y);
        total += L;
        inside += segmentLengthInsidePoly(a, b, poly);
    }
    if (total < 1e-12)
        return 0;
    return inside / total;
}

/** 0 when the group has no boundary ink (AABB does not qualify). */
inline double fractionStrokeInsideBoundary(const DocNode &sg, const std::vector<InkSample> &samples)
{
    const auto poly = smartGroupBoundaryWorld(sg);
    if (poly.size() < 3)
        return 0;
    return fractionPolylineLengthInsidePoly(samples, poly);
}

/** @implements [SRS-EP-10] already-grouped ink is skipped */
inline void walkInkCandidates(const std::vector<DocNode> &nodes, bool parentIsSmartGroup,
                              std::vector<const DocNode *> &out)
{
    for (const auto &n : nodes) {
        if (n.kind == NodeKind::Ink) {
            if (!parentIsSmartGroup)
                out.push_back(&n);
            continue;
        }
        if (n.kind == NodeKind::SmartGroup)
            continue;
        if (n.kind == NodeKind::Frame || n.kind == NodeKind::Group)
            walkInkCandidates(n.children, false, out);
    }
}

inline JsonValue inkSamplesToJson(const std::vector<InkSample> &samples)
{
    JsonValue::Array arr;
    arr.reserve(samples.size());
    for (const auto &s : samples)
        arr.push_back(sampleToJson(s));
    return JsonValue::array(std::move(arr));
}

inline DocNode makeInkChild(const std::string &id, const std::string &role,
                            const std::vector<InkSample> &samples, const Style &style,
                            const std::optional<std::pair<double, double>> &layoutOffset)
{
    DocNode n;
    n.id = id;
    n.kind = NodeKind::Ink;
    n.samples = samples;
    n.style = style;
    n.role = role;
    n.layoutOffset = layoutOffset;
    return n;
}

inline ApplyResult appendOrdinaryInk(DeviceDocument &doc, const EncloseStrokeInput &stroke)
{
    Style style;
    style.stroke = stroke.stroke;
    style.strokeWidth = stroke.width;
    AppendInkEdit edit(stroke.id, stroke.samples, std::move(style));
    edit.setId(std::string("append_ink:") + stroke.id);
    edit.setSource(stroke.source);
    return doc.commitEdit(edit);
}

inline void collectTopLevelSmartGroups(const std::vector<DocNode> &nodes,
                                       std::vector<const DocNode *> &out)
{
    for (const auto &n : nodes) {
        if (n.kind == NodeKind::SmartGroup) {
            out.push_back(&n);
            continue;
        }
        if (n.kind == NodeKind::Frame || n.kind == NodeKind::Group)
            collectTopLevelSmartGroups(n.children, out);
    }
}

inline double fractionNaturalAreaInsideAabb(const DocNode &sg, const SmartBounds &fitted)
{
    const SmartBounds nat = smartGroupWorldBounds(sg);
    const double area = std::max(0.0, nat.width) * std::max(0.0, nat.height);
    if (area <= 1e-12)
        return 0;
    const double x0 = std::max(nat.x, fitted.x);
    const double y0 = std::max(nat.y, fitted.y);
    const double x1 = std::min(nat.x + nat.width, fitted.x + fitted.width);
    const double y1 = std::min(nat.y + nat.height, fitted.y + fitted.height);
    if (x1 <= x0 || y1 <= y0)
        return 0;
    return ((x1 - x0) * (y1 - y0)) / area;
}

/**
 * Commit finished stroke. Ink-box latch → enclose; Pen → ordinary ink only (no membership).
 * Draw-into membership is ADR-0022 step 2 in recognizer_dispatch.hpp (before enclose).
 * @implements [SRS-EP-10] stroke_end enclose path
 * @implements [SRS-EP-75] nested SmartGroup capture + flatten
 */
inline EncloseResult commitStrokeWithEncloseRecognition(DeviceDocument &doc,
                                                        const EncloseStrokeInput &stroke)
{
    EncloseResult out;
    if (stroke.samples.size() < 2) {
        out.kind = EncloseKind::Skipped;
        out.reason = "too_few_samples";
        return out;
    }

    out.fittedWorldBounds = samplesAabb(stroke.samples);

    if (stroke.armedAtPenDown != StrokeArmedTool::InkBox) {
        appendOrdinaryInk(doc, stroke);
        out.kind = EncloseKind::OrdinaryInk;
        out.reason = "pen_armed";
        return out;
    }

    const double shorter = std::min(out.fittedWorldBounds.width, out.fittedWorldBounds.height);
    if (shorter < kMinEncloseWithContent) {
        appendOrdinaryInk(doc, stroke);
        out.kind = EncloseKind::OrdinaryInk;
        out.reason = "too_small";
        return out;
    }

    std::vector<const DocNode *> candidates;
    walkInkCandidates(doc.rootChildren, false, candidates);
    struct Captured {
        std::string id;
        std::vector<InkSample> samples;
        Style style;
    };
    std::vector<Captured> capturable;
    for (const DocNode *ink : candidates) {
        if (fractionSamplesInside(ink->samples, out.fittedWorldBounds) >= 0.8)
            capturable.push_back({ink->id, ink->samples, ink->style});
    }
    std::vector<const DocNode *> sgCands;
    collectTopLevelSmartGroups(doc.rootChildren, sgCands);
    std::vector<const DocNode *> capturedSgs;
    for (const DocNode *sg : sgCands) {
        if (sg && fractionNaturalAreaInsideAabb(*sg, out.fittedWorldBounds) >= 0.8)
            capturedSgs.push_back(sg);
    }
    if (capturable.empty() && capturedSgs.empty()) {
        bool insideExisting = false;
        for (const auto &n : doc.rootChildren) {
            if (n.kind != NodeKind::SmartGroup)
                continue;
            if (fractionStrokeInsideBoundary(n, stroke.samples) >= 0.8) {
                insideExisting = true;
                break;
            }
        }
        if (insideExisting) {
            appendOrdinaryInk(doc, stroke);
            out.kind = EncloseKind::OrdinaryInk;
            out.reason = "no_content";
            return out;
        }
        if (shorter < kMinEncloseEmpty) {
            appendOrdinaryInk(doc, stroke);
            out.kind = EncloseKind::OrdinaryInk;
            out.reason = "too_small_empty";
            return out;
        }
        const EmptyShapeVerdict sh = classifyEmptyBoundaryShape(
            stroke.samples, out.fittedWorldBounds.x, out.fittedWorldBounds.y,
            out.fittedWorldBounds.width, out.fittedWorldBounds.height);
        out.reason = sh.diag();
        if (!sh.ok) {
            appendOrdinaryInk(doc, stroke);
            out.kind = EncloseKind::OrdinaryInk;
            out.reason = std::string("not_primitive ") + sh.diag();
            return out;
        }
    }

    // Local-space Smart Group: bounds origin at (0,0), transform carries world origin.
    // @implements [SRS-EP-10] enclose → create_smart_group geometry
    const SmartBounds world = out.fittedWorldBounds;
    SmartBounds bounds;
    bounds.x = 0;
    bounds.y = 0;
    bounds.width = world.width;
    bounds.height = world.height;

    Style style = defaultStyle();
    style.stroke = stroke.stroke;
    style.strokeWidth = stroke.width;

    std::vector<InkSample> boundarySamples = stroke.samples;
    for (size_t i = 0; i < boundarySamples.size(); ++i) {
        boundarySamples[i].x -= world.x;
        boundarySamples[i].y -= world.y;
        if (!boundarySamples[i].t)
            boundarySamples[i].t = static_cast<double>(i);
    }

    std::vector<DocNode> children;
    children.push_back(makeInkChild(stroke.id, "boundary", boundarySamples, style, std::nullopt));

    std::vector<std::string> captureIds;
    std::vector<std::string> childIdsForLog;
    childIdsForLog.push_back(stroke.id);
    for (auto &cap : capturable) {
        for (size_t i = 0; i < cap.samples.size(); ++i) {
            cap.samples[i].x -= world.x;
            cap.samples[i].y -= world.y;
            if (!cap.samples[i].t)
                cap.samples[i].t = static_cast<double>(i);
        }
        const auto uv = seedLayoutOffset(cap.samples, bounds);
        children.push_back(makeInkChild(cap.id, "content", cap.samples, cap.style, uv));
        captureIds.push_back(cap.id);
        childIdsForLog.push_back(cap.id);
    }
    for (const DocNode *sg : capturedSgs) {
        if (!sg)
            continue;
        captureSmartGroupInto(*sg, world.x, world.y, bounds, &children, &captureIds);
        childIdsForLog.push_back(sg->id);
    }

    const std::string smartGroupId = std::string("sg_enclose_") + stroke.id;
    CreateSmartGroupEdit edit;
    edit.setId(std::string("create_smart_group:") + smartGroupId);
    edit.setSource(stroke.source);
    edit.setNodeId(smartGroupId);
    edit.setBounds(bounds);
    SmartTransform xf;
    xf.x = world.x;
    xf.y = world.y;
    edit.setTransform(xf);
    edit.setInkScaleMode("fixedInk");
    edit.setCaptureIds(std::move(captureIds));
    edit.setChildren(std::move(children));
    edit.setBoundaryPolyline(closedPolylineCopy(boundarySamples));

    const ApplyResult r = doc.commitEdit(edit);
    if (!r.applied) {
        appendOrdinaryInk(doc, stroke);
        out.kind = EncloseKind::OrdinaryInk;
        out.reason = r.reason.empty() ? "create_failed" : r.reason;
        return out;
    }
    out.kind = EncloseKind::Created;
    out.smartGroupId = smartGroupId;
    out.childIds = std::move(childIdsForLog);
    return out;
}

struct EncloseIngestTiming {
    std::int64_t ns = 0;
    EncloseResult result;
    ApplyResult apply;
};

/**
 * Map panel samples to world, then dispatch on the pen-down latch.
 * @implements [SRS-EP-10] pen-up ingest after pixels (never in paint)
 */
inline EncloseIngestTiming ingestStrokeAtPenUp(DeviceDocument &doc, const FinishedStroke &stroke,
                                               const PanelToWorld &map, StrokeArmedTool armedAtPenDown)
{
    EncloseIngestTiming timing;
    const auto t0 = std::chrono::steady_clock::now();
    if (stroke.samples.size() < 2) {
        timing.result.kind = EncloseKind::Skipped;
        timing.result.reason = "too_few_samples";
        timing.apply = {false, "too_few_samples"};
        const auto t1 = std::chrono::steady_clock::now();
        timing.ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        return timing;
    }

    EncloseStrokeInput input;
    input.id = stroke.id;
    input.width = stroke.strokeWidthWorld;
    input.stroke = stroke.stroke;
    input.armedAtPenDown = armedAtPenDown;
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

    timing.result = commitStrokeWithEncloseRecognition(doc, input);
    timing.apply.applied = timing.result.kind != EncloseKind::Skipped;
    timing.apply.reason = timing.result.reason;
    const auto t1 = std::chrono::steady_clock::now();
    timing.ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    return timing;
}

} // namespace document
} // namespace epaper
