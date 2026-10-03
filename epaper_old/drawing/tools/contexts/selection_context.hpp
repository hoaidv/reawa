#pragma once

/**
 * SelectionContext — durable ids + phase (ADR-0033).
 * Ephemeral lasso/marquee geometry lives on Operations.
 * Paste origin is interaction memory, not a HostCaps field.
 * @implements [SRS-EP-11]
 * @implements [SRS-EP-32] tap-origin paste location
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

/** Last tap used as paste AABB top-left. Qt-free; panel/world in device units. */
struct PasteOrigin {
    bool valid = false;
    double panelX = 0;
    double panelY = 0;
    double worldX = 0;
    double worldY = 0;
};

class SelectionContext {
public:
    SelectionPhase phase() const { return m_phase; }
    const std::vector<std::string> &ids() const { return m_ids; }
    const std::string &pickableId() const { return m_pickableId; }
    const PasteOrigin &pasteOrigin() const { return m_pasteOrigin; }
    bool pasteOriginValid() const { return m_pasteOrigin.valid; }

    // @implements [SRS-EP-32] paste chrome only on Selected or idle empty-tap
    // @fix paste strip during live move — hidden while Transforming / Selecting
    bool pasteChromeVisible(bool slotNonEmpty) const
    {
        if (!slotNonEmpty || !m_pasteOrigin.valid)
            return false;
        if (m_phase == SelectionPhase::Selected)
            return true;
        return m_phase == SelectionPhase::Idle && m_ids.empty();
    }

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

    void setPasteOrigin(double panelX, double panelY, double worldX, double worldY)
    {
        m_pasteOrigin.valid = true;
        m_pasteOrigin.panelX = panelX;
        m_pasteOrigin.panelY = panelY;
        m_pasteOrigin.worldX = worldX;
        m_pasteOrigin.worldY = worldY;
    }

    void clearPasteOrigin() { m_pasteOrigin = {}; }

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
    PasteOrigin m_pasteOrigin;
};

} // namespace tools
} // namespace epaper
