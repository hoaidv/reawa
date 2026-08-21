#pragma once

// @implements [SRS-EP-05] ToolChip 3 tools + hand-touch toggle + 2 recognizers + Undo/Redo
// @implements [SRS-EP-04] exclusive tools, recognizer toggles, pen-down latch
// @implements [SRS-EP-22] btn.hand_touch first primary tile

#include <string>

namespace epaper {
namespace toolchip {

constexpr double kTile = 64.0;
constexpr double kGap = 32.0;
constexpr double kPublish = 12.0;
constexpr double kHeight = 64.0;

inline double chipWidth()
{
    return kPublish + kTile * 4.0 + kGap + kTile * 2.0 + kGap + kTile * 2.0;
}

enum class Hit {
    None,
    Publish,
    Gap,
    HandTouch,
    SelRect,
    SelFreeform,
    Pen,
    RecogInkBox,
    RecogConnector,
    Undo,
    Redo
};

inline const char *hitId(Hit h)
{
    switch (h) {
    case Hit::Publish:
        return "publish";
    case Hit::Gap:
        return "gap";
    case Hit::HandTouch:
        return "tgl.hand_touch";
    case Hit::SelRect:
        return "sel_rect";
    case Hit::SelFreeform:
        return "sel_freeform";
    case Hit::Pen:
        return "pen";
    case Hit::RecogInkBox:
        return "tgl.recog.ink_box";
    case Hit::RecogConnector:
        return "tgl.recog.connector";
    case Hit::Undo:
        return "undo";
    case Hit::Redo:
        return "redo";
    case Hit::None:
    default:
        return "";
    }
}

inline Hit hitAtRelX(double relX)
{
    if (relX < 0.0 || relX >= chipWidth())
        return Hit::None;
    double x = relX;
    if (x < kPublish)
        return Hit::Publish;
    x -= kPublish;
    if (x < kTile)
        return Hit::HandTouch;
    x -= kTile;
    if (x < kTile)
        return Hit::SelRect;
    x -= kTile;
    if (x < kTile)
        return Hit::SelFreeform;
    x -= kTile;
    if (x < kTile)
        return Hit::Pen;
    x -= kTile;
    if (x < kGap)
        return Hit::Gap;
    x -= kGap;
    if (x < kTile)
        return Hit::RecogInkBox;
    x -= kTile;
    if (x < kTile)
        return Hit::RecogConnector;
    x -= kTile;
    if (x < kGap)
        return Hit::Gap;
    x -= kGap;
    if (x < kTile)
        return Hit::Undo;
    x -= kTile;
    if (x < kTile)
        return Hit::Redo;
    return Hit::None;
}

/** Device-local chip state. Exclusive tool is never a recognizer toggle. */
struct ChipModel {
    std::string exclusive = "pen";
    bool recogInkBox = true;
    bool recogConnector = true;
    std::string latchedTool = "pen";
    bool latchedInkBox = true;
    bool latchedConnector = true;
    bool strokeActive = false;

    bool isSelection() const
    {
        return exclusive == "sel_rect" || exclusive == "sel_freeform";
    }

    bool recogDimmed() const { return isSelection(); }

    bool setExclusive(const std::string &mode)
    {
        std::string next = mode;
        if (next == "selection")
            next = "sel_rect";
        if (next == "ink_box")
            next = "pen";
        if (next != "pen" && next != "sel_rect" && next != "sel_freeform")
            next = "pen";
        if (exclusive == next)
            return false;
        exclusive = next;
        return true;
    }

    bool flipRecogInkBox()
    {
        if (recogDimmed())
            return false;
        recogInkBox = !recogInkBox;
        return true;
    }

    bool flipRecogConnector()
    {
        if (recogDimmed())
            return false;
        recogConnector = !recogConnector;
        return true;
    }

    void latchPenDown()
    {
        latchedTool = exclusive;
        latchedInkBox = recogInkBox;
        latchedConnector = recogConnector;
        strokeActive = true;
    }

    void penUp() { strokeActive = false; }

    std::string dispatchTuple() const
    {
        std::string s = latchedTool;
        s += "|ink_box=";
        s += latchedInkBox ? "1" : "0";
        s += "|connector=";
        s += latchedConnector ? "1" : "0";
        return s;
    }
};

} // namespace toolchip
} // namespace epaper
