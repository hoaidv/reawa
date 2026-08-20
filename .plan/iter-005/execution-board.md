---
title: Execution board — hand-on-paper
iter: iter-005
track: TRACK-005
owner: sm
date: 2026-08-20
lock: vertical · verified · wip 2
verdict: "Device Settings persist on Epaper (REQ-20). NOW: designer EP-054 ∥ architect Device Settings rebind. Dev EP-038/039/IN-033 blocked: design+BDD + /init."
wave: W-empty
---

# Execution board — hand-on-paper

**Canonical board** for [TRACK-005](../tracks/TRACK-005-hand-on-paper.md).

---

## Summary (as of 2026-08-20)

| Band | Count | Meaning |
|---|---|---|
| Design **done** | 4 current | [UI-EP-06](../../.docs/design/index.md) hand-touch · [UI-EP-07](../../.docs/design/index.md) follow Epaper · [UI-IN-04](../../.docs/design/index.md) follow Infini · [UI-EP-08](../../.docs/design/index.md) pen-button map (on-device Settings) |
| Wave **NOW** | W-empty | Product Designer [STORY-EP-054](./stories/STORY-EP-054.md) **∥** Solution Architect Device Settings rebind |
| Queued | — | Quality Assurance Engineer then Developer EP-038 after EP-054; barrel after SRS rebind + BDD. Developer **blocked** until `/init` (`paths.src` empty). |

Cameras independent by default. [ADR-0029](../../.docs/adr/ADR-0029-independent-cameras-viewport-follow.md) accepted. [ADR-0023](../../.docs/adr/ADR-0023-viewport-last-writer.md) superseded.

Pen-button map: [REQ-18](../../.docs/modules/epaper/prd.md#pen-buttons) catalogues/dispatch. [REQ-20](../../.docs/modules/epaper/prd.md#device-settings) Settings shell + on-device persist. Infini [REQ-05](../../.docs/modules/infini/prd.md#pen-button-map) persist/restore **retired**. [ADR-0030](../../.docs/adr/ADR-0030-tablet-authors-pen-button-map.md) persist split pending supersede.

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
personas: /designer EP-054 ∥ /architect Device Settings
forbidden: REQ-15 tables; REQ-08; CHL-0011; CHL-0012; EP-032; AI; last-writer ADR-0023; do not /dev until design done + BDD; do not /dev until /init (paths.src empty)
NOW: W-empty designer STORY-EP-054 ∥ architect Device Settings rebind
cursor: /designer STORY-EP-054; /architect REQ-20 / CHL-0025
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
| **W-empty** | **NOW** | **∥** | Designer EP-054 (`hand-touch/` delta) **∥** Architect Device Settings (SRS/ADR; not `hand-touch/`) |
| **W2** | queued | serial | Quality Assurance Engineer then Developer EP-038 (after EP-054). **Blocked** until `/init`. |
| **W3** | queued | | erase / clipboard design |
| **W4** | queued | | connector ends / attachments |
| **W5** | queued | | barrel BDD then EP-052 / EP-058 / EP-057; then EP-039 local two-finger; IN-033 after IN-037. IN-035 cancelled. |
| **W6** | queued | | manual create |
| **Frozen** | — | | REQ-15 · REQ-08 · EP-035 parking |

### Parallelism rules (current wave)

| Lane | Story | Writes | Conflicts |
|---|---|---|---|
| **A** | [STORY-EP-054](./stories/STORY-EP-054.md) | `.plan/iter-005/design/hand-touch/` | do not edit `pen-button-map/` or follow packages |
| **B** | Architect Device Settings rebind | `.docs/modules/**/srs-*.md`, `.docs/adr/`, `.docs/domain/` | do not edit `hand-touch/` HTML; do not edit stories / MASTER / board |

Work-in-progress 2 = these two lanes. Same-file writers forbidden.

### Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group | Progress Detail |
|---|---|---|---|---|---|---|---|---|---|
| F-10 | REQ-10 hand-touch | Must | PRD 10 mm named | EP-037 done; [STORY-EP-054](./stories/STORY-EP-054.md) **in-progress** | NOW | W-empty | Product Designer | A | Palm ≤10 mm; empty pan >10 mm. EP-038 after design + BDD. |
| F-19 | REQ-19 viewport-follow Infini | Must | SRS bound | [STORY-EP-053](./stories/STORY-EP-053.md) **done** | done | W-follow | Quality Assurance Engineer then Developer EP-055 | — | Icon toggle; not ToolChip. |
| F-IN-06 | Infini REQ-06 viewport-follow Epaper | Must | SRS bound | [STORY-IN-036](./stories/STORY-IN-036.md) **done** | done | W-follow | Quality Assurance Engineer then Developer IN-037 | — | Desktop toggle. |
| F-18 | REQ-18 barrel accelerators | Must | PRD 0.11.0-draft | IN-034 historical; [STORY-EP-056](./stories/STORY-EP-056.md) **done** [UI-EP-08](./design/pen-button-map/) | design done | W-pen-map | Quality Assurance Engineer then [STORY-EP-052](./stories/STORY-EP-052.md) | — | Catalogues/dispatch. Settings shell is REQ-20. |
| F-20 | REQ-20 Device Settings | Must | PRD minted 0.11.0-draft | EP-056 painted Settings; persist/UI implement draft | NOW bind | W-pen-map | Solution Architect then EP-058 / EP-057 | B | On-device persist. [CHL-0025](./challenges/CHL-0025-pen-map-settings-page.md) adopted. GAP-01 adopted. |
| F-IN-05 | Infini REQ-05 persist map | — | **retired** | [STORY-IN-035](./stories/STORY-IN-035.md) **cancelled** | cancelled | — | — | — | Persist moved to REQ-20 / EP-057. |
| F-11 | REQ-11 erase | Must | — | EP-040 queued | queued | W3 | designer | — | — |
| F-12 | REQ-12 clipboard | Must | — | EP-043 queued | queued | W3 | designer | — | — |
| F-13 | REQ-13 endpoint styles | Should | — | EP-045 queued | queued | W4 | designer | — | — |
| F-14 | REQ-14 attachments | Should | — | EP-048 queued | queued | W4 | designer | — | — |
| F-17 | REQ-17 manual create | Should | — | EP-050 queued | queued | W6 | designer | — | — |
| CHORE-1 | BRD-07 pan/zoom amend | — | **done** 2026-08-20 | — | done | — | — | — | Independent cameras + follow. |
| CHORE-2 | PM srs-product BR-D08 always-on viewport | — | — | — | open | after bind | product-manager | — | Architect flagged; not a design blocker. |
| CHORE-3 | PM adopt GAP-01 pen-map entry tile | — | **adopted** 2026-08-20 | UI-EP-08 | done | — | — | — | Leading 10 mm tile in REQ-20. |
| CHORE-4 | PM/architect triage CHL-0025 Settings page | — | **PM adopted** | UI-EP-08 | NOW architect | now | architect | B | Drop present-sheet / extra scene ids; persist on device. |
| PL-035 | EP-035 enclose A/L | P1 | — | — | ready | parking | Developer later | — | Not this wave. |

### Current-wave sub-agent roster

| Lane | Agent role | Story | Writes | Done when |
|---|---|---|---|---|
| A | designer | STORY-EP-054 Revise hand-touch: palm-rest vs empty local pan | `design/hand-touch/` | ui-spec-gate; palm ≤10 mm vs empty pan >10 mm scenes; story `done` |
| B | architect | Device Settings rebind (REQ-20 / CHL-0025) | `.docs/` SRS, ADR, domain | ADR supersedes 0030 persist; SRS-EP-52/53 no sheets; Infini persist SRS retired; handoff to SM |

Wait: Designer and Architect run this turn. Developer EP-038 / EP-039 / IN-033 **not spawned** — stop line requires behavior-driven scenarios after EP-054; `paths.src` empty until `/init`.

### Backlog sink

| Item | Why |
|---|---|
| REQ-15 tables | human excluded |
| REQ-08 / CHL-0011 / CHL-0012 | parked |
| EP-032 chrome SM | parked iter-004 |
| Infini→Infini follow | forward, not Must |
| Last-writer ADR-0023 | **superseded** — do not implement |
| Infini desktop map editor | **retired in place** (Infini REQ-05 UI outcome; UI-IN-03 superseded) |
| Infini map persist/restore | **retired** (Infini REQ-05); on-device persist is REQ-20 / EP-057 |

---

## Verdict

**NOW** W-empty: [STORY-EP-054](./stories/STORY-EP-054.md) **∥** Architect Device Settings rebind. Work-in-progress 2 holds those two lanes. Do not Developer-implement until behavior-driven scenarios exist **and** `/init` sets implementation roots.
