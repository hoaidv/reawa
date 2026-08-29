#pragma once

/**
 * AreaErase — stub; chip arms erase_area. Commit lands in EP-065.
 * @implements [SRS-EP-54] erase_area exclusive
 */

#include "../host_caps.hpp"
#include "../operation.hpp"

#include <QPointF>
#include <vector>

namespace epaper {
namespace tools {

class AreaEraseOperation final : public Operation, public RawPointerSink {
public:
    explicit AreaEraseOperation(HostCaps *caps)
        : m_caps(caps)
    {
        m_desc.kind = OperationKind::AreaErase;
        m_desc.matchOn = StrategyKind::RawPointer;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 20;
        m_desc.acceptPrimary = true;
        m_desc.acceptSecondary = false;
    }

    OperationKind kind() const override { return OperationKind::AreaErase; }
    const OperationDescriptor &descriptor() const override { return m_desc; }

    bool match(StrategyKind channel, const PointerSample &s) const override
    {
        (void)s;
        if (channel != StrategyKind::RawPointer)
            return false;
        if (!m_caps || !m_caps->toolUi)
            return false;
        return m_caps->toolUi->exclusiveTool() == QLatin1String("erase_area");
    }

    void onDown(const PointerSample &s) override
    {
        m_pts.clear();
        m_pts.push_back(s.panel);
    }
    void onMove(const PointerSample &s) override { m_pts.push_back(s.panel); }
    void onUp(const PointerSample &) override
    {
        m_pts.clear();
        if (m_caps && m_caps->toolUi)
            m_caps->toolUi->requestChromeRefresh();
    }
    void onCancel() override { cancel(); }
    void cancel() override { m_pts.clear(); }

private:
    HostCaps *m_caps = nullptr;
    std::vector<QPointF> m_pts;
    OperationDescriptor m_desc;
};

} // namespace tools
} // namespace epaper
