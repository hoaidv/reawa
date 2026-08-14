---
from: pm
to: architect
date: 2026-08-14
iter: iter-003
source: CHL-0018
verdict: READY-WITH-CONCERNS
---

# Hand-off: PM → Architect — CHL-0018 live node on ToolCanvasLayer

## Verdict

**READY-WITH-CONCERNS** — product lock is adopted; ADR-0019 needs a one-paragraph amend (you own ADRs).

## Decision

Human + PM **adopt option 1** for SmartGroup move/resize paint:

- Document still transforms live (BR-B10 unchanged).
- **Pixels in flight** live on ToolCanvasLayer; CanvasLayer hides the origin box only.
- **Pen-up** rasterizes one committed node onto CanvasLayer.
- Mid-gesture e-ink ghosting / dirty traces are a **refresh allowance**.
- **Option 2** (live node on CanvasLayer) is **deferred**, not rejected — reopen in a later
  rendering phase if overlay composition fights Pen ink.

## Product docs (done)

REQ-06, BR-B19, SRS-EP-11/12/14, architecture refresh note. Challenge:
[CHL-0018](../challenges/CHL-0018-live-node-tool-canvas.md).

## Ask

Amend [ADR-0019](../../../.docs/adr/ADR-0019-selection-chrome-layers.md): ToolCanvasLayer also
paints the live SmartGroup during move/resize; option 2 in Alternatives as deferred. No new ADR
unless you disagree with folding it into 0019.

## Next

`/architect` (ADR amend) then `/sm` if EP-025 AC needs a re-slice. No option-2 story this track.
