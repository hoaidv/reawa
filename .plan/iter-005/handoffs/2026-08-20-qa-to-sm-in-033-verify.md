---
from: qa
to: sm
date: 2026-08-20
iter: iter-005
cc: [dev, pm]
---

# Hand-off: Quality Assurance Engineer → Scrum Master — STORY-IN-033 verified

[STORY-IN-033](../stories/STORY-IN-033.md) (Infini applies tablet viewport only while following) is **done**. No defect. [STORY-IN-037](../stories/STORY-IN-037.md) toggle tests still pass. Did not start W3. Campaign **pauses** for human Infini + Epaper deploy.

## Result

**PASS.** All 6 Gherkin scenarios in [viewport-follow-apply.feature](../../../.docs/modules/infini/features/infinity-canvas/bdd/viewport-follow-apply.feature) map 1:1 to `infini/tests/viewport-follow-apply.test.ts`. Command: `cd infini && npm test` — 21 files, **128 passed**, including **6** in `viewport-follow-apply.test.ts` and **7** in `viewport-follow-infini.test.ts` (IN-037 no regression). Last-writer [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md) is absent from Infini apply code.

## Scenario mapping

| Feature scenario | Result | Test |
|---|---|---|
| Tablet two-finger pan while Infini following matches after settle | **PASS** | `applies translate, drawingRegion, 0 competing down, 0 doc_*, follow stays on` |
| Tablet two-finger pinch while Infini following matches uniform scale after settle | **PASS** | `applies uniform scale, 0 rotation-skew, drawingRegion, 0 competing down` |
| Follow off leaves Infini camera unchanged when the tablet pans | **PASS** | `applies 0 inbound, 0 viewport either way, late inbound logged not implicit follow-on` |
| Infini as leader ignores inbound tablet viewport | **PASS** | `keeps local camera, logs ignore, 0 implicit follow-on, 0 doc_*` |
| Infini local pan while following turns follow off and leaves Epaper camera unchanged | **PASS** | `sets none before pan, 0 viewport down, 0 further tablet apply, 0 doc_*` |
| Infini local pinch while following turns follow off and leaves Epaper camera unchanged | **PASS** | `sets none before pinch, uniform 0.75, 0 viewport down, 0 further tablet apply` |

Not a UI story (`design_package` / `ui_spec` empty) — no Spec spot-check. Local-nav “none before gesture” is session `noteFollowerLocalNav` plus CanvasStage source-order (down/wheel before pan/pinch).

## Defects

None.

## Residual risks (not DEF)

- Live Electron↔RM2 TCP `:9877` / RM2 not run. Settle match is unit-level (`lastAppliedTabletViewport` + `publishViewport` null), not a live panel. Human field test after deploy.
- WorldLayer crop is session pose + `CanvasStage.applyFollowViewport` (translate/scale clone). No React mount test of the painted canvas crop.
- “Tablet pans while follow off” is Infini inbound ignore; Epaper 0-up remains [STORY-EP-039](../stories/STORY-EP-039.md).

## Asks

1. `/sm` — record IN-033 verified; **PAUSE** campaign for human deploy to Infini + Epaper. Do **not** start W3 (erase / Device Settings).
2. `/pm` may gate-close this story.

## Constraints

- Did not edit `epaper/`, Infini application sources, PRD, SRS, ADR, MASTER, execution board, tracks, or design HTML.
- Did not `adlc rollup`. Did not git commit.
- Last-writer [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md) remains forbidden.

## Out of scope

W3 (erase / Device Settings). Epaper follow apply ([STORY-EP-039](../stories/STORY-EP-039.md), already done). FollowToggle chrome ([STORY-IN-037](../stories/STORY-IN-037.md), already done). `macOS/`.
