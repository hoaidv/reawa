#pragma once

/**
 * SelectOperation — tap pick or clear; no gesture machine.
 * @implements [SRS-EP-21] @implements [SRS-EP-23]
 */

#include "../host_caps.hpp"
#include "../operation.hpp"

namespace epaper {
namespace tools {

class SelectOperation final : public Operation, public TapSink {
public:
    explicit SelectOperation(HostCaps *caps)
        : m_caps(caps)
    {
        m_desc.kind = OperationKind::Select;
        m_desc.matchOn = StrategyKind::Tap;
        m_desc.receive = StrategyKind::Tap;
        m_desc.priority = 20;
        m_desc.acceptPrimary = false;
        m_desc.acceptSecondary = true;
    }

    OperationKind kind() const override { return OperationKind::Select; }
    const OperationDescriptor &descriptor() const override { return m_desc; }
    bool didMutateSelection() const override { return m_didMutate; }

    bool match(StrategyKind channel, const PointerSample &s) const override
    {
        (void)s;
        return channel == StrategyKind::Tap;
    }

    void onTap(const PointerSample &s) override
    {
        if (!m_caps || !m_caps->doc || !m_caps->toolUi || !m_caps->selection)
            return;
        m_didMutate = false;
        const auto w = m_caps->toolUi->panelToWorld(s.panel);
        const std::string id = m_caps->doc->hitSelectTarget(w.x, w.y);
        if (!id.empty()) {
            m_caps->selection->setIds({id});
            m_caps->selection->setPhase(SelectionPhase::Selected);
            m_didMutate = true;
        } else {
            m_caps->selection->clear();
            m_didMutate = true;
        }
        m_caps->toolUi->requestChromeRefresh();
    }

    void cancel() override {}

private:
    HostCaps *m_caps = nullptr;
    bool m_didMutate = false;
    OperationDescriptor m_desc;
};

} // namespace tools
} // namespace epaper
