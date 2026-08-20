---
title: Execution board — hand-on-paper
iter: iter-005
track: TRACK-005
owner: sm
date: 2026-08-20
lock: vertical · verified · wip 2
verdict: "Pen-button map interrupt done (EP-056 / UI-EP-08). NOW EP-054 hand-touch empty-pan after human glance at pen-button-map. Barrel implement waits on BDD."
wave: W-empty
---

# Execution board — hand-on-paper

**Canonical board** for [TRACK-005](../tracks/TRACK-005-hand-on-paper.md).

---

## Summary (as of 2026-08-20)

| Band | Count | Meaning |
|---|---|---|
| Design **done** | 4 current | [UI-EP-06](../../.docs/design/index.md) hand-touch · [UI-EP-07](../../.docs/design/index.md) follow Epaper · [UI-IN-04](../../.docs/design/index.md) follow Infini · [UI-EP-08](../../.docs/design/index.md) pen-button map (on-device) |
| Wave **NOW** | W-empty | Product Designer [STORY-EP-054](./stories/STORY-EP-054.md) after human glance at [UI-EP-08](./design/pen-button-map/) |
| Queued | — | Quality Assurance Engineer EP-038 after EP-054; barrel BDD after human glance at UI-EP-08 |

Cameras independent by default. [ADR-0029](../../.docs/adr/ADR-0029-independent-cameras-viewport-follow.md) accepted. [ADR-0023](../../.docs/adr/ADR-0023-viewport-last-writer.md) superseded.

Pen-button map: [REQ-18](../../.docs/modules/epaper/prd.md#pen-buttons) on-device editor. Infini [REQ-05](../../.docs/modules/infini/prd.md#pen-button-map) persist/restore only. [ADR-0030](../../.docs/adr/ADR-0030-tablet-authors-pen-button-map.md) accepted.

---

## Lock (copy into every sub-agent brief)

```
direction: vertical
stop_line: verified
autonomy: bounded
wip: 2
out_of_scope: backlog
modules: epaper, infini
features: epaper/ink-box; epaper/tool-modes; epaper/connector-ink; epaper/region-sync; epaper/local-pen-ink; epaper/device-document; infini/infinity-canvas; infini/tablet-sync; infini/vector-document
personas: /designer EP-054 after human glance
forbidden: REQ-15 tables; REQ-08; CHL-0011; CHL-0012; EP-032; AI; last-writer ADR-0023; do not /dev until design done + BDD
NOW: W-empty designer STORY-EP-054
cursor: /designer STORY-EP-054 after human glance at UI-EP-08
```

---

## Execution map (canonical)

### Wave legend

| Wave | Status | Parallel? | What |
|---|---|---|---|
| **W0** | **done** | serial | Architect SRS bind (hand-on-paper) |
| **W1** | **done** | **∥** | Designer EP-037 ∥ IN-034 (IN-034 Infini paint **superseded** by EP-056) |
| **W-follow** | **done** 2026-08-20 | **∥** | Designer EP-053 ∥ IN-036 (viewport-follow toggles) |
| **W-pen-map** | **done** 2026-08-20 | serial | Architect rebind; Designer EP-056 revise `pen-button-map/` as epaper-device |
| **W-empty** | **NOW** | serial | Designer EP-054 (`hand-touch/` delta — palm vs empty pan) after human glance |
| **W2** | queued | serial | Quality Assurance Engineer then Developer EP-038 (after EP-054) |
| **W3** | queued | | erase / clipboard design |
| **W4** | queued | | connector ends / attachments |
| **W5** | queued | | barrel BDD then EP-052 / IN-035; then EP-039 local two-finger; IN-033 after IN-037 |
| **W6** | queued | | manual create |
| **Frozen** | — | | REQ-15 · REQ-08 · EP-035 parking |

### Parallelism rules (current wave)

| Lane | Story | Writes | Conflicts |
|---|---|---|---|
| **A** | [STORY-EP-054](./stories/STORY-EP-054.md) | `.plan/iter-005/design/hand-touch/` | do not edit `pen-button-map/` or follow packages; do not edit `.docs/design/index.md` (parent stitches) |

Work-in-progress 2 = this lane only until join. Do not parallel-edit `hand-touch/` with another designer.

### Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group | Progress Detail |
|---|---|---|---|---|---|---|---|---|---|
| F-10 | REQ-10 hand-touch | Must | SRS rebound | EP-037 done; [STORY-EP-054](./stories/STORY-EP-054.md) **ready** | NOW | W-empty | designer after human glance | — | Empty pan >10 mm; palm ≤10 mm. EP-038 AC replanned. |
| F-19 | REQ-19 viewport-follow Infini | Must | SRS bound | [STORY-EP-053](./stories/STORY-EP-053.md) **done** | done | W-follow | Quality Assurance Engineer then Developer EP-055 | — | Icon toggle; not ToolChip. |
| F-IN-06 | Infini REQ-06 viewport-follow Epaper | Must | SRS bound | [STORY-IN-036](./stories/STORY-IN-036.md) **done** | done | W-follow | Quality Assurance Engineer then Developer IN-037 | — | Desktop toggle. |
| F-18 | REQ-18 barrel accelerators | Must | PRD 0.10.0-draft; SRS-EP-52/53 bound | IN-034 historical; [STORY-EP-056](./stories/STORY-EP-056.md) **done** [UI-EP-08](./design/pen-button-map/) | design done | W-pen-map | human glance; then Quality Assurance Engineer; GAP-01 PM adopt entry tile | — | On-device editor; Click 3; Hold-move temp eraser / drag / off. No temp freeform. |
| F-IN-05 | Infini REQ-05 persist map | Must | PRD 0.7.0-draft; UI outcome retired | no Infini design | queued | W5 | Developer IN-035 after BDD | — | Persist/restore only. No desktop editor. ADR-0030. |
| F-11 | REQ-11 erase | Must | — | EP-040 queued | queued | W3 | designer | — | — |
| F-12 | REQ-12 clipboard | Must | — | EP-043 queued | queued | W3 | designer | — | — |
| F-13 | REQ-13 endpoint styles | Should | — | EP-045 queued | queued | W4 | designer | — | — |
| F-14 | REQ-14 attachments | Should | — | EP-048 queued | queued | W4 | designer | — | — |
| F-17 | REQ-17 manual create | Should | — | EP-050 queued | queued | W6 | designer | — | — |
| CHORE-1 | BRD-07 pan/zoom amend | — | **done** 2026-08-20 | — | done | — | — | — | Independent cameras + follow. |
| CHORE-2 | PM srs-product BR-D08 always-on viewport | — | — | — | open | after bind | product-manager | — | Architect flagged; not a design blocker. |
| CHORE-3 | PM adopt GAP-01 pen-map entry tile | — | — | UI-EP-08 | open | after glance | product-manager | — | Leading 10 mm stylus-with-barrels tile; sibling of ToolChip. |
| PL-035 | EP-035 enclose A/L | P1 | — | — | ready | parking | Developer later | — | Not this wave. |

### Current-wave sub-agent roster

| Lane | Agent role | Story | Writes | Done when |
|---|---|---|---|---|
| A | designer | STORY-EP-054 Revise hand-touch: palm-rest vs empty local pan | `design/hand-touch/` | ui-spec-gate; palm ≤10 mm vs empty pan >10 mm scenes; story `done` |

Wait: human glance at [UI-EP-08](./design/pen-button-map/) before spawning EP-054 (capacity) unless the human says go.

### Backlog sink

| Item | Why |
|---|---|
| REQ-15 tables | human excluded |
| REQ-08 / CHL-0011 / CHL-0012 | parked |
| EP-032 chrome SM | parked iter-004 |
| Infini→Infini follow | forward, not Must |
| Last-writer ADR-0023 | **superseded** — do not implement |
| Infini desktop map editor | **retired in place** (Infini REQ-05 UI outcome; UI-IN-03 superseded) |

---

## Verdict

**NOW** W-empty: [STORY-EP-054](./stories/STORY-EP-054.md) after a human glance at the revised pen-button map. Work-in-progress 2 holds one designer lane. Do not Developer-implement barrel until behavior-driven scenarios exist.
