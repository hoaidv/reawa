#pragma once

/**
 * SelectionContext — durable ids + phase (ADR-0033).
 * Ephemeral lasso/marquee geometry lives on Operations.
 * @implements [SRS-EP-11]
 */

#include <string>
#include <vector>

namespace epaper {
namespace tools {

enum class SelectionPhase {
    Idle,
    Selecting,
    Selected,
    Transforming,
};

class SelectionContext {
public:
    SelectionPhase phase() const { return m_phase; }
    const std::vector<std::string> &ids() const { return m_ids; }
    const std::string &pickableId() const { return m_pickableId; }

    void clear()
    {
        m_ids.clear();
        m_pickableId.clear();
        m_phase = SelectionPhase::Idle;
    }

    void setIds(const std::vector<std::string> &ids)
    {
        m_ids = ids;
        m_pickableId = m_ids.empty() ? std::string{} : m_ids.front();
        if (m_phase != SelectionPhase::Transforming && m_phase != SelectionPhase::Selecting)
            m_phase = m_ids.empty() ? SelectionPhase::Idle : SelectionPhase::Selected;
    }

    void setPhase(SelectionPhase phase) { m_phase = phase; }

private:
    std::vector<std::string> m_ids;
    std::string m_pickableId;
    SelectionPhase m_phase = SelectionPhase::Idle;
};

} // namespace tools
} // namespace epaper
