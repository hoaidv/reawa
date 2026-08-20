---
id: CHL-0023
author: designer
target: [SRS-EP-05, SRS-EP-12]
severity: medium
status: open
opened: 2026-08-20
iter: iter-005
expedite: false
interrupts_track: ""
---

# CHL-0023 — Epaper physical scale: 246×187 mm, 1 cm tiles, dotted selection

## Context

Human visual review of [UI-EP-06](../design/hand-touch/) (and every other epaper package) on 2026-08-20:

1. **Wrong scale.** Device is **187 × 246 mm** at **1872 × 1404**. Primary buttons must be **1 × 1 cm**.
2. **Toolbar position wrong** — caused by DeviceScreen `width/height: 100%` filling a portrait IDE preview, clipping the centered chip so only the right clusters show at top-left.
3. **Helper text** (scene captions, `orient:` tags, StatusLine debug strings) is not product chrome.
4. **Selection** is a thin-thick double-rail. Needed: **dotted** outline.
5. **Eight filled handles** are wrong. Needed: a **simple empty square with solid border** (same language as UI-EP-03 anchors).
6. **Preview** must keep real size **187 × 246 mm**, not scale to the parent pane.
7. **Mode toggle** is icon-only; size = primary ToolChip tile (**10 mm**). No hatch pill, no “Scale ink” label.
8. **Primary toolbar** (sel_rect / sel_freeform / Pen cluster) sits on the **horizontal center** of the panel. Recognizers left, Undo/Redo right.
9. **Finger marks** are design-preview only: a **filled color circle, no border**, not 1-bit hatch chrome.
10. **Selection context controls** (mode toggle, `cta.enclose`) sit **8 mm below** the selection box — not flush with the south edge.

This contradicts closed [CHL-0019](../../iter-004/challenges/CHL-0019-toolchip-tile-size.md) (64 px tiles), [SRS-EP-05](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md) chip **64×64 px**, [SRS-EP-12](../../../.docs/modules/epaper/features/ink-box/srs-ui.md) SmartGroup double-rail + 8 resize knobs, and the designer navigator 80% mobile scale.

Human override is painted in the iter packages now. Product SRS still says 64 px / 8-knob overlay until PM adopts.

## Proposal

| Topic | Adopt |
|---|---|
| Panel | Landscape preview **246 mm × 187 mm** = 1872 × 1404 (long side 246 mm ↔ 1872 px). Portrait native 187 mm × 246 mm. |
| Primary tile | **10 mm × 10 mm (1 cm)**. Finger-eligible floor = that tile. Update CHL-0019 / SRS-EP-05. |
| Selection overlay | Single **dotted** AABB; **no** double-rail. Resize/select anchors = **hollow squares, solid 1–2 px border** (paper fill). Drop the extra E/W knobs (8 → 6, matching UI-EP-03). |
| Preview | Navigator frame = **246 mm × 187 mm**, `transform: none`. Scroll the parent; do not `min(100%, …)` scale. |
| Helper copy | Scene captions, orient tags, StatusLine debug strings are design-preview only — hide in hi-fi scenes. |
| Mode toggle | Icon-only **10 mm** square (same as primary tile). Label visually hidden. Contradicts SRS-EP-12 36 du pen-only hatch pill. |
| ToolChip | Primary cluster (3 exclusive tools) **centered** on the panel; side clusters flank it. |
| Finger annotate | Filled color circle, no border — not product 1-bit chrome. |
| Selection context chrome | Mode toggle / Enclose **8 mm** below the AABB (clear of south handle). |

## Resolution

<!-- PM fills after triage: adopted | deferred | rejected -->

## Product doc updates

<!-- List docs updated if adopted -->
