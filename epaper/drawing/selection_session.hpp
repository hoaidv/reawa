#pragma once

/**
 * Selection + marquee/lasso session. Qt-free.
 * Mutators return SelectionIntent; the canvas alone damages chrome / ToolCanvas.
 */

#include "document/device_document.hpp"
#include "document/surround_create.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

namespace epaper {
namespace selection {

enum class Gesture { None, Move, Resize, Marquee, Lasso };

enum class SelectionIntent : int {
    None = 0,
    ChromeChanged = 1 << 0,
    SyncToolCanvas = 1 << 1,
    StrokeWaveformOn = 1 << 2,
    StrokeWaveformOff = 1 << 3,
    DamageLive = 1 << 4,
    DamageSegment = 1 << 5,
    RefreshChrome = 1 << 6,
    DebugChanged = 1 << 7,
    ResetDrag = 1 << 8,
};

inline SelectionIntent operator|(SelectionIntent a, SelectionIntent b)
{
    return static_cast<SelectionIntent>(static_cast<int>(a) | static_cast<int>(b));
}
inline SelectionIntent &operator|=(SelectionIntent &a, SelectionIntent b)
{
    a = a | b;
    return a;
}
inline bool has(SelectionIntent mask, SelectionIntent bit)
{
    return (static_cast<int>(mask) & static_cast<int>(bit)) != 0;
}

struct PanelPt {
    double x = 0;
    double y = 0;
};

struct SelectionResult {
    SelectionIntent intent = SelectionIntent::None;
    PanelPt damageA{};
    PanelPt damageB{};
    bool hasDamage = false;
    std::string debugInfo;
};

/** Panel → world for finish. Canvas supplies its frame transform. */
using PanelToWorldFn = std::function<void(double px, double py, double *wx, double *wy)>;

struct SelectionSession {
    Gesture gesture = Gesture::None;
    std::vector<std::string> ids;
    std::string pickableId;
    PanelPt marqueeStart{};
    PanelPt marqueeEnd{};
    std::vector<PanelPt> lasso;

    bool active() const { return gesture != Gesture::None; }
    bool isMarqueeOrLasso() const
    {
        return gesture == Gesture::Marquee || gesture == Gesture::Lasso;
    }
    bool isLiveManip() const
    {
        return gesture == Gesture::Move || gesture == Gesture::Resize;
    }

    void clear()
    {
        ids.clear();
        pickableId.clear();
    }

    void setIds(const std::vector<std::string> &next)
    {
        ids = next;
        pickableId = ids.empty() ? std::string{} : ids.front();
    }

    SelectionResult beginMarqueeOrLasso(double panelX, double panelY, bool freeform)
    {
        SelectionResult r;
        r.intent = SelectionIntent::ResetDrag | SelectionIntent::ChromeChanged
            | SelectionIntent::SyncToolCanvas | SelectionIntent::StrokeWaveformOn
            | SelectionIntent::DamageLive;
        marqueeStart = {panelX, panelY};
        marqueeEnd = {panelX, panelY};
        lasso.clear();
        lasso.push_back(marqueeStart);
        gesture = freeform ? Gesture::Lasso : Gesture::Marquee;
        r.hasDamage = true;
        r.damageA = marqueeStart;
        r.damageB = marqueeStart;
        return r;
    }

    SelectionResult updateMarquee(double panelX, double panelY)
    {
        SelectionResult r;
        if (gesture != Gesture::Marquee)
            return r;
        marqueeEnd = {panelX, panelY};
        r.intent = SelectionIntent::DamageLive;
        r.hasDamage = true;
        r.damageA = marqueeStart;
        r.damageB = marqueeEnd;
        return r;
    }

    SelectionResult updateLasso(double panelX, double panelY)
    {
        SelectionResult r;
        if (gesture != Gesture::Lasso)
            return r;
        const PanelPt prev = lasso.empty() ? PanelPt{panelX, panelY} : lasso.back();
        const PanelPt next{panelX, panelY};
        lasso.push_back(next);
        marqueeEnd = next;
        r.intent = SelectionIntent::DamageSegment;
        r.hasDamage = true;
        r.damageA = prev;
        r.damageB = next;
        return r;
    }

    /**
     * Resolve marquee/lasso. Maps panel points via @p map, runs selectBy*.
     * Always clears gesture to None and empties lasso.
     */
    SelectionResult finish(double minGestureSize, const document::DeviceDocument &doc,
                           const PanelToWorldFn &map)
    {
        SelectionResult r;
        r.intent = SelectionIntent::StrokeWaveformOff | SelectionIntent::RefreshChrome
            | SelectionIntent::DebugChanged;
        const Gesture kind = gesture;

        double gestureSize =
            std::hypot(marqueeEnd.x - marqueeStart.x, marqueeEnd.y - marqueeStart.y);
        if (kind == Gesture::Lasso && lasso.size() >= 2) {
            double pathLen = 0;
            double minX = lasso[0].x, maxX = lasso[0].x, minY = lasso[0].y, maxY = lasso[0].y;
            for (size_t i = 1; i < lasso.size(); ++i) {
                const double dx = lasso[i].x - lasso[i - 1].x;
                const double dy = lasso[i].y - lasso[i - 1].y;
                pathLen += std::sqrt(dx * dx + dy * dy);
                minX = std::min(minX, lasso[i].x);
                maxX = std::max(maxX, lasso[i].x);
                minY = std::min(minY, lasso[i].y);
                maxY = std::max(maxY, lasso[i].y);
            }
            const double diag = std::hypot(maxX - minX, maxY - minY);
            gestureSize = std::max(pathLen, diag);
        }

        if (gestureSize < minGestureSize) {
            clear();
            gesture = Gesture::None;
            lasso.clear();
            r.debugInfo = "sel=0 (tap)";
            return r;
        }

        std::vector<std::string> hit;
        if (kind == Gesture::Lasso) {
            std::vector<document::InkSample> poly;
            poly.reserve(lasso.size());
            for (const PanelPt &p : lasso) {
                document::InkSample s;
                map(p.x, p.y, &s.x, &s.y);
                poly.push_back(s);
            }
            hit = document::selectByFreeform(doc, poly);
        } else {
            double ax = 0, ay = 0, bx = 0, by = 0;
            map(marqueeStart.x, marqueeStart.y, &ax, &ay);
            map(marqueeEnd.x, marqueeEnd.y, &bx, &by);
            document::SmartBounds rect;
            rect.x = std::min(ax, bx);
            rect.y = std::min(ay, by);
            rect.width = std::abs(ax - bx);
            rect.height = std::abs(ay - by);
            hit = document::selectByRect(doc, rect);
        }

        lasso.clear();
        gesture = Gesture::None;
        setIds(hit);
        if (ids.empty())
            r.debugInfo = "sel=0 (no nodes ≥80% inside)";
        else {
            r.debugInfo = "sel=" + std::to_string(ids.size());
            for (const auto &id : ids)
                r.debugInfo += " " + id;
        }
        return r;
    }
};

} // namespace selection
} // namespace epaper
