#pragma once

/**
 * Ink-box recognizer — Pen Mode behavior modifier (chip toggle + latch query).
 * Not an InputHub sink; pen-up dispatch still on Tablet ingest path.
 * @implements [SRS-EP-04] @implements [ADR-0022]
 */

#include "tool_modifier.hpp"

#include "../../canvas_session.h"

namespace epaper {
namespace tools {

class InkBoxRecognizerModifier : public ToolModifier {
public:
    explicit InkBoxRecognizerModifier(CanvasSession *session)
        : m_session(session)
    {
    }

    bool armed() const override { return m_session && m_session->chip.recogInkBox; }

    bool flip()
    {
        return m_session && m_session->flipRecogInkBox();
    }

    bool latched() const { return m_session && m_session->chip.latchedInkBox; }

private:
    CanvasSession *m_session = nullptr;
};

} // namespace tools
} // namespace epaper
