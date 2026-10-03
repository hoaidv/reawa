#pragma once

/**
 * Stylus near vs contact, from evdev SYN_REPORT packets.
 *
 * Qt's evdevtablet plugin only synthesizes QTabletEvent while BTN_TOUCH is (or
 * just was) down — hover ABS moves never become TabletMove. This tracker is the
 * host-testable half of that gap: BTN_TOOL_PEN/RUBBER + ABS without BTN_TOUCH
 * is near-and-moving.
 * @implements [SRS-EP-21] pen near
 * @implements [SRS-EP-56] brush hover follows pen near
 */

#include <cstdint>

namespace epaper {
namespace input {

/** linux/input.h values — duplicated so host tests compile off-device. */
constexpr uint16_t kEvSyn = 0x00;
constexpr uint16_t kEvKey = 0x01;
constexpr uint16_t kEvAbs = 0x03;
constexpr uint16_t kSynReport = 0;
constexpr uint16_t kAbsX = 0x00;
constexpr uint16_t kAbsY = 0x01;
constexpr uint16_t kBtnToolPen = 0x140;
constexpr uint16_t kBtnToolRubber = 0x141;
constexpr uint16_t kBtnTouch = 0x14a;

enum class StylusPhase { Away, Near, Contact };

class StylusProximityTracker {
public:
    void setAbsRange(int minX, int maxX, int minY, int maxY)
    {
        m_minX = minX;
        m_maxX = maxX;
        m_minY = minY;
        m_maxY = maxY;
    }

    void feed(uint16_t type, uint16_t code, int32_t value)
    {
        if (type == kEvAbs) {
            if (code == kAbsX) {
                m_x = value;
                m_hasAbs = true;
            } else if (code == kAbsY) {
                m_y = value;
                m_hasAbs = true;
            }
        } else if (type == kEvKey) {
            if (code == kBtnTouch)
                m_touching = value != 0;
            else if (code == kBtnToolPen || code == kBtnToolRubber)
                m_tool = value != 0;
        }
    }

    StylusPhase phase() const
    {
        if (!m_tool)
            return StylusPhase::Away;
        if (m_touching)
            return StylusPhase::Contact;
        return StylusPhase::Near;
    }

    bool hasPosition() const { return m_hasAbs; }

    /** Same normalization as Qt evdevtablet: (abs-min)/(max-min) × window. */
    void windowPos(double winW, double winH, double *x, double *y) const
    {
        const double dx = static_cast<double>(m_maxX - m_minX);
        const double dy = static_cast<double>(m_maxY - m_minY);
        const double nx = dx != 0.0 ? (static_cast<double>(m_x) - m_minX) / dx : 0.0;
        const double ny = dy != 0.0 ? (static_cast<double>(m_y) - m_minY) / dy : 0.0;
        if (x)
            *x = nx * winW;
        if (y)
            *y = ny * winH;
    }

    /** Hover circle: near, has a tip sample, and Qt is not already in contact. */
    bool shouldEmitHover(bool qtPenDown) const
    {
        return phase() == StylusPhase::Near && m_hasAbs && !qtPenDown;
    }

private:
    int m_minX = 0;
    int m_maxX = 1;
    int m_minY = 0;
    int m_maxY = 1;
    int m_x = 0;
    int m_y = 0;
    bool m_hasAbs = false;
    bool m_tool = false;
    bool m_touching = false;
};

} // namespace input
} // namespace epaper
