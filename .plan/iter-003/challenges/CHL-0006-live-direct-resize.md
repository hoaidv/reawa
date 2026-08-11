---
id: CHL-0006
title: Live direct resize on Epaper — drop resize ghost
status: resolved
resolution: adopted
severity: normal
raised_by: pm
resolved_by: pm
iter: iter-003
date: 2026-08-11
related: [SRS-EP-04, SRS-IN-13, REQ-03, REQ-04, CHL-0005]
---

# CHL-0006 — Live direct resize (drop ghost)

## Conflict

Human: resize ghost still scales content under fixedInk (CHL-0005 incomplete in practice). Preference: **drop the resize ghost**; mutate baked ink on device; **live-sync** to desktop. Slow e-ink refresh is acceptable.

## Prior design

SRS-EP-04 / ADR-0013 §4: advisory local ghost; one `tool_intent` on pen-up; `doc_snapshot` is truth.

## Resolution

**Adopted** 2026-08-11 by PM (human directive).

1. **Resize** (not move): no ink ghost overlay. Epaper mutates member path points in the local `doc_snapshot` cache with mode-correct fixedInk/withBounds mapping, updates pickable bounds, re-rasterizes (slow OK).
2. **Live sync:** emit `tool_intent` resize with `live: true` during drag (throttled); Infini applies without per-sample undo; desktop rebuilds + may push snapshot. Pen-up emits commit (`live` absent/false) with one undo entry from pre-gesture snapshot.
3. Mid-resize inbound `doc_snapshot` may be ignored until gesture ends (device owns live picture).
4. Move may keep ghost for this change set.
5. CHL-0005 ghost mode-correctness superseded for resize by this decision (members still used for local mutate).

Stories: EP-011, IN-026. Dev implements immediately.
