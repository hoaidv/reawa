---
from: sm
to: dev
date: 2026-08-16
iter: iter-004
---

# Handoff — SM → Dev — IN-030 ∥ EP-033

## Context

Human 2026-08-16: **run both in parallel.** WIP 2 is filled. EP-031 is done.

| Lane | Story | Writes | Do not touch |
|---|---|---|---|
| **D** | [STORY-IN-030](../stories/STORY-IN-030.md) | `infini/` envelope + derived warp | `epaper/tabletcanvasitem.cpp`, recognizers |
| **H** | [STORY-EP-033](../stories/STORY-EP-033.md) | `epaper/tabletappfilter.cpp` + ingest in `tabletcanvasitem.cpp` | `infini/`, recognizer thresholds |

Open **two** `/dev` sessions (one story each). Do not merge the write sets.

## Lane D — IN-030

BDD: `.docs/modules/infini/features/vector-document/bdd/create-connector.feature` (`@SRS-IN-09`).

1. Mirror `create_connector` (`from`/`to`/`warpStyle`/body/restShape) with 0 loss.
2. Same rest + ends + style → byte-comparable samples vs device (0 divergent nodes).
3. `set_smart_transform` on a bound box → 0 connector ops; re-derive.
4. Missing bind → last-live pose, not invalid (D39).

Infini is viewer + persistence. Do not author connectors on the desktop. Do not copy EXP-0002.

## Lane H — EP-033

No new UI. Host analog + device: no diagonal from panel origin on pen-down.

1. Discard/replace first contact at origin / implausible jump before paint.
2. ToolChip hit-test on the first **plausible** sample (Move-after-stale-Press).
3. Ignore synthesized mouse while tablet contact is in flight.
4. Normal strokes: no extra latency gate beyond ~8 ms flush.
5. `@fix [STORY-EP-033]` at the guard.

## Constraints

Lock: vertical · verified. Forbidden: REQ-08, CHL-0011, CHL-0012. EP-034 stays queued. Do not start a third `tabletcanvasitem.cpp` story.

## Next after each lane

**`/qa`** on that story (`in-review`). Do not wait for the other lane.
