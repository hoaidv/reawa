---
from: sm
to: architect
date: 2026-08-13
iter: iter-003
cc: [pm, designer]
---

# Hand-off: SM → Architect — confirm EP-012 device units

## Context

[STORY-EP-012](../stories/STORY-EP-012.md) / `[UI-EP-02]` proposes constants in **device units**.
They are flagged open in the Spec. EP-019 must not treat them as locked and must not fall back
to 8 CSS px or `TILE_LOD_SCALE = 0.35`.

| Proposal | Value | Source |
|---|---|---|
| Handle visual | **28 du** | Spec spike @ 226 dpi |
| Handle hit | **56 du** | Spec spike |
| LOD cutoff | min on-panel axis **< 96 du** | Spec spike; fixed panel, no tiles |

Package: `.plan/iter-003/design/device-selection-chrome/ui-spec.md`
SRS Open table: `.docs/modules/epaper/features/ink-box/srs-ui.md` `[SRS-EP-12]`

## Ask

Confirm or counter-propose **in device units**. Close the two Open rows in `srs-ui.md` (handle +
LOD only). Do **not** triage CHL-0010 (PM). Do not edit `epaper/` code or the design package
HTML unless a Spec note is required.

EP-019 stays `draft` until this lands. Not a W9 unblock.

## Parallel

`/pm` triages [CHL-0010](../challenges/CHL-0010-undo-vs-selection-create-chrome.md).
`/qa` still owns EP-013 device protocol.
