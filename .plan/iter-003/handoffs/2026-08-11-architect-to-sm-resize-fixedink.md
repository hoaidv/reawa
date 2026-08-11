---
from: architect
to: sm
iter: iter-003
date: 2026-08-11
subject: verify-bugs-resize-fixedink-ghost
cc: [dev, qa, pm]
verdict: READY-WITH-CONCERNS
---

# Architect → SM — Resize / fixedInk / tablet ghost (verify #1–#4)

## Verdict

**READY-WITH-CONCERNS** — Spec clarified; implementation drifted. Open [CHL-0004](../challenges/CHL-0004-fixedink-resize-boundary.md) for PM adopt (restores BR-09i). No new ADR.

## Human feedback (mapped)

| # | Report | Architect diagnosis | Spec action |
|---|---|---|---|
| Pass | Move/select chrome + cross-device sync | Keep | — |
| **1** | Cannot resize on tablet | Handles are **paint-only**; no hit-test / `tool_intent resize` | SRS-EP-04 amended — resize required |
| **2** | Desktop resize scales content, boundary ink stuck | `fixedInk` resize mutates **local bounds only**; boundary draws via **scale+translate** → never stretches | SRS-IN-11 resize mapping locked; CHL-0004 |
| **3** | Not default `fixedInk` | Create paths set `fixedInk`, but resize behaviour looks like broken hybrid; no `tgl.ink_scale_mode` UI | Default reaffirmed in SRS; toggle gap → story |
| **4** | Tablet move: content doesn’t follow selection rect | Ghost is bounds chrome only | SRS-EP-04 — ghost = bounds **+ ink preview** |

## Ask SM (slice after PM adopt CHL-0004)

| Story | Kind | AC gist |
|---|---|---|
| **IN-023** | implement | `smartTransformFromWorldAabb(fixedInk)` = same scale+translate as withBounds; local bounds unchanged; BDD: boundary transforms, content sample size fixed |
| **IN-024** | implement | Surface `tgl.ink_scale_mode` on selected SG (srs-ui); default create remains fixedInk |
| **EP-008** | implement | Handle hit-test → resize ghost → `tool_intent { action: resize, bounds }` |
| **EP-009** | implement | Move ghost translates vector ink preview with dashed bounds (≥20 Hz dirty-rect) |

## Ask PM

Adopt CHL-0004 (severity normal) — restores written product rule, not a scope expansion.

## Ask Dev (once sliced)

Do **not** “fix” fixedInk by only moving content UV — boundary must transform.

## Review checklist (abbrev)

- ✅ Strength: sync + selection chrome landed; enclose local-space geometry
- ⚠️ Concern: surround-create geometry migration still pending; tablet resize was explicitly deferred and is now in scope for verify
- ❌ Risk: shipping fixedInk resize as bounds-only violates BR-09i / quality table “boundary always scales”
