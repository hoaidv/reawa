#pragma once

/**
 * InkStrokeOperation — pen RawPointer → InkSink.
 * @implements [SRS-EP-04]
 */

#include "../host_caps.hpp"
#include "../operation.hpp"
#include "../tablet_ink_sink.hpp"

namespace epaper {
namespace tools {

class InkStrokeOperation final : public Operation, public RawPointerSink {
public:
    explicit InkStrokeOperation(HostCaps *caps)
        : m_caps(caps)
    {
        m_desc.kind = OperationKind::InkStroke;
        m_desc.matchOn = StrategyKind::RawPointer;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 10;
        m_desc.acceptPen = true;
        m_desc.acceptFinger = false;
    }

    OperationKind kind() const override { return OperationKind::InkStroke; }
    const OperationDescriptor &descriptor() const override { return m_desc; }

    bool match(StrategyKind channel, const PointerSample &s) const override
    {
        return channel == StrategyKind::RawPointer && s.device == PointerDevice::Pen;
    }

    void onDown(const PointerSample &s) override
    {
        if (!ink())
            return;
        RawPt raw;
        auto ch = stash(s.panel, &raw);
        ch.pressure = s.pressure;
        ink()->ingestPen(QEvent::TabletPress, s.panel, raw, ch);
    }

    void onMove(const PointerSample &s) override
    {
        if (!ink())
            return;
        RawPt raw;
        auto ch = stash(s.panel, &raw);
        ch.pressure = s.pressure;
        ink()->ingestPen(QEvent::TabletMove, s.panel, raw, ch);
    }

    void onUp(const PointerSample &s) override
    {
        if (!ink())
            return;
        RawPt raw;
        const auto ch = stash(s.panel, &raw);
        ink()->ingestPen(QEvent::TabletRelease, s.panel, raw, ch);
        ink()->clearStash();
    }

    void onCancel() override { cancel(); }

    void cancel() override
    {
        if (!ink())
            return;
        if (ink()->strokeActive())
            ink()->cancelActiveStroke();
        ink()->clearStash();
    }

private:
    InkSink *ink() const { return m_caps ? m_caps->ink : nullptr; }

    epaper::input::PenSample stash(const QPointF &panel, RawPt *raw) const
    {
        auto *sink = dynamic_cast<TabletInkSink *>(ink());
        if (sink)
            return sink->stashedChannels(panel, raw);
        if (raw)
            *raw = {};
        return {};
    }

    HostCaps *m_caps = nullptr;
    OperationDescriptor m_desc;
};

} // namespace tools
} // namespace epaper
