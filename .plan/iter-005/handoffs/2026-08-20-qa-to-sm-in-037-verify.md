---
from: qa
to: sm
date: 2026-08-20
iter: iter-005
cc: [dev, pm]
---

# Hand-off: Quality Assurance Engineer → Scrum Master — STORY-IN-037 verified

[STORY-IN-037](../stories/STORY-IN-037.md) (Infini follow Epaper — toggle, exclusion, disconnect) is **done**. No defect. Did not require [STORY-IN-033](../stories/STORY-IN-033.md) continuous tablet-pan apply. Did not touch `epaper/`. DEF-0004 unused (Infini lane; iter-005 defects folder empty).

## Result

**PASS.** All 6 Gherkin scenarios in [viewport-follow-infini.feature](../../../.docs/modules/infini/features/tablet-sync/bdd/viewport-follow-infini.feature) map to `infini/tests/viewport-follow-infini.test.ts`. `cd infini && npm test` — 20 files, **122 passed**, including 7 in `viewport-follow-infini.test.ts` (6 scenarios + FollowToggle chrome). Last-writer [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md) is absent from Infini follow code.

## Scenario mapping

| Feature scenario | Test |
|---|---|
| Creator turns Infini follow on from both-off | `emits viewport_follow epaper_to_infini, applies tablet viewport after settle, 0 doc_*` |
| Creator tap on Infini follow takes over from Epaper follow | `sets epaper_to_infini, peer off, 0 dual-on, apply after settle` |
| Connection lost forces Infini follow off | `forces none, toggle unavailable, 0 apply and 0 doc_* from the drop` |
| Reconnect does not restore Infini follow | `hello does not carry last follow.direction; stays off until click` |
| Creator pan on Infini while following turns follow off | `sets none before local pan applies and ignores further tablet viewport` |
| No session leaves the Infini follow toggle unavailable | `connection_lost, 0 follow-on, direction none` |

Extra chrome probe (not an extra scenario): `btn.viewport_follow is a desktop icon toggle not a ToolChip and not a child of WorldLayer`.

## UI spot-check ([UI-IN-04](../design/viewport-follow-infini/ui-spec.md))

`FollowToggle.tsx` + `viewportFollow.ts` + `CanvasStage` fragment + `app.css` vs Spec. **No Spec drift DEF.**

- **Regions** — `data-region="FollowToggle"` / `data-control="btn.viewport_follow"` in WindowFrame (`App` + CanvasStage fragment sibling of WorldLayer canvas). Not ToolChip, not WorldLayer child. Trailing cluster immediately leading StatusZoom; `--space-sm` 8 px gap; `--follow-hit: 32px`.
- **States** — `follow.off` / `following_epaper` / `peer_following_you` (pressed false, still tappable) / `local_nav_turns_off` / `connection_lost` (disabled) / `reconnect_stays_off` (enabled off). Copy table matches Spec (`FOLLOW_COPY`). Four distinct icons (off / on / peer / offline).
- **Pointer** — `:hover`, `:focus-visible`, `:active`, disabled opacity. Caption hides below 960 px; toggle stays. Native `type="button"` for Enter/Space.
- **No last-writer chrome.** No dual-on (enum). Apply-on-toggle uses cached tablet pose after settle; inbound pan-while-following remains [STORY-IN-033](../stories/STORY-IN-033.md).

Did not edit design HTML.

## Defects

None.

## Residual risks (not DEF)

- Live Electron↔RM2 TCP `:9877` not run. p95 ≤300 ms is `performance.now()` around `clickFollowToggle` / apply-cached, not a real panel.
- WorldLayer transform is session `lastAppliedTabletViewport` + `CanvasStage.applyFollowViewport`; no React mount test of the canvas crop.
- Spec focus order FollowToggle → CanvasStage: DOM has CanvasStage (`tabIndex={0}`) before the trailing cluster. Residual a11y, not AC/BDD.
- Continuous apply while the tablet pans is [STORY-IN-033](../stories/STORY-IN-033.md), not this sign-off.

## Asks

1. `/sm` — record IN-037 verified; do not start IN-033 until dispatched.
2. `/pm` may gate-close this story.

## Constraints

- Did not edit `epaper/`, `src/` (Infini implementation), PRD, SRS, MASTER, execution board, or design HTML.
- Did not `adlc rollup`. Did not git commit.
- Last-writer [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md) remains forbidden.

## Out of scope

IN-033 apply-while-tablet-pans. Epaper follow chrome (EP-055). `macOS/`.
