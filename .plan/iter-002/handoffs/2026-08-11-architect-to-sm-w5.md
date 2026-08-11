---
from: architect
to: sm
iter: iter-002
date: 2026-08-11
subject: w5-viewport-thicken-ready
verdict: READY-WITH-CONCERNS
cc: [qa, dev, pm]
---

# Architect → SM — W5 thicken complete

## Verdict: **READY-WITH-CONCERNS**

SRS + ADR closed for marker, publish coalesce, e-ink refresh, and stroke/zoom parity.
Stories IN-011 / EP-002 may move toward `ready` after QA BDD; Dev must replace legacy map
math where noted.

## What changed

| Artifact | Change |
|---|---|
| [SRS-IN-07](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md) | Tablet CSS frame → `drawingRegion`; marker only while gesturing; ≤30 Hz viewport coalesce + settle flush; world-width paint |
| [SRS-IN-08](../../../.docs/modules/infini/features/tablet-sync/srs-quality.md) | Marker, publish rate, ≤5% relative stroke parity, zoom-halve thickness |
| [SRS-EP-02](../../../.docs/modules/epaper/features/region-sync/srs-logic.md) | Panel→`drawingRegion` normalize; map immediate; paint uses `s_panel`; same-picture refresh |
| [SRS-EP-03](../../../.docs/modules/epaper/features/region-sync/srs-quality.md) | Refresh min **250 ms**; settle flush ≤100 ms; zoom parity ±5% |
| [ADR-0012](../../../.docs/adr/ADR-0012-world-stroke-viewport-parity.md) | **accepted** — world stroke width × viewport/panel scale |
| Infini `architecture.md` | Parity quality goal + W5 risks |

## Review (evidence-first)

### Strengths

- ADR-0009 map-before-refresh preserved; paint coalesce explicit.
- Stroke parity is measurable (relative thickness vs region / panel).
- Marker scoped as gesture affordance (no design story) — aligns PM Needs design: no.

### Concerns (accepted for W5)

| Id | Concern | Action |
|---|---|---|
| C1 | Reconnect `hello`/`snapshot` still TBD | Do not block W5 ACs; remain debt |
| C2 | Existing `viewport_map.hpp` `panelToWorld` uses Infini screen formula, not panel→region | **EP-002 must fix** to SRS-EP-02 normalize |
| C3 | Live EXP StrokeSync brush width may still be device-ish until session path owns paint | IN-011/EP-002 migrate; disable legacy map ownership when session connected |
| C4 | `TabletSession.publishViewport` currently uses full-window AABB | IN-011: emit tablet-frame AABB |

### Risks

- None new that block READY; C2/C3 are implement risks with closed specs.

## SM next

1. Flip IN-011 / EP-002 toward ready once BDD exists (or keep draft → `/qa` first per board).
2. Tell user: **`/qa`** BDD against thickened SRS, then **`/dev`** IN-011 → EP-002.
3. Do not open IN-010.
