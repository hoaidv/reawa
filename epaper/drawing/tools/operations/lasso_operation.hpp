#pragma once

/**
 * LassoOperation — RawPointer freeform select under SelectionMode.
 * @implements [SRS-EP-11] @implements [SRS-EP-12]
 */

#include "../operation.hpp"
#include "../selection_stroke_host.hpp"

namespace epaper {
namespace tools {

class LassoOperation final : public Operation, public RawPointerSink {
public:
    explicit LassoOperation(SelectionStrokeHost host)
        : m_host(std::move(host))
    {
        m_desc.kind = OperationKind::Lasso;
        m_desc.matchOn = StrategyKind::RawPointer;
        m_desc.receive = StrategyKind::RawPointer;
        m_desc.priority = 40;
        m_desc.acceptPen = true;
        m_desc.acceptFinger = true;
    }

    OperationKind kind() const override { return OperationKind::Lasso; }
    const OperationDescriptor &descriptor() const override { return m_desc; }

    bool match(StrategyKind channel, const PointerSample &s) const override
    {
        (void)s;
        return channel == StrategyKind::RawPointer;
    }

    void onDown(const PointerSample &s) override
    {
        if (!m_host.session || !m_host.applyIntent)
            return;
        m_host.applyIntent(m_host.session->beginMarqueeOrLasso(s.panel.x(), s.panel.y(), true));
    }

    void onMove(const PointerSample &s) override
    {
        if (!m_host.session || !m_host.applyIntent)
            return;
        m_host.applyIntent(m_host.session->updateLasso(s.panel.x(), s.panel.y()));
    }

    void onUp(const PointerSample &s) override
    {
        (void)s;
        finish();
    }

    void onCancel() override { cancel(); }

    void cancel() override
    {
        if (!m_host.session)
            return;
        m_host.session->lasso.clear();
        m_host.session->gesture = epaper::selection::Gesture::None;
        if (m_host.applyIntent) {
            epaper::selection::SelectionResult r;
            r.intent = epaper::selection::SelectionIntent::StrokeWaveformOff
                | epaper::selection::SelectionIntent::RefreshChrome;
            m_host.applyIntent(r);
        }
    }

private:
    void finish()
    {
        if (!m_host.session || !m_host.applyIntent || !m_host.document || !m_host.panelToWorld)
            return;
        m_host.applyIntent(m_host.session->finish(m_host.minGesture, m_host.document(),
                                                  m_host.panelToWorld));
    }

    SelectionStrokeHost m_host;
    OperationDescriptor m_desc;
};

} // namespace tools
} // namespace epaper
