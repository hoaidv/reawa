#pragma once

/**
 * InputHub — Interaction Router demux (ADR-0033).
 * Unified pointer/pinch dispatch; Operations own gesture bodies.
 * @implements [SRS-EP-04] @implements [SRS-EP-21]
 * @implements [SRS-EP-11] SelectionMode tap vs travel
 */

#include "host_caps.hpp"
#include "interventions.hpp"
#include "mode.hpp"
#include "modifiers/secondary_device_modifier.hpp"
#include "operation.hpp"
#include "strategy.hpp"

#include <QPointF>
#include <memory>
#include <unordered_map>
#include <vector>

namespace epaper {
namespace tools {

class InputHub {
public:
    InputHub() = default;
    ~InputHub() = default;
    InputHub(const InputHub &) = delete;
    InputHub &operator=(const InputHub &) = delete;

    void setHostCaps(HostCaps caps) { m_caps = caps; }
    HostCaps &hostCaps() { return m_caps; }
    const HostCaps &hostCaps() const { return m_caps; }

    DeviceMap &deviceMap() { return m_devices; }
    const DeviceMap &deviceMap() const { return m_devices; }
    void setDeviceMap(DeviceMap map) { m_devices = std::move(map); }

    SecondaryDeviceModifier &secondary() { return m_secondary; }
    const SecondaryDeviceModifier &secondary() const { return m_secondary; }

    void setActiveMode(InteractionMode *mode) { m_activeMode = mode; }
    InteractionMode *activeMode() const { return m_activeMode; }

    Operation *lockedOperation() const { return m_lockedOp; }
    Operation *operation(OperationKind kind) const { return opFor(kind); }

    void dispatchHoverMove(const PointerSample &s);
    void dispatchHoverLeave();

    void setOperation(OperationKind kind, std::unique_ptr<Operation> op)
    {
        m_ops[static_cast<int>(kind)] = std::move(op);
    }

    void clearOperations() { m_ops.clear(); }

    void registerHitRegion(const HitRegion &r) { m_hits.push_back(r); }
    void clearHitRegions() { m_hits.clear(); }
    const HitRegion *overlayHitAt(const QPointF &panel) const;

    void registerIntervention(Intervention iv) { m_interventions.push_back(std::move(iv)); }
    void clearInterventions() { m_interventions.clear(); }
    void dispatchIntervention(InterventionGate gate);

    bool dispatchPointerDown(const PointerSample &s);
    bool dispatchPointerMove(const PointerSample &s);
    bool dispatchPointerUp(const PointerSample &s);
    void dispatchPointerCancel();
    void cancelAll();

    bool dispatchTap(const PointerSample &s);

    bool dispatchPinchBegin(qreal x, qreal y, qreal scale);
    bool dispatchPinchUpdate(qreal x, qreal y, qreal scale);
    void dispatchPinchEnd();

private:
    bool stampRole(PointerSample *s) const;
    bool kindAllowed(OperationKind kind, PointerRole role) const;
    Operation *opFor(OperationKind kind) const;
    Operation *matchOperation(StrategyKind channel, const PointerSample &s);
    StylusHoverSink *matchHoverSink(const PointerSample &s) const;
    void endHover();

    void feedRawDown(Operation *op, const PointerSample &s);
    void feedRawMove(Operation *op, const PointerSample &s);
    void feedRawUp(Operation *op, const PointerSample &s);
    void feedRawCancel(Operation *op);
    void runSecondaryCommit();
    bool selectionTravelDefer() const;
    void tapSelect(const PointerSample &s);

    HostCaps m_caps;
    DeviceMap m_devices;
    SecondaryDeviceModifier m_secondary;
    InteractionMode *m_activeMode = nullptr;
    Operation *m_lockedOp = nullptr;
    StylusHoverSink *m_hoverSink = nullptr;
    std::unordered_map<int, std::unique_ptr<Operation>> m_ops;
    std::vector<HitRegion> m_hits;
    std::vector<Intervention> m_interventions;
    bool m_travelDeferred = false;
    PointerSample m_downSample;
};

} // namespace tools
} // namespace epaper
