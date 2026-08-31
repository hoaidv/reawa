#pragma once

/**
 * Interaction Mode — object with enum id + state (ADR-0033).
 * @implements [SRS-EP-04]
 */

#include "host_caps.hpp"
#include "operation.hpp"

#include <vector>

class QPainter;

namespace epaper {
namespace tools {

enum class ModeId {
    Ink,
    Selection, // sel_rect / sel_freeform arms under this id
    Eraser,
};

class InputHub;

/** Filled by InputHub after a Secondary Operation ends. */
struct SecondaryCommitInfo {
    bool selectionNonEmpty = false;
    bool didMutateSelection = false;
};

class InteractionMode {
public:
    virtual ~InteractionMode() = default;
    virtual ModeId id() const = 0;
    virtual void activate(HostCaps &caps, InputHub &hub)
    {
        (void)caps;
        (void)hub;
    }
    virtual void deactivate(InputHub &hub) { (void)hub; }
    virtual std::vector<OperationKind> primaryOps() const { return {}; }
    virtual std::vector<OperationKind> secondaryOps() const { return {}; }
    virtual void onSecondaryCommit(HostCaps &caps, const SecondaryCommitInfo &info)
    {
        (void)caps;
        (void)info;
    }
    /** Mode owns overlay policy; may forward in-flight paint to a locked Operation. */
    virtual void paintOverlay(QPainter *painter, HostCaps &caps, InputHub &hub)
    {
        (void)painter;
        (void)caps;
        (void)hub;
    }
    /**
     * Overlay attach + Pen vs Mono waveform.
     * Idle / phase / exclusive-arm: Mode tells. Gesture start/end: Operation tells.
     */
    virtual void syncOverlay(HostCaps &caps, InputHub &hub)
    {
        (void)caps;
        (void)hub;
    }
    /**
     * Settled overlay / knobs / hits after a gesture or document change.
     * Policy is Mode-specific; ToolContextImpl only forwards.
     */
    virtual void refreshChrome(HostCaps &caps, InputHub &hub)
    {
        (void)caps;
        (void)hub;
    }
};

} // namespace tools
} // namespace epaper
