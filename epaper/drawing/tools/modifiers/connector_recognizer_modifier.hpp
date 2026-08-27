#pragma once

/**
 * Connector recognizer — Pen Mode behavior modifier (chip toggle + latch query).
 * @implements [SRS-EP-04] @implements [ADR-0021]
 */

#include "tool_modifier.hpp"

#include "../../canvas_session.h"

namespace epaper {
namespace tools {

class ConnectorRecognizerModifier : public ToolModifier {
public:
    explicit ConnectorRecognizerModifier(CanvasSession *session)
        : m_session(session)
    {
    }

    bool armed() const override { return m_session && m_session->chip.recogConnector; }

    bool flip()
    {
        return m_session && m_session->flipRecogConnector();
    }

    bool latched() const { return m_session && m_session->chip.latchedConnector; }

private:
    CanvasSession *m_session = nullptr;
};

} // namespace tools
} // namespace epaper
