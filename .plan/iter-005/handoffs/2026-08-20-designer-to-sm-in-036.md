---
from: designer
to: sm
date: 2026-08-20
iter: iter-005
---

# Hand-off: Designer → SM — IN-036 done

## Context

Lane B painted Infini **viewport-follow Epaper toggle** as `[UI-IN-04]`. Package: [`.plan/iter-005/design/viewport-follow-infini/`](../design/viewport-follow-infini/). Story [STORY-IN-036](../stories/STORY-IN-036.md) is **done**. Primary scene: `viewport-follow-infini-off.html`. Open `index.html` (iframe navigator, desktop @ 100%).

Copied `design_package` / `ui_spec` / `scenes` / `hifi` onto [STORY-IN-037](../stories/STORY-IN-037.md) only. Did **not** edit `.docs/DESIGN.md`, `.docs/design/index.md`, `design/hand-touch/`, or `design/viewport-follow-epaper/`.

Icon toggle on WindowFrame trailing chrome (leading StatusZoom). Not document chrome (SRS-IN-05), not pen-button map (IN-034), not a ToolChip. Peer-following-you keeps this toggle **off** (exactly one direction) and tappable. Connection lost disables the control; reconnect stays off until opt-in. Local pan while following turns follow off.

## Asks

1. `/sm` stitch `.docs/design/index.md` row for `[UI-IN-04]` after Lane A joins (lock: Designer did not edit the index).
2. `/pm` optional thicken: `srs-experience` for tablet-sync; copy table (drafted in Spec).
3. `/qa` BDD then `/dev` on IN-037 — **after** design done + BDD. Do not `/dev` yet.

## Concerns (experience not thickened — campaign override)

- No `srs-experience.md` / `srs-ui-multi-scene.md` for tablet-sync. Scene inventory = SRS-IN-27 state ids only. Did not invent Infini→Infini follow or last-writer chrome.
- SRS-IN-27 has no copy table. Copy was drafted **in the UI Spec** for PM adopt — product docs were not silently edited.
- Optional drawing-region marker **omitted** (would imply always-on match). Camera crop + zoom % show apply vs local.
- **AC vs SRS (PM):** [STORY-IN-037](../stories/STORY-IN-037.md) AC says if Epaper follow is on, enabling Infini follow **stays off**. [REQ-06](../../../.docs/modules/infini/prd.md#viewport-follow) and [SRS-IN-27](../../../.docs/modules/infini/features/tablet-sync/srs-ui.md#srs-in-27-follow-toggle) say click **turns Epaper off and Infini begins following**. This package follows SRS-UI: peer scene is visually off and tappable → `follow.following_epaper`. `/pm` should align IN-037 AC before `/dev`.

## Constraints

- HTML is a visual reference, not production Electron/React.
- Infini slate/ink; hover required; `data-platform: desktop`.
- Unique system files only: `icon-viewport-follow-infini*.svg`, `viewport-follow-toggle-infini.html`.

## Out of scope

REQ-15 · REQ-08 · last-writer chrome · IN-034 pen-button map · SRS-IN-05 open/save · `design/hand-touch/` · `design/viewport-follow-epaper/` · `.docs/design/index.md`.
