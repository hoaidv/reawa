---
id: CHL-0012
title: Ink-box sizing FREE_FORM / WRAP_CONTENT + align-content
author: pm
target: [REQ-05, REQ-06, SRS-EP-10, SRS-EP-11, ADR-0011]
severity: medium
status: resolved
resolution: adopted-future
resolved_by: pm
resolved: 2026-08-13
opened: 2026-08-13
iter: iter-003
expedite: false
interrupts_track: ""
raised_by: human
source: human product intent (sizing modes)
---

# CHL-0012 — Ink-box sizing modes + content align (future)

## Context

Human (2026-08-13) adopted three related future product features for ink-box layout. They are
**not** the current campaign’s `inkScaleMode` (`withBounds` | `fixedInk`) from
[ADR-0011](../../../.docs/adr/ADR-0011-smart-group.md) — that pair stays the **shipping**
resize-feel policy. These new axes describe **how bounds are owned and how content sits
inside them**.

Today: enclose sets `bounds` from the boundary stroke AABB; draw-into never expands bounds;
content placement is freehand / UV under `fixedInk` — **no** `align-content`, **no**
`WRAP_CONTENT` / `FREE_FORM` sizing field.

## Proposal — three features (Adopt → future)

### 1. Sizing = `FREE_FORM`

- Width/height of the ink-box **must be provided** (explicit geometric size).
- **First create:** w/h are detected from the **boundary-ink** (fitted AABB), then treated as
  the box’s free-form size.
- Content may be realigned independently of the boundary (see feature 3).

### 2. Sizing = `WRAP_CONTENT`

- Width/height of the ink-box = **detected from content-ink** (content AABB drives bounds).
- **No** `align-content` in this mode.
- Ink-box **may auto-expand** when new ink joins via draw-into membership (bounds grow to
  wrap content). That **contradicts** today’s “membership does not expand bounds” rule — only
  under `WRAP_CONTENT`, when scheduled.

### 3. `align-content` (only when sizing = `FREE_FORM`)

- Align **content-ink** (not boundary-ink) within the ink-box to **TOP | RIGHT | BOTTOM | LEFT**
  relative to the boundary-ink / bounds.
- Not available under `WRAP_CONTENT`.

## Resolution

**Adopted → future** — 2026-08-13 (PM). All three features accepted as product intent.

- **Not scheduled** on TRACK-003 W10–W12 (EP-017 membership stays **flat, bounds-not-expanded**).
- Shipping model unchanged: `inkScaleMode` `withBounds` | `fixedInk`; enclose bounds from
  boundary AABB; draw-into does **not** expand bounds; no `align-content`.
- Architect thickens when PM opens a later campaign: new field(s) vs extending `inkScaleMode`,
  schema/`create_smart_group` payload, Infini mirror parity, ADR-0011 amendment or successor.
- No expedite. No interrupt. Does not change EP-017 AC.

## Product doc updates

- [epaper/prd.md](../../../.docs/modules/epaper/prd.md) — Won’t-this-campaign / Future bullets
- [SRS-EP-10](../../../.docs/modules/epaper/features/ink-box/srs-logic.md) — current membership /
  bounds rule stays; pointer to this CHL
- [srs-product.md](../../../.docs/modules/epaper/features/ink-box/srs-product.md) — out-of-scope row
- [execution-board](../execution-board.md) backlog sink
- [MASTER](../../MASTER.md) challenge list

## Interrupt / expedite (when applicable)

n/a
