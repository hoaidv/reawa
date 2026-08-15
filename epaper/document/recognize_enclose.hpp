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
#include "ingest_stroke.hpp"

#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace epaper {
namespace document {

/** ADR-0013 §6 / SRS-EP-10 — shorter side minimum in world units. */
constexpr double kMinEncloseWorld = 28;

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
                              const Vec2 *contentMin)
{
    const SmartTransform &t = sg.transform;
    double x = localX;
    double y = localY;
    if (role == "content" && sg.inkScaleMode == "fixedInk") {
        (void)layoutOffset;
        (void)contentMin;
        // Keep-size: unscaled, stay at local (x,y) in the parent — do not pin to (0,0).
        return {localX + t.x, localY + t.y};
    }
    if (role == "boundary" || sg.inkScaleMode == "withBounds") {
        x *= t.scaleX != 0 ? t.scaleX : 1.0;
        y *= t.scaleY != 0 ? t.scaleY : 1.0;
    }
    if (t.rotation != 0) {
        const double csn = std::cos(t.rotation);
        const double sn = std::sin(t.rotation);
        const double rx = x * csn - y * sn;
        const double ry = x * sn + y * csn;
        x = rx;
        y = ry;
    }
    return {x + t.x, y + t.y};
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

inline JsonValue inkChildToJson(const std::string &id, const std::string &role,
                                const std::vector<InkSample> &samples, const Style &style,
                                const std::optional<std::pair<double, double>> &layoutOffset)
{
    JsonValue::Object o;
    o.emplace_back("id", JsonValue::string(id));
    o.emplace_back("kind", JsonValue::string("ink"));
    o.emplace_back("samples", inkSamplesToJson(samples));
    o.emplace_back("style", styleToJson(style));
    o.emplace_back("role", JsonValue::string(role));
    if (layoutOffset) {
        JsonValue::Object lo;
        lo.emplace_back("u", JsonValue::number(layoutOffset->first));
        lo.emplace_back("v", JsonValue::number(layoutOffset->second));
        o.emplace_back("layoutOffset", JsonValue::object(std::move(lo)));
    }
    return JsonValue::object(std::move(o));
}

inline ApplyResult appendOrdinaryInk(DeviceDocument &doc, const EncloseStrokeInput &stroke)
{
    JsonValue::Object style;
    style.emplace_back("stroke", JsonValue::string(stroke.stroke));
    style.emplace_back("strokeWidth", JsonValue::number(stroke.width));

    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(stroke.id));
    payload.emplace_back("samples", inkSamplesToJson(stroke.samples));
    payload.emplace_back("style", JsonValue::object(std::move(style)));

    DocOp op;
    op.opId = std::string("append_ink:") + stroke.id;
    op.type = "append_ink";
    op.source = stroke.source;
    op.payload = JsonValue::object(std::move(payload));
    return doc.commitOp(op);
}

/**
 * Commit finished stroke. Ink-box latch → enclose; Pen → ordinary ink only (no membership).
 * Draw-into membership is ADR-0022 step 2 in recognizer_dispatch.hpp.
 * @implements [SRS-EP-10] stroke_end enclose path
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
    if (shorter < kMinEncloseWorld) {
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
    if (capturable.empty()) {
        // Blank-canvas closed box is a legal create (boundary only). A closed
        // stroke that already sits inside an existing group must fall through
        // to membership (D21 / CHL-0011 — no nested enclose).
        bool insideExisting = false;
        for (const auto &n : doc.rootChildren) {
            if (n.kind != NodeKind::SmartGroup)
                continue;
            const SmartBounds &b = n.smartBounds;
            const SmartTransform &t = n.transform;
            SmartBounds w;
            w.x = t.x + b.x * t.scaleX;
            w.y = t.y + b.y * t.scaleY;
            w.width = b.width * t.scaleX;
            w.height = b.height * t.scaleY;
            if (fractionSamplesInside(stroke.samples, w) >= 0.8) {
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

    JsonValue::Array children;
    children.push_back(inkChildToJson(stroke.id, "boundary", boundarySamples, style, std::nullopt));

    JsonValue::Array captureIds;
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
        children.push_back(inkChildToJson(cap.id, "content", cap.samples, cap.style, uv));
        captureIds.push_back(JsonValue::string(cap.id));
        childIdsForLog.push_back(cap.id);
    }

    JsonValue::Object b;
    b.emplace_back("x", JsonValue::number(bounds.x));
    b.emplace_back("y", JsonValue::number(bounds.y));
    b.emplace_back("width", JsonValue::number(bounds.width));
    b.emplace_back("height", JsonValue::number(bounds.height));

    JsonValue::Object transform;
    transform.emplace_back("x", JsonValue::number(world.x));
    transform.emplace_back("y", JsonValue::number(world.y));
    transform.emplace_back("rotation", JsonValue::number(0));
    transform.emplace_back("scaleX", JsonValue::number(1));
    transform.emplace_back("scaleY", JsonValue::number(1));

    const std::string smartGroupId = std::string("sg_enclose_") + stroke.id;
    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(smartGroupId));
    payload.emplace_back("bounds", JsonValue::object(std::move(b)));
    payload.emplace_back("transform", JsonValue::object(std::move(transform)));
    payload.emplace_back("inkScaleMode", JsonValue::string("fixedInk"));
    payload.emplace_back("captureIds", JsonValue::array(std::move(captureIds)));
    payload.emplace_back("children", JsonValue::array(std::move(children)));

    DocOp op;
    op.opId = std::string("create_smart_group:") + smartGroupId;
    op.type = "create_smart_group";
    op.source = stroke.source;
    op.payload = JsonValue::object(std::move(payload));

    const ApplyResult r = doc.commitOp(op);
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
