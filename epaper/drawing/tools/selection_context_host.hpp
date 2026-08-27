#pragma once

/**
 * SelectionContext over SelectionSession — durable ids + explicit phase.
 * Ephemeral marquee/lasso geometry lives on Operations.
 * @implements [SRS-EP-11]
 */

#include "selection_context.hpp"
#include "../selection_session.hpp"

namespace epaper {
namespace tools {

class SelectionContextHost final : public SelectionContext {
public:
    void attach(epaper::selection::SelectionSession *session) { m_session = session; }

    SelectionPhase phase() const override { return m_phase; }

    const std::vector<std::string> &ids() const override
    {
        static const std::vector<std::string> kEmpty;
        return m_session ? m_session->ids : kEmpty;
    }

    const std::string &pickableId() const override
    {
        static const std::string kEmpty;
        return m_session ? m_session->pickableId : kEmpty;
    }

    void clear() override
    {
        if (m_session)
            m_session->clear();
        m_phase = SelectionPhase::Idle;
    }

    void setIds(const std::vector<std::string> &ids) override
    {
        if (m_session)
            m_session->setIds(ids);
        if (m_phase != SelectionPhase::Transforming && m_phase != SelectionPhase::Selecting)
            m_phase = ids.empty() ? SelectionPhase::Idle : SelectionPhase::Selected;
    }

    void setPhase(SelectionPhase phase) override { m_phase = phase; }

    epaper::selection::SelectionSession *session() const { return m_session; }

private:
    epaper::selection::SelectionSession *m_session = nullptr;
    SelectionPhase m_phase = SelectionPhase::Idle;
};

} // namespace tools
} // namespace epaper
