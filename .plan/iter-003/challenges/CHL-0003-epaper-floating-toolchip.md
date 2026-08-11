---
id: CHL-0003
title: Epaper tool chrome — floating 32px orientation-top chip
status: open
iter: iter-003
raised_by: designer
date: 2026-08-11
source: human
---

# CHL-0003 — Floating narrow tool chip (human override)

## Conflict

[SRS-EP-05](../../.docs/modules/epaper/features/tool-modes/srs-ui.md) currently requires:

- Fixed **full-band** edge strip; InkSurface **shrinks**
- Strip does **not** float over ink
- Finger targets ≥120 px

Human (2026-08-11) directed:

1. **Narrow-small** — height **32px** (refined from ≤1 cm)
2. **On top of the activated current orientation** — sit on the top edge of the oriented frame (moves with gut orientation)
3. **Floating-like** — compact chip, **not** a full-width band; InkSurface stays full-bleed
4. Preview as **tablet landscape** (RM2 1872×1404), not mobile phone chrome

Designer revised `[UI-EP-01]` package under human override. Product SRS still says the opposite until PM adopts.

## Proposed SRS adopt (draft for PM)

**Containment (replace):** `ToolChip` (aka ToolStrip) is a **floating** compact control cluster (height **32px**, width hug content, icon-only) anchored to the **top edge of the current gut orientation**. `InkSurface` remains **full-bleed**. Pen/touch hits on the chip are excluded from ink (hit-test), but chrome does not reserve a full band.

**Physical:** height 32px; prefer compact targets (pen-on-strip remains the touch-unavailable fallback). Relax ≥120 px for this chip when PM accepts.

**Anti-pattern to retire:** “does not float over ink” / “full edge strip”.

## Ask

`/pm` adopt into SRS-EP-05 + logic exclusion rect = chip bounds (not full band). Then `/dev` EP-005 follows Spec.
