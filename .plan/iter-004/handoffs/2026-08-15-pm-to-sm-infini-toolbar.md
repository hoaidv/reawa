---
from: pm
to: sm
date: 2026-08-15
iter: iter-004
---

# Hand-off: PM → SM — remove Infini editing toolbar this track

## Context

Human: **do not show Infini desktop editing tools**; **remove the leftover toolbar in TRACK-004**.

Product already said this ([infini REQ-04](../../../.docs/modules/infini/prd.md#smart-group) deprecated; [SRS-IN-14](../../../.docs/modules/infini/features/vector-document/srs-ui.md) deprecated). Code still mounts `infini/src/canvas/ToolStrip.tsx` from `CanvasStage.tsx`. That is now **in-campaign Must**, not backlog.

Lock stays **vertical · verified**. Feature is already in scope: `infini/vector-document`. No new module. No `/architect` (no new SRS). No `/designer` (removing deprecated chrome; 0 new states).

Also **adopted [CHL-0019](../challenges/CHL-0019-toolchip-tile-size.md)** — SRS-EP-05 is 64×64. EP-028 BDD already asserts 64.

## Asks

1. Slice one **implement** story (suggest STORY-IN-031): remove ToolStrip + SelectionOverlay + transform handles from Infini. Parent `[REQ-04]` / `[SRS-IN-14]` (deprecated — story is *remove leftover*, not implement SRS-IN-14). AC from REQ-04: 0 authoring affordances on desktop. Keep pan/zoom + open/save.
2. Commit it to iter-004 / TRACK-004. May run **∥ EP-028** (different tree: `infini/` vs `epaper/`).
3. `/qa` one BDD scenario on Infini (REQ-04 AC) then `/dev`.
4. Refresh execution board (CHORE or F-row). Do not start REQ-08.

## Constraints

- Do not revive STORY-IN-013 chrome.
- Do not add a desktop Pen.
- Do not widen lock to other Infini chrome (DocChrome IN-006 stays cancelled).

## Out of scope

Desktop connector authoring; multi-directional sync (when REQ-04 *returns*, new REQ id).

## Next after you

`/qa` (tiny Infini hide-toolbar scenario) then `/dev` on the new story **and/or** EP-028.
