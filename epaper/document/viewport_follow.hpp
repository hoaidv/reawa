#pragma once

/**
 * Viewport-follow Infini — session enum, FollowToggle, disconnect.
 * @implements [SRS-EP-49] viewport-follow Infini session enum
 * @implements [SRS-EP-50] FollowToggle sibling of ToolChip
 * @implements [SRS-EP-51] follow exclusivity and map-apply quality
 */

#include "hand_touch.hpp"
#include "json_value.hpp"

#include <string>
#include <vector>

namespace epaper {
namespace follow {

using epaper::handtouch::FollowDirection;
using epaper::handtouch::WorldAabb;
using epaper::handtouch::followId;
using epaper::handtouch::parseFollow;
using epaper::handtouch::shouldApplyInboundViewport;
using epaper::document::JsonValue;
using epaper::document::parseJson;
using epaper::document::stringify;

/** Icon toggle id — not a ToolChip exclusive. */
constexpr const char *kFollowControlId = "btn.viewport_follow";
constexpr const char *kFollowRegion = "FollowToggle";
constexpr const char *kToolChipRegion = "ToolChip";
constexpr const char *kDeviceScreenRegion = "DeviceScreen";
constexpr int kToolChipExclusiveCount = 3;
constexpr double kFollowTileDu = 64.0;
constexpr double kFollowInsetDu = 8.0;
/** Trailing row, index 0 = USB (rightmost), 1 = Follow, 2 = Debug, 3 = Hand-touch. */
constexpr int kTrailingUsb = 0;
constexpr int kTrailingFollow = 1;
constexpr int kTrailingDebug = 2;
constexpr int kTrailingHandTouch = 3;
constexpr double kMapApplyBudgetMs = 100.0;
constexpr double kExclusivityBudgetMs = 300.0;

enum class FollowUiState {
    Off,
    FollowingInfini,
    PeerFollowingYou,
    ConnectionLost,
    ReconnectStaysOff
};

inline const char *uiStateId(FollowUiState s)
{
    switch (s) {
    case FollowUiState::FollowingInfini:
        return "follow.following_infini";
    case FollowUiState::PeerFollowingYou:
        return "follow.peer_following_you";
    case FollowUiState::ConnectionLost:
        return "follow.connection_lost";
    case FollowUiState::ReconnectStaysOff:
        return "follow.reconnect_stays_off";
    case FollowUiState::Off:
    default:
        return "follow.off";
    }
}

struct PanelRect {
    double x = 0;
    double y = 0;
    double w = 0;
    double h = 0;

    bool contains(double px, double py) const
    {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    bool intersects(const PanelRect &o) const
    {
        return x < o.x + o.w && x + w > o.x && y < o.y + o.h && y + h > o.y;
    }
};

/**
 * Trailing orientation-top row: Hand-touch | Debug | FollowToggle | UsbLink
 * (index 0 = rightmost USB).
 * @implements [SRS-EP-50] FollowToggle placement
 */
inline PanelRect trailingChromeTile(double panelW, double panelH, bool gutOnTop, int fromTrailing)
{
    PanelRect r;
    r.w = kFollowTileDu;
    r.h = kFollowTileDu;
    r.y = gutOnTop ? (panelH - kFollowInsetDu - kFollowTileDu) : kFollowInsetDu;
    r.x = panelW - kFollowInsetDu - kFollowTileDu
        - double(fromTrailing) * (kFollowTileDu + kFollowInsetDu);
    return r;
}

inline PanelRect usbLinkRect(double panelW, double panelH, bool gutOnTop)
{
    return trailingChromeTile(panelW, panelH, gutOnTop, kTrailingUsb);
}

inline PanelRect followToggleRect(double panelW, double panelH, bool gutOnTop)
{
    return trailingChromeTile(panelW, panelH, gutOnTop, kTrailingFollow);
}

inline PanelRect debugToggleRect(double panelW, double panelH, bool gutOnTop)
{
    return trailingChromeTile(panelW, panelH, gutOnTop, kTrailingDebug);
}

inline PanelRect handTouchToggleRect(double panelW, double panelH, bool gutOnTop)
{
    return trailingChromeTile(panelW, panelH, gutOnTop, kTrailingHandTouch);
}

/** Orientation-bottom log panel — 1/4 of panel height. */
inline PanelRect debugLogRect(double panelW, double panelH, bool gutOnTop)
{
    PanelRect r;
    r.x = 0;
    r.w = panelW;
    r.h = panelH * 0.25;
    r.y = gutOnTop ? 0.0 : (panelH - r.h);
    return r;
}

inline bool followIsInsideToolChip(const PanelRect &follow, const PanelRect &chip)
{
    return follow.x >= chip.x && follow.y >= chip.y && follow.x + follow.w <= chip.x + chip.w
        && follow.y + follow.h <= chip.y + chip.h;
}

inline std::string encodeViewportFollow(FollowDirection d, int seq)
{
    JsonValue::Object o;
    o.emplace_back("type", JsonValue::string("viewport_follow"));
    o.emplace_back("direction", JsonValue::string(followId(d)));
    o.emplace_back("seq", JsonValue::number(seq));
    return stringify(JsonValue::object(std::move(o)));
}

inline bool isDocType(const std::string &type)
{
    return type == "doc_load" || type == "doc_change" || type == "doc_snapshot";
}

inline bool isViewportFollowType(const std::string &type)
{
    return type == "viewport_follow";
}

struct ViewportFollowMsg {
    FollowDirection direction = FollowDirection::None;
    int seq = 0;
    bool ok = false;
};

inline ViewportFollowMsg parseViewportFollow(const JsonValue &msg)
{
    ViewportFollowMsg out;
    if (!msg.isObject() || msg.getString("type") != "viewport_follow")
        return out;
    out.direction = parseFollow(msg.getString("direction"));
    out.seq = static_cast<int>(msg.getNumber("seq"));
    out.ok = true;
    return out;
}

inline ViewportFollowMsg parseViewportFollowLine(const std::string &line)
{
    try {
        return parseViewportFollow(parseJson(line));
    } catch (...) {
        return {};
    }
}

struct FollowTapResult {
    FollowDirection direction = FollowDirection::None;
    bool emitted = false;
    bool appliedInfiniViewport = false;
    bool infiniFollowOn = false;
    int dualOnIntervals = 0;
};

/**
 * Session probe for viewport-follow-epaper.feature.
 * @implements [SRS-EP-49] mutually exclusive one-way follow
 */
struct FollowSession {
    FollowDirection direction = FollowDirection::None;
    int seq = 0;
    bool connected = false;
    bool lostWhileFollowing = false;
    std::string exclusiveTool = "pen";
    WorldAabb localCamera{};
    WorldAabb infiniViewport{};
    bool hasInfiniViewport = false;
    bool mapApplied = false;
    int viewportFollowEmitted = 0;
    int docMessagesFromFollow = 0;
    int dualOnIntervals = 0;
    int inboundApplied = 0;
    std::vector<std::string> outbound;
    bool restoredOnReconnect = false;

    bool infiniFollowOn() const { return direction == FollowDirection::EpaperToInfini; }
    bool epaperFollowOn() const { return direction == FollowDirection::InfiniToEpaper; }
    bool ariaPressed() const { return direction == FollowDirection::InfiniToEpaper; }
    bool ariaDisabled() const { return !connected; }
    bool tappable() const { return connected; }

    FollowUiState uiState() const
    {
        if (!connected)
            return FollowUiState::ConnectionLost;
        if (direction == FollowDirection::InfiniToEpaper)
            return FollowUiState::FollowingInfini;
        if (direction == FollowDirection::EpaperToInfini)
            return FollowUiState::PeerFollowingYou;
        if (lostWhileFollowing)
            return FollowUiState::ReconnectStaysOff;
        return FollowUiState::Off;
    }

    void noteOutbound(const std::string &line)
    {
        outbound.push_back(line);
        const auto msg = parseViewportFollowLine(line);
        if (msg.ok)
            ++viewportFollowEmitted;
        try {
            const JsonValue j = parseJson(line);
            if (isDocType(j.getString("type")))
                ++docMessagesFromFollow;
        } catch (...) {
        }
    }

    void emitFollow()
    {
        if (!connected)
            return;
        ++seq;
        noteOutbound(encodeViewportFollow(direction, seq));
    }

    void applyInfiniViewportIfFollowing()
    {
        if (direction != FollowDirection::InfiniToEpaper || !hasInfiniViewport)
            return;
        localCamera = infiniViewport;
        mapApplied = true;
        ++inboundApplied;
    }

    void cacheInfiniViewport(const WorldAabb &v)
    {
        infiniViewport = v;
        hasInfiniViewport = true;
    }

    FollowTapResult tapToggle()
    {
        FollowTapResult r;
        r.infiniFollowOn = infiniFollowOn();
        if (!connected) {
            r.direction = direction;
            return r;
        }
        if (direction == FollowDirection::InfiniToEpaper) {
            direction = FollowDirection::None;
            lostWhileFollowing = false;
            mapApplied = false;
            emitFollow();
            r.emitted = true;
            r.direction = direction;
            r.infiniFollowOn = false;
            return r;
        }
        // Off or peer-following-you → takeover to infini_to_epaper. Enum cannot be dual-on.
        direction = FollowDirection::InfiniToEpaper;
        lostWhileFollowing = false;
        emitFollow();
        applyInfiniViewportIfFollowing();
        r.emitted = true;
        r.appliedInfiniViewport = mapApplied;
        r.direction = direction;
        r.infiniFollowOn = false;
        r.dualOnIntervals = dualOnIntervals;
        return r;
    }

    bool adoptInbound(const ViewportFollowMsg &msg)
    {
        if (!msg.ok)
            return false;
        if (msg.seq < seq)
            return false;
        seq = msg.seq;
        direction = msg.direction;
        if (direction != FollowDirection::InfiniToEpaper)
            mapApplied = false;
        if (direction == FollowDirection::None)
            lostWhileFollowing = false;
        return true;
    }

    void onDisconnect()
    {
        lostWhileFollowing = (direction == FollowDirection::InfiniToEpaper) || lostWhileFollowing;
        direction = FollowDirection::None;
        connected = false;
        mapApplied = false;
    }

    void onReconnect()
    {
        connected = true;
        if (direction != FollowDirection::None)
            restoredOnReconnect = true;
        direction = FollowDirection::None;
        mapApplied = false;
    }

    bool tryApplyInboundViewport()
    {
        if (!shouldApplyInboundViewport(direction))
            return false;
        applyInfiniViewportIfFollowing();
        return true;
    }
};

inline FollowSession liveBothOff()
{
    FollowSession s;
    s.connected = true;
    s.direction = FollowDirection::None;
    s.exclusiveTool = "pen";
    s.localCamera = {0, 0, 100, 80};
    s.cacheInfiniViewport({40, 20, 180, 140});
    return s;
}

} // namespace follow
} // namespace epaper
