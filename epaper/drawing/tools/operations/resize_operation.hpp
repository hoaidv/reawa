#pragma once

/**
 * ResizeOperation — knob HitTarget match, RawPointer drag; owns TransformGesture.
 * @implements [SRS-EP-11] @implements [SRS-EP-12]
 */

#include "../host_caps.hpp"
#include "../input_hub.hpp"
#include "../operation.hpp"
#include "transform_gesture.hpp"
#include "document/capability.hpp"
#include "document/manipulate.hpp"
#include "../ui/selection_overlay.hpp"

#include <cstdint>

namespace epaper {
namespace tools {

class ResizeOperation final : public Operation, public RawPointerSink {
public:
    ResizeOperation(HostCaps *caps, InputHub *hub)
        : m_caps(caps)
        , m_hub(hub)
    {
        m_desc.kind = OperationKind::Resize;
        m_desc.matchOn = StrategyKind::HitTarget;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 60;
        m_desc.acceptPrimary = true;
        m_desc.acceptSecondary = true;
    }

    OperationKind kind() const override { return OperationKind::Resize; }
    const OperationDescriptor &descriptor() const override { return m_desc; }
    bool didMutateSelection() const override { return m_gesture.didMutateSelection(); }

    bool match(StrategyKind channel, const PointerSample &s) const override
    {
        if (!m_hub || (channel != StrategyKind::HitTarget && channel != StrategyKind::RawPointer))
            return false;
        return m_hub->overlayHitAt(s.panel) != nullptr;
    }

    void onDown(const PointerSample &s) override
    {
        if (!m_caps || !m_caps->doc || !m_caps->toolUi || !m_caps->selection || !m_hub)
            return;
        m_gesture.resetMutate();
        if (m_caps->overlay)
            m_caps->overlay->clearManipUnavailable();
        const HitRegion *hit = m_hub->overlayHitAt(s.panel);
        const int idx = hit ? int(reinterpret_cast<intptr_t>(hit->ownerToken)) : -1;
        const epaper::document::ResizeHandle handle = handleFromIndex(idx);
        if (handle == epaper::document::ResizeHandle::None)
            return;
        const epaper::document::DocNode *selected =
            m_caps->doc->document().find(m_caps->selection->pickableId());
        if (!selected || !epaper::document::descriptorFor(selected->kind).has(epaper::document::Verb::Resize))
            return;
        epaper::document::SmartBounds wb;
        if (!epaper::document::boundsOf(*selected, wb))
            return;
        if (!m_caps->toolUi->lodOkPanel(wb)) {
            if (m_caps->overlay)
                m_caps->overlay->showManipUnavailable(*m_caps, wb);
            return;
        }
        const auto w = m_caps->toolUi->panelToWorld(s.panel);
        m_gesture.begin(m_caps, selected, handle, {w.x, w.y});
    }

    void onMove(const PointerSample &s) override
    {
        if (!m_caps || !m_caps->toolUi)
            return;
        const auto w = m_caps->toolUi->panelToWorld(s.panel);
        m_gesture.apply(m_caps, {w.x, w.y});
    }

    void onUp(const PointerSample &s) override
    {
        (void)s;
        m_gesture.commit(m_caps);
    }

    void onCancel() override { cancel(); }

    void cancel() override { m_gesture.abort(m_caps); }

private:
    HostCaps *m_caps = nullptr;
    InputHub *m_hub = nullptr;
    TransformGesture m_gesture;
    OperationDescriptor m_desc;
};

} // namespace tools
} // namespace epaper
