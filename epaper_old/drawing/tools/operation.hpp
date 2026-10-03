#pragma once

/**
 * Operation — locked gesture lifecycle (ADR-0033).
 * Prefer name Operation; internals may still use session/intent style.
 * @implements [SRS-EP-04]
 */

#include "strategy.hpp"

#include <cstdint>

class QPainter;

namespace epaper {
namespace tools {

enum class OperationKind : std::uint8_t {
    None = 0,
    InkStroke,
    Lasso,
    Marquee,
    Select,
    Move,
    Resize,
    Rotate,
    Navigation,
    BrushErase,
    AreaErase,
    ObjectErase,
};

struct OperationDescriptor {
    OperationKind kind = OperationKind::None;
    StrategyKind matchOn = StrategyKind::RawPointer;
    StrategyKind receive = StrategyKind::RawPointer;
    int priority = 0;
    bool acceptPrimary = true;
    bool acceptSecondary = true;
};

/**
 * Base Operation. Concrete ops also implement one strategy sink
 * (e.g. RawPointerSink) matching descriptor().receive.
 */
class Operation {
public:
    virtual ~Operation() = default;
    virtual OperationKind kind() const = 0;
    virtual const OperationDescriptor &descriptor() const = 0;
    /** Side-effect free; used before lock. */
    virtual bool match(StrategyKind channel, const PointerSample &s) const
    {
        (void)channel;
        (void)s;
        return true;
    }
    virtual void cancel() = 0;
    virtual void paintOverlay(QPainter *painter)
    {
        (void)painter;
    }
    virtual bool didMutateSelection() const { return false; }
};

} // namespace tools
} // namespace epaper
