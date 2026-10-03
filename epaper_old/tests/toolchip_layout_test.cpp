/**
 * STORY-EP-028 / @SRS-EP-04 @SRS-EP-05 — ToolChip inventory, dimmed toggles, pen-down latch.
 * Host test, no Qt.
 */
#include "drawing/primary_toolbar.hpp"

#include <cmath>
#include <cstdio>
#include <string>

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);               \
            return 1;                                                                          \
        }                                                                                      \
    } while (0)

using epaper::toolchip::ChipModel;
using epaper::toolchip::Hit;
using epaper::toolchip::chipWidth;
using epaper::toolchip::hitAtRelX;
using epaper::toolchip::hitId;
using epaper::toolchip::kGap;
using epaper::toolchip::kPublish;
using epaper::toolchip::kTile;

int main()
{
    CHECK(kTile == 64.0);
    CHECK(std::fabs(chipWidth() - (12.0 + 64.0 * 4.0 + 32.0 + 64.0 * 2.0 + 32.0 + 64.0 * 3.0
                                   + 32.0 + 64.0 * 2.0))
          < 0.001);

    CHECK(hitAtRelX(0) == Hit::Publish);
    CHECK(hitAtRelX(kPublish + 1) == Hit::HandTouch);
    CHECK(std::string(hitId(Hit::HandTouch)) == "tgl.hand_touch");
    CHECK(hitAtRelX(kPublish + kTile + 1) == Hit::SelRect);
    CHECK(hitAtRelX(kPublish + kTile * 2 + 1) == Hit::SelFreeform);
    CHECK(hitAtRelX(kPublish + kTile * 3 + 1) == Hit::Pen);
    CHECK(std::string(hitId(hitAtRelX(kPublish + kTile * 3 + 1))) == "pen");

    const double afterTools = kPublish + kTile * 4.0;
    CHECK(hitAtRelX(afterTools + 1) == Hit::Gap);
    CHECK(hitAtRelX(afterTools + kGap + 1) == Hit::RecogInkBox);
    CHECK(hitAtRelX(afterTools + kGap + kTile + 1) == Hit::RecogConnector);
    CHECK(std::string(hitId(Hit::RecogInkBox)) == "tgl.recog.ink_box");
    CHECK(std::string(hitId(Hit::RecogConnector)) == "tgl.recog.connector");

    const double afterRecog = afterTools + kGap + kTile * 2.0;
    CHECK(hitAtRelX(afterRecog + 1) == Hit::Gap);
    CHECK(hitAtRelX(afterRecog + kGap + 1) == Hit::EraseBrush);
    CHECK(hitAtRelX(afterRecog + kGap + kTile + 1) == Hit::EraseArea);
    CHECK(hitAtRelX(afterRecog + kGap + kTile * 2 + 1) == Hit::EraseObject);
    CHECK(std::string(hitId(Hit::EraseBrush)) == "erase_brush");

    const double afterErase = afterRecog + kGap + kTile * 3.0;
    CHECK(hitAtRelX(afterErase + 1) == Hit::Gap);
    CHECK(hitAtRelX(afterErase + kGap + 1) == Hit::Undo);
    CHECK(hitAtRelX(afterErase + kGap + kTile + 1) == Hit::Redo);
    CHECK(hitAtRelX(chipWidth()) == Hit::None);
    CHECK(hitAtRelX(-1) == Hit::None);

    ChipModel chip;
    CHECK(chip.exclusive == "pen");
    CHECK(chip.recogInkBox);
    CHECK(chip.recogConnector);
    CHECK(!chip.recogDimmed());

    CHECK(chip.setExclusive("sel_rect"));
    CHECK(chip.exclusive == "sel_rect");
    CHECK(chip.recogDimmed());
    CHECK(chip.recogInkBox);
    CHECK(chip.recogConnector);
    CHECK(!chip.flipRecogInkBox());
    CHECK(!chip.flipRecogConnector());
    CHECK(chip.recogInkBox);
    CHECK(chip.recogConnector);

    CHECK(chip.setExclusive("pen"));
    CHECK(!chip.recogDimmed());
    CHECK(chip.recogInkBox);
    CHECK(chip.flipRecogConnector());
    CHECK(!chip.recogConnector);

    CHECK(!chip.setExclusive("ink_box"));
    CHECK(chip.exclusive == "pen");

    chip.recogConnector = true;
    chip.latchPenDown();
    CHECK(chip.flipRecogConnector());
    CHECK(!chip.recogConnector);
    CHECK(chip.latchedConnector);
    CHECK(chip.dispatchTuple() == "pen|ink_box=1|connector=1");
    chip.penUp();

    const std::string beforeUndo = chip.exclusive;
    CHECK(hitAtRelX(afterErase + kGap + 1) == Hit::Undo);
    CHECK(chip.exclusive == beforeUndo);

    CHECK(chip.lastUsedEraser == "erase_brush");
    CHECK(chip.setExclusive("erase_area"));
    CHECK(chip.exclusive == "erase_area");
    CHECK(chip.isEraser());
    CHECK(chip.recogDimmed());
    CHECK(chip.lastUsedEraser == "erase_area");
    CHECK(!chip.flipRecogInkBox());
    CHECK(chip.togglePenEraser());
    CHECK(chip.exclusive == "pen");
    CHECK(chip.togglePenEraser());
    CHECK(chip.exclusive == "erase_area");
    CHECK(chip.setExclusive("pen"));
    CHECK(chip.beginTempErase());
    CHECK(chip.exclusive == "erase_area");
    CHECK(chip.endTempErase());
    CHECK(chip.exclusive == "pen");
    CHECK(chip.setExclusive("erase_brush"));
    CHECK(!chip.beginTempErase());
    CHECK(chip.exclusive == "erase_brush");
    CHECK(chip.beginNibErase());
    CHECK(chip.exclusive == "erase_brush");
    CHECK(chip.endNibErase());
    CHECK(chip.exclusive == "erase_brush");

    std::printf("toolchip_layout_test ok tiles=64 chipW=%.0f\n", chipWidth());
    return 0;
}
