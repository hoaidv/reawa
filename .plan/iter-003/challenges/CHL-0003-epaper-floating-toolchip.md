---
id: CHL-0003
title: Epaper tool chrome — floating 32px orientation-top chip
status: resolved
resolution: adopted
iter: iter-003
raised_by: designer
date: 2026-08-11
source: human
resolved_by: pm
resolved: 2026-08-11
---

# CHL-0003 — Floating narrow tool chip (human override)

## Conflict

[SRS-EP-05](../../.docs/modules/epaper/features/tool-modes/srs-ui.md) previously required:

- Fixed **full-band** edge strip; InkSurface **shrinks**
- Strip does **not** float over ink
- Finger targets ≥120 px

Human (2026-08-11) directed:

1. **Narrow-small** — height **32px**
2. **On top of the activated current orientation** — sit on the top edge of the oriented frame
3. **Floating-like** — compact chip, **not** a full-width band; InkSurface stays full-bleed
4. Preview as **tablet landscape** (RM2 1872×1404), not mobile phone chrome

Designer revised `[UI-EP-01]` package under human override. Product SRS still said the opposite until PM adopts.

## Proposed SRS adopt (draft for PM)

**Containment (replace):** `ToolChip` (aka ToolStrip) is a **floating** compact control cluster (height **32px**, width hug content, icon-only) anchored to the **top edge of the current gut orientation**. `InkSurface` remains **full-bleed**. Pen/touch hits on the chip are excluded from ink (hit-test), but chrome does not reserve a full band.

**Physical:** height 32px; prefer compact targets (pen-on-chip remains the touch-unavailable fallback). Relax ≥120 px for this chip when PM accepts.

**Anti-pattern to retire:** “does not float over ink” / “full edge strip”.

## Ask

`/pm` adopt into SRS-EP-05 + logic exclusion rect = chip bounds (not full band). Then `/dev` EP-005 follows Spec.

## Resolution

**Adopted** — 2026-08-11 (PM). Human override is product truth.

### Product doc updates

| File | Change |
|---|---|
| `epaper/.../srs-ui.md` SRS-EP-05 | Floating ToolChip 32px, orientation-top, full-bleed InkSurface; retire full-band / no-float |
| `epaper/.../srs-logic.md` SRS-EP-04 | Exclusion rect = chip bounds; pen-on-chip; orientation follow |
| `epaper/.../srs-quality.md` SRS-EP-06 | Chrome exclusion + a11y 32×32; dual-ask `orient.gutOnTop` |
| `epaper/prd.md` REQ-03 AC | Chip wording (exclusion + partial refresh) |

No ID retire — same `[SRS-EP-05]` / `[SRS-EP-04]` / `[SRS-EP-06]` thickened in place.
`[UI-EP-01]` Spec becomes aligned with product SoT (no longer “pending PM adopt”).

### Notify

- `/sm` — replan/confirm EP-005 AC against chip bounds (not full-band shrink); EP-003 design already matches.
- `/architect` — no new ADR required (device chrome placement, not system boundary); ADR-0013 still holds on thin-client intent.
- `/qa` — refresh BDD / AC for exclusion rect + 32px chip if already drafted against full-band.
- `/dev` — EP-005 implements chip + hit-test exclusion, not InkSurface shrink.
