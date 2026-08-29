#pragma once

// @implements [SRS-EP-05] ToolChip exclusive tools + hand-touch + recognizers + Undo/Redo
// @implements [SRS-EP-04] exclusive tools, recognizer toggles, pen-down latch
// @implements [SRS-EP-22] btn.hand_touch first primary tile
// @implements [SRS-EP-54] six exclusives + eraser last-used + recog dim

#include <string>

namespace epaper {
namespace toolchip {

constexpr double kTile = 64.0;
constexpr double kGap = 32.0;
constexpr double kPublish = 12.0;
constexpr double kHeight = 64.0;

inline double chipWidth()
{
    return kPublish + kTile * 4.0 + kGap + kTile * 2.0 + kGap + kTile * 3.0 + kGap + kTile * 2.0;
}

inline bool isEraserId(const std::string &id)
{
    return id == "erase_brush" || id == "erase_area" || id == "erase_object";
}

inline bool isExclusiveId(const std::string &id)
{
    return id == "pen" || id == "sel_rect" || id == "sel_freeform" || isEraserId(id);
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
    EraseBrush,
    EraseArea,
    EraseObject,
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
    case Hit::EraseBrush:
        return "erase_brush";
    case Hit::EraseArea:
        return "erase_area";
    case Hit::EraseObject:
        return "erase_object";
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
        return Hit::EraseBrush;
    x -= kTile;
    if (x < kTile)
        return Hit::EraseArea;
    x -= kTile;
    if (x < kTile)
        return Hit::EraseObject;
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
    /** Last armed eraser; default brush. Persisted on-device, not in SVG. */
    std::string lastUsedEraser = "erase_brush";
    /** Hover circle while erase_brush + pen near. Kill-switch, default on. */
    bool eraseBrushHover = true;
    std::string tempRestore;
    std::string nibRestore;

    bool isSelection() const
    {
        return exclusive == "sel_rect" || exclusive == "sel_freeform";
    }

    bool isEraser() const { return isEraserId(exclusive); }

    bool recogDimmed() const { return isSelection() || isEraser(); }

    bool setExclusive(const std::string &mode)
    {
        std::string next = mode;
        if (next == "selection")
            next = "sel_rect";
        if (next == "ink_box")
            next = "pen";
        if (!isExclusiveId(next))
            next = "pen";
        if (isEraserId(next))
            lastUsedEraser = next;
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

    /** Pen ↔ last-used eraser. */
    bool togglePenEraser()
    {
        if (isEraser())
            return setExclusive("pen");
        return setExclusive(lastUsedEraser.empty() ? "erase_brush" : lastUsedEraser);
    }

    /** From pen only; already-in-eraser is a no-op. */
    bool beginTempErase()
    {
        if (isEraser())
            return false;
        if (!tempRestore.empty())
            return false;
        tempRestore = exclusive;
        setExclusive(lastUsedEraser.empty() ? "erase_brush" : lastUsedEraser);
        return true;
    }

    bool endTempErase()
    {
        if (tempRestore.empty())
            return false;
        const std::string prev = tempRestore;
        tempRestore.clear();
        setExclusive(prev);
        return true;
    }

    /** Force erase_brush while the inverted nib is reported. */
    bool beginNibErase()
    {
        if (!nibRestore.empty())
            return false;
        nibRestore = exclusive;
        setExclusive("erase_brush");
        return true;
    }

    bool endNibErase()
    {
        if (nibRestore.empty())
            return false;
        const std::string prev = nibRestore;
        nibRestore.clear();
        setExclusive(prev);
        return true;
    }

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
