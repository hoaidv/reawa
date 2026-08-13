#pragma once
/**
 * Finished-stroke → append_ink. Runs at pen-up, never in paint().
 * @implements [SRS-EP-07] stroke ingestion
 * @implements [SRS-EP-09] world samples + digitizer channel retention
 */

#include "device_document.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace epaper {
namespace document {

struct DigitizerSample {
    double panelX = 0;
    double panelY = 0;
    std::optional<double> pressure;
    std::optional<double> tiltX;
    std::optional<double> tiltY;
    std::optional<double> distance;
    std::optional<double> timestamp;
    std::optional<double> t;
    std::map<std::string, JsonValue> extras;
};

struct FinishedStroke {
    std::string id;
    std::vector<DigitizerSample> samples;
    double strokeWidthWorld = 2.5;
    std::string stroke = "#1C2430";
    std::optional<std::string> parentId;
    std::string source = "epaper";
};

using PanelToWorld = std::function<void(double panelX, double panelY, double *worldX, double *worldY)>;

inline PanelToWorld identityWorldMap()
{
    return [](double px, double py, double *wx, double *wy) {
        *wx = px;
        *wy = py;
    };
}

/**
 * Map panel samples to world, store every reported channel, commit append_ink
 * through the undo-aware path. Failure leaves the tree unchanged (0 half-inserted nodes).
 * @implements [SRS-EP-07] pen-up ingest as append_ink
 */
inline ApplyResult ingestFinishedStroke(DeviceDocument &doc, const FinishedStroke &stroke,
                                        const PanelToWorld &map)
{
    if (stroke.samples.size() < 2)
        return {false, "too_few_samples"};

    JsonValue::Array samples;
    samples.reserve(stroke.samples.size());
    for (size_t i = 0; i < stroke.samples.size(); ++i) {
        const DigitizerSample &s = stroke.samples[i];
        double wx = s.panelX;
        double wy = s.panelY;
        map(s.panelX, s.panelY, &wx, &wy);
        JsonValue::Object o;
        o.emplace_back("x", JsonValue::number(wx));
        o.emplace_back("y", JsonValue::number(wy));
        auto pushOpt = [&](const char *k, const std::optional<double> &v) {
            if (v)
                o.emplace_back(k, JsonValue::number(*v));
        };
        pushOpt("pressure", s.pressure);
        pushOpt("tiltX", s.tiltX);
        pushOpt("tiltY", s.tiltY);
        if (s.t)
            pushOpt("t", s.t);
        else
            o.emplace_back("t", JsonValue::number(static_cast<double>(i)));
        pushOpt("timestamp", s.timestamp);
        pushOpt("distance", s.distance);
        if (!s.extras.empty()) {
            JsonValue::Object ex;
            for (const auto &kv : s.extras)
                ex.emplace_back(kv.first, kv.second);
            o.emplace_back("extras", JsonValue::object(std::move(ex)));
        }
        samples.push_back(JsonValue::object(std::move(o)));
    }

    JsonValue::Object style;
    style.emplace_back("stroke", JsonValue::string(stroke.stroke));
    style.emplace_back("strokeWidth", JsonValue::number(stroke.strokeWidthWorld));

    JsonValue::Object payload;
    payload.emplace_back("id", JsonValue::string(stroke.id));
    if (stroke.parentId && !stroke.parentId->empty())
        payload.emplace_back("parentId", JsonValue::string(*stroke.parentId));
    payload.emplace_back("samples", JsonValue::array(std::move(samples)));
    payload.emplace_back("style", JsonValue::object(std::move(style)));

    DocOp op;
    op.opId = std::string("append_ink:") + stroke.id;
    op.type = "append_ink";
    op.source = stroke.source;
    op.payload = JsonValue::object(std::move(payload));
    return doc.commitOp(op);
}

struct IngestTiming {
    std::int64_t ns = 0;
    ApplyResult result;
};

inline IngestTiming ingestFinishedStrokeTimed(DeviceDocument &doc, const FinishedStroke &stroke,
                                              const PanelToWorld &map)
{
    const auto t0 = std::chrono::steady_clock::now();
    ApplyResult r = ingestFinishedStroke(doc, stroke, map);
    const auto t1 = std::chrono::steady_clock::now();
    IngestTiming out;
    out.ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    out.result = r;
    return out;
}

} // namespace document
} // namespace epaper
