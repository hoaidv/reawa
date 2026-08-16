/**
 * STORY-EP-033 / @SRS-EP-01 — origin/stale first-sample predicate.
 * Host test, no Qt.
 */
#include "ingest_origin_guard.hpp"

#include <cstdio>

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);               \
            return 1;                                                                          \
        }                                                                                      \
    } while (0)

using epaper::ingest::decideOriginPress;
using epaper::ingest::isImplausibleOriginJump;
using epaper::ingest::isStaleOriginSample;
using epaper::ingest::kOriginCanvasEps;
using epaper::ingest::kOriginJumpPx;
using epaper::ingest::OriginGuardAction;

int main()
{
    constexpr double kPanelH = 1872.0;

    // Digitizer origin: raw (0,0) → mapped (0, height).
    CHECK(isStaleOriginSample(0.0, 0.0, 0.0, kPanelH, kPanelH));
    CHECK(isStaleOriginSample(0.0, 0.0, 0.0, 1.0, 1.0));
    CHECK(isStaleOriginSample(1.0, 2.0, 400.0, 400.0, kPanelH));
    CHECK(isStaleOriginSample(100.0, 100.0, 0.0, kPanelH, kPanelH));
    CHECK(isStaleOriginSample(0.0, 0.0, 3.0, kPanelH - 4.0, kPanelH));
    CHECK(isStaleOriginSample(-0.5, -0.25, -1.0, kPanelH + 1.0, kPanelH));

    // Real contact is not origin (including bottom edge away from the corner).
    CHECK(!isStaleOriginSample(400.0, 300.0, 225.0, 1339.0, kPanelH));
    CHECK(!isStaleOriginSample(0.0, 500.0, 375.0, kPanelH, kPanelH));
    CHECK(!isStaleOriginSample(200.0, 0.0, 0.0, 1600.0, kPanelH));
    CHECK(!isStaleOriginSample(kOriginCanvasEps + 1.0, kOriginCanvasEps + 1.0, 40.0, 40.0,
                               kPanelH));

    // Jump from mapped origin must not be painted; ordinary first segments may.
    CHECK(isImplausibleOriginJump(0.0, kPanelH, 400.0, 200.0, kPanelH));
    CHECK(isImplausibleOriginJump(0.0, kPanelH, 0.0, kPanelH - (kOriginJumpPx + 1.0), kPanelH));
    CHECK(!isImplausibleOriginJump(0.0, kPanelH, 1.0, kPanelH - 1.0, kPanelH));
    CHECK(!isImplausibleOriginJump(200.0, 400.0, 900.0, 1600.0, kPanelH));

    bool awaiting = false;
    CHECK(decideOriginPress(true, false, false, true, &awaiting) == OriginGuardAction::Discard);
    CHECK(awaiting);
    CHECK(decideOriginPress(false, true, false, true, &awaiting) == OriginGuardAction::Discard);
    CHECK(awaiting);
    CHECK(decideOriginPress(false, true, false, false, &awaiting)
          == OriginGuardAction::PromoteToPress);
    CHECK(!awaiting);

    awaiting = false;
    CHECK(decideOriginPress(true, false, false, false, &awaiting) == OriginGuardAction::Proceed);
    CHECK(!awaiting);
    // Origin Move after a real Press must not paint tip → bottom-left.
    CHECK(decideOriginPress(false, true, false, true, &awaiting) == OriginGuardAction::Discard);
    CHECK(!awaiting);
    CHECK(decideOriginPress(false, true, false, false, &awaiting) == OriginGuardAction::Proceed);
    CHECK(!awaiting);

    awaiting = true;
    CHECK(decideOriginPress(false, false, true, false, &awaiting)
          == OriginGuardAction::DropContact);
    CHECK(!awaiting);

    awaiting = false;
    CHECK(decideOriginPress(false, false, true, false, &awaiting) == OriginGuardAction::Proceed);
    CHECK(!awaiting);

    awaiting = true;
    CHECK(decideOriginPress(true, false, false, false, &awaiting) == OriginGuardAction::Proceed);
    CHECK(!awaiting);

    std::printf("ingest_origin_guard_test OK\n");
    return 0;
}
