---
iter: iter-005
goal: "Hand-on-paper (REQ-10…18 except REQ-15): finger grammar, erase, clipboard, connector decorate, barrel buttons, manual create"
start: 2026-08-16
end: ""
capacity: 72
committed_points: 77
status: active
---

# Iter 005 — Hand-on-paper

Track: [TRACK-005](../tracks/TRACK-005-hand-on-paper.md) ·
Board: [execution-board](./execution-board.md)

Lock: **vertical · verified · wip 2**.

**Not in this iter:** [REQ-15](../../.docs/modules/epaper/prd.md#table-recognition) tables. [REQ-16](../../.docs/modules/epaper/prd.md#device-pan-zoom) retired into REQ-10.

Stories are **draft** until `/architect` binds dedicated SRS (parents today are nearest existing sections).

## Committed

### Design (W1+)

- [STORY-EP-037](./stories/STORY-EP-037.md) — design — 5 — hand-touch
- [STORY-IN-034](./stories/STORY-IN-034.md) — design — 3 — pen-button map **∥ EP-037**
- [STORY-EP-040](./stories/STORY-EP-040.md) — design — 3 — erase
- [STORY-EP-043](./stories/STORY-EP-043.md) — design — 3 — clipboard
- [STORY-EP-045](./stories/STORY-EP-045.md) — design — 3 — connector ends
- [STORY-EP-048](./stories/STORY-EP-048.md) — design — 3 — attachments
- [STORY-EP-050](./stories/STORY-EP-050.md) — design — 5 — manual create

### Implement (gated by design + BDD)

- [STORY-EP-038](./stories/STORY-EP-038.md) — 5 — 1-finger · depends EP-037
- [STORY-EP-039](./stories/STORY-EP-039.md) — 5 — 2-finger pan · depends EP-037
- [STORY-IN-033](./stories/STORY-IN-033.md) — 3 — Infini viewport · depends EP-039
- [STORY-EP-041](./stories/STORY-EP-041.md) — 5 — eraser nib · depends EP-040
- [STORY-EP-042](./stories/STORY-EP-042.md) — 3 — selection-erase · depends EP-040
- [STORY-EP-044](./stories/STORY-EP-044.md) — 5 — clipboard ops · depends EP-043
- [STORY-EP-046](./stories/STORY-EP-046.md) — 5 — end styles · depends EP-045
- [STORY-EP-047](./stories/STORY-EP-047.md) — 5 — endpoint ink · depends EP-045
- [STORY-EP-049](./stories/STORY-EP-049.md) — 5 — attachments warp · depends EP-048
- [STORY-EP-051](./stories/STORY-EP-051.md) — 8 — manual insert · depends EP-050
- [STORY-EP-052](./stories/STORY-EP-052.md) — 5 — barrel dispatch · depends IN-034
- [STORY-IN-035](./stories/STORY-IN-035.md) — 3 — publish map · depends IN-034

## Carry-over (not NOW)

- [STORY-EP-035](../iter-004/stories/STORY-EP-035.md) — enclose A/L measure — parking lot

## Parked (not this lock)

- REQ-15 tables · REQ-08 · CHL-0011 · CHL-0012 · EP-032

## Risks

- SRS not yet decomposed — architect is W0
- REQ-10 two-finger pan still needs BRD-07 amendment
- Vertical wip 2 vs many features — waves are serial after W1 pair

## Links

- [REQ-10](../../.docs/modules/epaper/prd.md#hand-touch) … [REQ-18](../../.docs/modules/epaper/prd.md#pen-buttons)
- infini [REQ-05](../../.docs/modules/infini/prd.md#pen-button-map)
- [BS-0002](../iter-004/brainstorms/BS-0002-iter-005-feature-wave.md)
