---
id: CHL-0005
title: Tablet resize ghost scales content under fixedInk
status: resolved
resolution: adopted
severity: normal
raised_by: architect
resolved_by: pm
iter: iter-003
date: 2026-08-11
related: [SRS-EP-04, SRS-IN-11, SRS-IN-13, REQ-04, CHL-0004]
---

# CHL-0005 — Tablet resize ghost ignores fixedInk

## Conflict

Human verify after CHL-0004:

| Surface | During resize | Final (pen-up / snapshot) |
|---|---|---|
| Desktop | OK — content size fixed; box ink scales | OK |
| Tablet | **BUG** — stub/ghost scales content + box together | OK — content fixed; box scales |

Final authority matches SRS-IN-11 / BR-09i. The advisory **resize ghost** on Epaper contradicts the mode the user will see after commit → confusion.

## Prior design

- SRS-IN-11: under `fixedInk`, **content** sample size stays fixed (UV track); **boundary** always × scale.
- SRS-EP-04 (post CHL-0004): resize ghost = “bounds + ink preview scale/translate” — **underspecified**; did not require mode-correct member treatment.
- Implementation: `paintInkGhost` uniformly scales every world path intersecting the pickable AABB. Snapshot `nodes` are flattened world paths with **no** `role` / `inkScaleMode`; `pickables` only carry `{id,kind,bounds}`.

## Root cause

Ghost cannot distinguish boundary vs content and assumes `withBounds` for all ink.

## Proposed resolution (Architect)

1. **Adopt** — amend SRS-EP-04 + SRS-IN-13 (done with this challenge): resize ghost must match Infini draw semantics for the pickable’s `inkScaleMode`.
2. Enrich `pickables[]` with `inkScaleMode` + `members: [{ id, role }]` (ids = flattened path node ids). Additive; older devices ignore.
3. Epaper resize ghost:
   - `withBounds`: scale+translate all members (current behavior).
   - `fixedInk`: **boundary** members scale+translate with the gesture AABB; **content** members **translate only** so each path’s centroid tracks UV in the live world AABB (`u,v` from centroid vs origin AABB — no protocol UV required for pilot).
4. Move ghost unchanged (pure translate of all members).
5. Do **not** change commit / Infini `smartLocalToWorld` — already correct.

## Out of scope

- Device `inkScaleMode` toggle (desktop-only pilot).
- Replacing advisory ghost with live Infini round-trip during drag.

## Resolution

**Adopted** 2026-08-11 by PM — restores BR-09i consistency during advisory ghost; Architect SRS amendments accepted. Stories IN-025 + EP-010; Dev implements immediately (human: adopt then fix). No interrupt-track (severity normal; TRACK-003).
