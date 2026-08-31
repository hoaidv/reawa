---
id: CHL-0029
author: dev
target: [SRS-EP-03, SRS-EP-01]
severity: high
status: adopted
opened: 2026-08-31
iter: iter-005
expedite: true
interrupts_track: TRACK-005
---

# CHL-0029 — Settle is not FullClear on ordinary ink

## Context

Dense-page field: after erase phase, new strokes hitch ~100 ms then run smooth. Probe on device:
`reason=queued behind=rasterizeVectors` (680–946 ms FullClear of every on-camera node). The hitch
is GUI-thread document paint **between** strokes, not `InkStrokeOperation`.

[SRS-EP-03](../../../.docs/modules/epaper/features/region-sync/srs-quality.md) says settle /
committed local op → “Immediate; AA on”. Read as FullClear of the panel, that row **steals**
[SRS-EP-01](../../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md) p95 ≤30 ms
(architecture quality goal 1). Live stamps of ordinary `append_ink` already match the new free-ink
node (same samples, `world × s_panel`). A FullClear does not add content; it queues the next
pen-down.

## Proposal

Interpret settle as **pixels matching the local document**, not “white-fill the panel”:

- Ordinary `RecogOutcome::Ink`: skip document rasterize. Live stamps **are** the settle.
- Structural ops (enclose, connector create, erase, live-manip punch, undo when AABB unknown):
  sharp **InPlaceDirty** of the changed AABB, or FullClear when the dirty rect is missing / huge /
  the whole map (`doc_load`, camera, resize).
- Recog blink / membership bold: ToolCanvas `NodeEmphasis` (CHL-0030), not a second Tablet FullClear.

## Resolution

**Adopted** 2026-08-31 (interrupt TRACK-005 / SRS-EP-01). Architecture goal 1 outranks a
full-panel reading of SRS-EP-03 settle. Sharp InPlaceDirty counts as settle paint. Ordinary Ink skip
counts as settle because pixels already match. **Field-verified 2026-08-31** on a dense page
(recog off): ordinary ink no longer hitch-then-smooth.

## Product doc updates

- [SRS-EP-03](../../../.docs/modules/epaper/features/region-sync/srs-quality.md) / [SRS-EP-02 coalesce](../../../.docs/modules/epaper/features/region-sync/srs-logic.md): ordinary `append_ink` is live stamps, not FullClear; camera 250 ms must not run on that pen-up.
- [SRS-EP-01](../../../.docs/modules/epaper/features/local-pen-ink/srs-logic.md) item 10: GUI-thread steal rules.
- [`.docs/memory/ink-path-density-hitch.md`](../../../.docs/memory/ink-path-density-hitch.md)

## Interrupt / expedite

TRACK-005 ink-latency hitch. EP-069 is done; this does not mix clipboard.
