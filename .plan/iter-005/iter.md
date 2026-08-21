---
iter: iter-005
goal: "Hand-on-paper (REQ-10…18 except REQ-15) plus independent cameras and optional viewport follow"
committed_points: 96
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

W0 bind **done** 2026-08-19. W1 design EP-037 / IN-034 **done** (IN-034 Infini paint superseded). 2026-08-20: BRD-07 lifted; follow EP-053 / IN-036 **done**. W-pen-map EP-056 **done** ([UI-EP-08](./design/pen-button-map/ui-spec.md)). Device Settings persist on Epaper ([REQ-20](../../.docs/modules/epaper/prd.md#device-settings); [ADR-0031](../../.docs/adr/ADR-0031-device-settings-persist-on-epaper.md)). Follow toggles [STORY-EP-055](./stories/STORY-EP-055.md) and [STORY-IN-037](./stories/STORY-IN-037.md) **done**. [STORY-IN-033](./stories/STORY-IN-033.md) **done**. Hand-touch **human-approved** 2026-08-20 (20 mm / HT). 2026-08-21: [CHL-0026](./challenges/CHL-0026-inverse-op-undo.md) inverse-op undo — [ADR-0032](../../.docs/adr/ADR-0032-inverse-op-undo.md) **proposed**. WAIT Product Manager adopt. **No code.** W3 frozen. Follow field test still outstanding.

## Committed

### Design (W1+)

- [STORY-EP-037](./stories/STORY-EP-037.md) — design — 5 — hand-touch — **done** (W1-A) [UI-EP-06](./design/hand-touch/)
- [STORY-IN-034](./stories/STORY-IN-034.md) — design — 3 — pen-button map **∥ EP-037** — **done** (W1-B, Infini paint **superseded**) [UI-IN-03](./design/pen-button-map/)
- [STORY-EP-053](./stories/STORY-EP-053.md) — design — 3 — viewport-follow Epaper — **done** (W-follow) [UI-EP-07](./design/viewport-follow-epaper/)
- [STORY-IN-036](./stories/STORY-IN-036.md) — design — 3 — viewport-follow Infini — **done** (W-follow) [UI-IN-04](./design/viewport-follow-infini/)
- [STORY-EP-056](./stories/STORY-EP-056.md) — design — 5 — revise pen-button map as Epaper — **done** (W-pen-map) [UI-EP-08](./design/pen-button-map/)
- [STORY-EP-054](./stories/STORY-EP-054.md) — design — 3 — hand-touch empty pan delta — **done** (20 mm / HT package amend 2026-08-20)
- [STORY-EP-057](./stories/STORY-EP-057.md) — implement — 3 — persist Device Settings — **draft**
- [STORY-EP-058](./stories/STORY-EP-058.md) — implement — 5 — Settings page Pen buttons — **draft**
- [STORY-EP-040](./stories/STORY-EP-040.md) — design — 3 — erase
- [STORY-EP-043](./stories/STORY-EP-043.md) — design — 3 — clipboard
- [STORY-EP-045](./stories/STORY-EP-045.md) — design — 3 — connector ends
- [STORY-EP-048](./stories/STORY-EP-048.md) — design — 3 — attachments
- [STORY-EP-050](./stories/STORY-EP-050.md) — design — 5 — manual create

### Implement (gated by design + BDD)

- [STORY-EP-038](./stories/STORY-EP-038.md) — 5 — 1-finger · depends EP-037 + EP-054 — **done**
- [STORY-EP-039](./stories/STORY-EP-039.md) — 5 — 2-finger **local** pan — **done**
- [STORY-IN-033](./stories/STORY-IN-033.md) — 3 — Infini apply while following · depends EP-039 + IN-037 — **done**
- [STORY-EP-055](./stories/STORY-EP-055.md) — 5 — Epaper follow toggle · depends EP-053 — **done**
- [STORY-IN-037](./stories/STORY-IN-037.md) — 5 — Infini follow toggle · depends IN-036 — **done**
- [STORY-EP-041](./stories/STORY-EP-041.md) — 5 — eraser nib · depends EP-040
- [STORY-EP-042](./stories/STORY-EP-042.md) — 3 — selection-erase · depends EP-040
- [STORY-EP-044](./stories/STORY-EP-044.md) — 5 — clipboard ops · depends EP-043
- [STORY-EP-046](./stories/STORY-EP-046.md) — 5 — end styles · depends EP-045
- [STORY-EP-047](./stories/STORY-EP-047.md) — 5 — endpoint ink · depends EP-045
- [STORY-EP-049](./stories/STORY-EP-049.md) — 5 — attachments warp · depends EP-048
- [STORY-EP-051](./stories/STORY-EP-051.md) — 8 — manual insert · depends EP-050
- [STORY-EP-052](./stories/STORY-EP-052.md) — 5 — barrel dispatch · depends EP-056
- [STORY-IN-035](./stories/STORY-IN-035.md) — 3 — Infini persist — **cancelled**

## Carry-over (not NOW)

- [STORY-EP-035](../iter-004/stories/STORY-EP-035.md) — enclose A/L measure — parking lot

## Parked (not this lock)

- REQ-15 tables · REQ-08 · CHL-0011 · CHL-0012 · EP-032

## Risks

- CHL-0022 shipped “no pan” prose — Product Manager adopt
- EP-037 package still empty=no-op until EP-054
- PM `srs-product` BR-D08 still always-on viewport (architect flagged)
- [CHL-0026](./challenges/CHL-0026-inverse-op-undo.md) inverse-op undo — [ADR-0032](../../.docs/adr/ADR-0032-inverse-op-undo.md) proposed (READY-WITH-CONCERNS); **no application code** until adopt
- Vertical work-in-progress 2: Architect docs lane; W3 / Device Settings still frozen
- Agent host has no RM2 panel / no live TCP `:9877`; remaining follow field test is still outstanding

## Links

- [REQ-10](../../.docs/modules/epaper/prd.md#hand-touch) … [REQ-20](../../.docs/modules/epaper/prd.md#device-settings)
- infini [REQ-05](../../.docs/modules/infini/prd.md#pen-button-map) **retired**
- [BS-0002](../iter-004/brainstorms/BS-0002-iter-005-feature-wave.md)
