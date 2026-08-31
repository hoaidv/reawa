#pragma once

/**
 * SelectionContext — durable ids + phase (ADR-0033).
 * Ephemeral lasso/marquee geometry lives on Operations.
 * @implements [SRS-EP-11]
 */

#include "document/device_document.hpp"

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

    void retainExisting(const epaper::document::DeviceDocument &doc)
    {
        std::vector<std::string> keep;
        keep.reserve(m_ids.size());
        for (const std::string &id : m_ids) {
            if (doc.find(id))
                keep.push_back(id);
        }
        setIds(keep);
    }

private:
    std::vector<std::string> m_ids;
    std::string m_pickableId;
    SelectionPhase m_phase = SelectionPhase::Idle;
};

} // namespace tools
} // namespace epaper
