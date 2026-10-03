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
#include <memory>
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

    std::vector<InkSample> samples;
    samples.reserve(stroke.samples.size());
    for (size_t i = 0; i < stroke.samples.size(); ++i) {
        const DigitizerSample &s = stroke.samples[i];
        InkSample out;
        map(s.panelX, s.panelY, &out.x, &out.y);
        out.pressure = s.pressure;
        out.tiltX = s.tiltX;
        out.tiltY = s.tiltY;
        out.t = s.t ? s.t : std::optional<double>(static_cast<double>(i));
        out.timestamp = s.timestamp;
        out.distance = s.distance;
        out.extras = s.extras;
        samples.push_back(std::move(out));
    }

    Style style;
    style.stroke = stroke.stroke;
    style.strokeWidth = stroke.strokeWidthWorld;

    AppendInkEdit edit(stroke.id, std::move(samples), std::move(style));
    edit.setId(std::string("append_ink:") + stroke.id);
    edit.setSource(stroke.source);
    edit.setParentId(stroke.parentId);
    return doc.commitEdit(edit);
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
