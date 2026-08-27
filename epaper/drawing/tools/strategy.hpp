#pragma once

/**
 * Event delivery strategies for Operations (ADR-0033 / tool system).
 * Router owns demux; Operations declare matchOn/receive — they do not bind Qt.
 * @implements [SRS-EP-04]
 */

#include <QPointF>
#include <QRectF>

namespace epaper {
namespace tools {

enum class StrategyKind {
    RawPointer,
    Drag,
    Tap,
    Pinch,
    HitTarget,
};

enum class PointerDevice {
    Pen,
    Finger,
    Mouse,
};

enum class PointerRole {
    Primary,
    Secondary,
};

struct PointerSample {
    QPointF panel;
    qreal pressure = 1.0;
    PointerDevice device = PointerDevice::Pen;
    PointerRole role = PointerRole::Primary;
};

struct DeviceMap {
    PointerDevice primary = PointerDevice::Pen;
    PointerDevice secondary = PointerDevice::Finger;

    bool tryRole(PointerDevice d, PointerRole *out) const
    {
        if (!out)
            return false;
        if (d == primary) {
            *out = PointerRole::Primary;
            return true;
        }
        if (d == secondary) {
            *out = PointerRole::Secondary;
            return true;
        }
        return false;
    }
};

/** Narrow sink — Operation implements at most one receive strategy. */
struct RawPointerSink {
    virtual ~RawPointerSink() = default;
    virtual void onDown(const PointerSample &s) = 0;
    virtual void onMove(const PointerSample &s) = 0;
    virtual void onUp(const PointerSample &s) = 0;
    virtual void onCancel() = 0;
};

struct TapSink {
    virtual ~TapSink() = default;
    virtual void onTap(const PointerSample &s) = 0;
};

struct PinchSink {
    virtual ~PinchSink() = default;
    virtual void onPinchBegin(const QPointF &centroid, qreal scale) = 0;
    virtual void onPinchUpdate(const QPointF &centroid, qreal scale) = 0;
    virtual void onPinchEnd() = 0;
};

/** Registered hit region for HitTarget strategy (panel space). */
struct HitRegion {
    QRectF panelRect;
    int priority = 0;
    void *ownerToken = nullptr; // Operation* or chrome id; opaque to hub
};

} // namespace tools
} // namespace epaper
