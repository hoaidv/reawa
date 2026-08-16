---
id: STORY-EP-033
title: Reject origin/stale first sample on pen-down
kind: implement
parent_srs: [SRS-EP-01]
parent_req: [REQ-01]
status: done
priority: P0
iter: iter-004
estimate: 2
owner: dev
depends_on: []
acceptance_criteria:
  - "Given a TabletPress whose mapped sample is digitizer origin (raw 0,0 → panel bottom-left), When the next sample is the real contact point, Then no ink segment is painted from origin to the tip (SRS-EP-01)."
  - "Given pen-down on the ToolChip (including after a stale first packet), When the real sample lands on a tile, Then the chip arms and no diagonal is committed to the ink image."
  - "Given synthesized mouse events while the stylus is in contact, When both mouse and tablet fire, Then only one coordinate space is ingested (no second stroke from widget-local 0,0)."
  - "Given a normal page stroke with no zero packet, When the user draws, Then ink still starts under the tip with no extra latency gate beyond the existing ~8 ms flush (REQ-01)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-033 — Reject origin/stale first sample on pen-down

**Bug / hotfix.** Human 2026-08-15: random straight diagonal from panel **bottom-left** to the pen tip (often the Pen tile) on pen-down. Not a refresh ghost — a real `emitSegment` on the persistent ink `QImage`.

Parent: [SRS-EP-01](../../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md) · [REQ-01](../../../.docs/modules/epaper/prd.md#local-pen-ink).

No design story — ingest defect, no new UI.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Priority | **P0** — **done** (human verified 2026-08-16) |

## Diagnosis (device)

`mapInputToCanvas(0,0)` is panel `(0, height)` (bottom-left). Sequence:

1. `evdevtablet` sometimes emits `TabletPress` at raw `(0,0)` (`BTN_TOUCH` before a new `ABS_X`/`ABS_Y`).
2. `beginStroke` starts at origin; the next `TabletMove` at the real tip draws one long line (`appendPoint` → `emitSegment`).
3. ToolChip hit-test swallows **Press** only — a Move onto the Pen tile still inks.
4. App-wide filter also maps **mouse** `position()` (widget-local) through the landscape digitizer transform, so a synthesized mouse press at local `(0,0)` produces the same origin stroke.

Writes: `epaper/tabletappfilter.cpp`, `epaper/tabletcanvasitem.cpp` (`ingestPoint` / `beginStroke` / chip hit). **∥ [STORY-IN-030](./STORY-IN-030.md)** (`infini/` only). Do not parallel other `tabletcanvasitem.cpp` work.

## Fix intent (do not expand)

- Discard or replace a first contact sample at origin / implausible jump before painting.
- Chip (and other chrome) hit-test on the first **plausible** sample, including Move-after-stale-Press.
- Ignore synthesized mouse while a tablet contact is in flight.
- `@fix [STORY-EP-033]` at the guard (traceability: bug-fix line).

## Out of scope

Coordinate-map rewrite, Pen waveform, EP-030 recognition, EP-032 chrome SM.
