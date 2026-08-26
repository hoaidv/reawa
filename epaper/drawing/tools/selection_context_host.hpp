#pragma once

/**
 * SelectionContext over SelectionSession — durable ids + derived phase.
 * Ephemeral marquee/lasso geometry stays on SelectionSession / Operations.
 * @implements [SRS-EP-11]
 */

#include "selection_context.hpp"
#include "../selection_session.hpp"

namespace epaper {
namespace tools {

class SelectionContextHost final : public SelectionContext {
public:
    void attach(epaper::selection::SelectionSession *session) { m_session = session; }

    SelectionPhase phase() const override
    {
        if (!m_session)
            return SelectionPhase::Idle;
        if (m_session->isMarqueeOrLasso())
            return SelectionPhase::Selecting;
        if (m_session->isLiveManip())
            return SelectionPhase::Transforming;
        if (!m_session->ids.empty())
            return SelectionPhase::Selected;
        return SelectionPhase::Idle;
    }

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
    }

    void setIds(const std::vector<std::string> &ids) override
    {
        if (m_session)
            m_session->setIds(ids);
    }

    epaper::selection::SelectionSession *session() const { return m_session; }

private:
    epaper::selection::SelectionSession *m_session = nullptr;
};

} // namespace tools
} // namespace epaper
