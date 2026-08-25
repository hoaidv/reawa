#pragma once

/**
 * Live pen-stroke session: sample buffer + origin-aware begin/append/end.
 * Qt-free. Concrete type — no virtuals on the sample path (SRS-EP-13 latency).
 *
 * Mutators return StrokeIntent; the canvas alone paints, syncs, and commits.
 * Finished strokes become document::FinishedStroke for ingest_stroke.hpp.
 */

#include "document/ingest_stroke.hpp"
#include "ingest_origin_guard.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace epaper {
namespace strokecapture {

/** Digitizer channels for one sample. Doubles — canvas converts from PenSample. */
struct Channels {
    double pressure = 0;
    bool hasTilt = false;
    double tiltX = 0;
    double tiltY = 0;
    bool hasDistance = false;
    double distance = 0;
    bool hasTimestamp = false;
    double timestamp = 0;
    bool hasRotation = false;
    double rotation = 0;
    bool hasTangential = false;
    double tangential = 0;
};

struct Sample {
    double panelX = 0;
    double panelY = 0;
    double rawX = 0;
    double rawY = 0;
    Channels ch;
};

enum class StrokeIntent : int {
    None = 0,
    CancelSettle = 1 << 0,
    BeginGesture = 1 << 1,
    LatchChip = 1 << 2,
    PreviewBegin = 1 << 3,
    PreviewPoint = 1 << 4,
    PreviewEnd = 1 << 5,
    EmitSegment = 1 << 6,
    FlushInk = 1 << 7,
    StrokeCountChanged = 1 << 8,
    IngestReady = 1 << 9,
    AbortGesture = 1 << 10,
    NotifyHistory = 1 << 11,
    FlushWire = 1 << 12,
    ChipPenUp = 1 << 13,
};

inline StrokeIntent operator|(StrokeIntent a, StrokeIntent b)
{
    return static_cast<StrokeIntent>(static_cast<int>(a) | static_cast<int>(b));
}

inline StrokeIntent &operator|=(StrokeIntent &a, StrokeIntent b)
{
    a = a | b;
    return a;
}

inline bool has(StrokeIntent mask, StrokeIntent bit)
{
    return (static_cast<int>(mask) & static_cast<int>(bit)) != 0;
}

inline double worldStrokeWidth(double pressure, double base = 2.5)
{
    const double p = std::clamp(pressure, 0.0, 1.0);
    return base * (0.7 + 0.3 * p);
}

struct StrokeResult {
    StrokeIntent intent = StrokeIntent::None;
    Sample segmentFrom{};
    Sample segmentTo{};
    bool hasSegment = false;
    Sample previewSample{};
    bool hasPreviewSample = false;
    /** Populated on end when IngestReady — buffer is cleared after this is filled. */
    document::FinishedStroke finished;
    bool hasFinished = false;
};

/**
 * Owns the live stroke buffer and origin awaiting flag.
 * panelH must be set by the canvas before contact/begin/append (origin tests).
 */
struct StrokeCapture {
    std::vector<Sample> current;
    Sample lastEmitted{};
    bool hasEmitted = false;
    bool active = false;
    bool previewSent = false;
    bool awaitingPlausiblePress = false;
    std::string activeStrokeId;
    int strokeSeq = 0;
    int strokeCount = 0;
    double activeWorldStrokeWidth = 2.5;
    double lastPanelX = 0;
    double lastPanelY = 0;
    double lastRawX = 0;
    double lastRawY = 0;
    double panelH = 1.0;
    static constexpr double kBaseWorldStroke = 2.5;

    void setPanelHeight(double h) { panelH = std::max(1.0, h); }

    void noteContact(double panelX, double panelY, double rawX, double rawY)
    {
        lastPanelX = panelX;
        lastPanelY = panelY;
        lastRawX = rawX;
        lastRawY = rawY;
    }

    /** Origin-guard for one contact. Mutates awaitingPlausiblePress. */
    ingest::OriginGuardAction guardContact(bool isPress, bool isMove, bool isRelease,
                                           bool staleOrigin)
    {
        return ingest::decideOriginPress(isPress, isMove, isRelease, staleOrigin,
                                         &awaitingPlausiblePress);
    }

    bool sampleStale(double panelX, double panelY, double rawX, double rawY) const
    {
        return ingest::isStaleOriginSample(rawX, rawY, panelX, panelY, panelH);
    }

    Sample makeSample(double panelX, double panelY, const Channels &ch) const
    {
        Sample s;
        s.panelX = panelX;
        s.panelY = panelY;
        s.rawX = lastRawX;
        s.rawY = lastRawY;
        s.ch = ch;
        return s;
    }

    StrokeResult begin(double panelX, double panelY, const Channels &ch)
    {
        StrokeResult r;
        r.intent = StrokeIntent::CancelSettle | StrokeIntent::BeginGesture
            | StrokeIntent::LatchChip;

        current.clear();
        activeStrokeId = std::string("s-") + std::to_string(++strokeSeq);
        activeWorldStrokeWidth = worldStrokeWidth(ch.pressure, kBaseWorldStroke);
        const Sample s = makeSample(panelX, panelY, ch);
        current.push_back(s);
        lastEmitted = s;
        hasEmitted = false;
        active = true;
        previewSent = false;

        if (sampleStale(panelX, panelY, lastRawX, lastRawY))
            return r;

        r.intent |= StrokeIntent::PreviewBegin | StrokeIntent::PreviewPoint
            | StrokeIntent::EmitSegment | StrokeIntent::FlushInk;
        r.hasSegment = true;
        r.segmentFrom = s;
        r.segmentTo = s;
        r.hasPreviewSample = true;
        r.previewSample = s;
        previewSent = true;
        hasEmitted = true;
        return r;
    }

    /**
     * @p flushDue — canvas timer said a flush is due (≥ kFlushIntervalMs).
     * First painted sample always flushes regardless.
     */
    StrokeResult append(double panelX, double panelY, const Channels &ch, bool flushDue)
    {
        StrokeResult r;
        if (!active || current.empty())
            return r; // canvas must call begin() first

        if (sampleStale(panelX, panelY, lastRawX, lastRawY))
            return r;

        if (current.size() == 1
            && ingest::isImplausibleOriginJump(current.front().panelX, current.front().panelY,
                                               panelX, panelY, panelH)) {
            // Replace stale origin start; do not emit the diagonal.
            const Sample s = makeSample(panelX, panelY, ch);
            current[0] = s;
            lastEmitted = s;
            if (!previewSent) {
                r.intent |= StrokeIntent::PreviewBegin;
                previewSent = true;
            }
            r.intent |= StrokeIntent::PreviewPoint | StrokeIntent::EmitSegment
                | StrokeIntent::FlushInk;
            r.hasPreviewSample = true;
            r.previewSample = s;
            r.hasSegment = true;
            r.segmentFrom = s;
            r.segmentTo = s;
            hasEmitted = true;
            return r;
        }

        const Sample next = makeSample(panelX, panelY, ch);
        current.push_back(next);
        if (!previewSent) {
            r.intent |= StrokeIntent::PreviewBegin;
            previewSent = true;
        }
        r.intent |= StrokeIntent::PreviewPoint | StrokeIntent::EmitSegment;
        r.hasPreviewSample = true;
        r.previewSample = next;
        r.hasSegment = true;
        r.segmentFrom = lastEmitted;
        r.segmentTo = next;
        lastEmitted = next;
        if (!hasEmitted || flushDue) {
            hasEmitted = true;
            r.intent |= StrokeIntent::FlushInk;
        }
        return r;
    }

    StrokeResult end()
    {
        StrokeResult r;
        if (!active)
            return r;

        // Order the canvas must honor: segment → previewEnd → flush → ingest → teardown.
        if (current.size() >= 2) {
            const Sample &last = current.back();
            if (last.panelX != lastEmitted.panelX || last.panelY != lastEmitted.panelY) {
                r.intent |= StrokeIntent::EmitSegment;
                r.hasSegment = true;
                r.segmentFrom = lastEmitted;
                r.segmentTo = last;
            }
            ++strokeCount;
            r.intent |= StrokeIntent::StrokeCountChanged | StrokeIntent::PreviewEnd;
            r.finished = buildFinished();
            r.hasFinished = true;
            r.intent |= StrokeIntent::IngestReady;
        }

        r.intent |= StrokeIntent::FlushInk | StrokeIntent::AbortGesture
            | StrokeIntent::NotifyHistory | StrokeIntent::FlushWire | StrokeIntent::ChipPenUp;

        current.clear();
        hasEmitted = false;
        active = false;
        previewSent = false;
        return r;
    }

    /** Build the FinishedStroke for ingest_stroke / recog dispatch. Empty if <2 samples. */
    document::FinishedStroke buildFinished() const
    {
        document::FinishedStroke stroke;
        stroke.id = activeStrokeId;
        stroke.strokeWidthWorld = activeWorldStrokeWidth;
        if (current.size() < 2)
            return stroke;
        stroke.samples.reserve(current.size());
        for (size_t i = 0; i < current.size(); ++i) {
            const Sample &pt = current[i];
            document::DigitizerSample d;
            d.panelX = pt.panelX;
            d.panelY = pt.panelY;
            d.pressure = pt.ch.pressure;
            if (pt.ch.hasTilt) {
                d.tiltX = pt.ch.tiltX;
                d.tiltY = pt.ch.tiltY;
            }
            if (pt.ch.hasDistance)
                d.distance = pt.ch.distance;
            if (pt.ch.hasTimestamp)
                d.timestamp = pt.ch.timestamp;
            d.t = double(i);
            if (pt.ch.hasRotation)
                d.extras.emplace("rotation", document::JsonValue::number(pt.ch.rotation));
            if (pt.ch.hasTangential)
                d.extras.emplace("tangentialPressure",
                                 document::JsonValue::number(pt.ch.tangential));
            stroke.samples.push_back(std::move(d));
        }
        return stroke;
    }
};

} // namespace strokecapture
} // namespace epaper
