---
from: designer
to: sm
date: 2026-08-20
iter: iter-005
cc: [pm, qa]
---

# Hand-off: Designer → Scrum Master — UI-EP-06 field-test chrome delta

## Context

In-place revision of [UI-EP-06](../design/hand-touch/ui-spec.md) (Hand-touch — one-finger pick/move and two-finger pan/zoom) after human **2026-08-20 approved hand-touch**. Package slug unchanged: `.plan/iter-005/design/hand-touch/`.

[STORY-EP-054](../stories/STORY-EP-054.md) (Revise hand-touch: palm-rest vs empty local pan) stays **`done`**. This is not a new design story.

## Contract painted

1. Empty-canvas palm-rest **≤ 20 mm / 178 du**; local pan **> 20 mm**. Leftover 10 mm / 89 du removed from this package (finger-eligible floor stays 10 mm).
2. Closed control `btn.hand_touch` — 64×64 du 1-bit tile, label **HT**, trailing orientation-top **left of Debug**. Inverted when **on** (default); paper when **off**. Kill-switch, not a pan-mode / hand-tool tile. Follow stays in UI-EP-07.
3. Optional states `hand.toggle_off` and `hand.palm_contacts` documented in the Spec matrix. **No new scene files** — reuse palm empty-canvas + states showcase.
4. Debug log panel is **not** product inventory (placement mention only).

## Files changed

### Package (`.plan/iter-005/design/hand-touch/`)

- `ui-spec.md`
- `tokens.json` / `tokens.css`
- `common.css`
- `components.md`
- `components/hand-touch-toggle.html` **(new component HTML, not a scene)**
- `components/finger-contact.html` (threshold copy)
- Keep scenes (HT on + palm/pan copy): `hand-touch-finger-hit-box.html`, `hand-touch-finger-moving.html`, `hand-touch-finger-resizing.html`, `hand-touch-one-finger-empty-palm.html`, `hand-touch-one-finger-empty-pan.html`, `hand-touch-two-finger-pan.html`, `hand-touch-pinch.html`, `hand-touch-pan-vs-move.html`, `hand-touch-link-down-local-view.html`
- `hand-touch-states.html` (HT on/off/pressed + 20 mm copy)
- `index.html` (navigator hint + inlined copies)

### Index (allowed)

- `.docs/design/index.md` — UI-EP-06 blurb no longer says 10 mm

## New scene files

**None.** Optional SRS ids reuse existing palm / states HTML.

## Gate (light)

`ui-spec-gate` mentally **pass** for this delta: closed inventory includes `btn.hand_touch`; 9 Keep scenes still 1:1; optional states marked N/A + reuse; CSS allowlist `tokens.css` + `common.css`; inlined-scene `index.html` intact.

Pre-existing mechanical: `data-platform="epaper"` vs engine allowlist — [CHL-0002](../../iter-003/challenges/CHL-0002-epaper-platform-gate.md). Did **not** fail this slice on that.

## Asks

1. Do **not** reopen EP-054. Story AC still names 10 mm — PM/SM stitch when convenient; Designer did not edit the story.
2. `/qa` / `/dev` already shipped field chrome: this package now matches. No new implement story from Designer.

## Out of scope

- Application code, PRD, SRS, Follow-toggle (UI-EP-07), Debug log panel, Erase / Device Settings.
