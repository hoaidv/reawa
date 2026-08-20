---
title: Execution board — hand-on-paper
iter: iter-005
track: TRACK-005
owner: sm
date: 2026-08-20
lock: vertical · verified · wip 2
verdict: "IN-033 done (verified, host). TRACK-005 paused for human Infini + Epaper field test. Do not start W3."
wave: W-follow-apply
---

# Execution board — hand-on-paper

**Canonical board** for [TRACK-005](../tracks/TRACK-005-hand-on-paper.md). Track **paused**.

---

## Summary (as of 2026-08-20)

| Band | Count | Meaning |
|---|---|---|
| Design **done** | 5 current | [UI-EP-06](../../.docs/design/index.md) hand-touch (EP-054 palm/pan) · [UI-EP-07](../../.docs/design/index.md) follow Epaper · [UI-IN-04](../../.docs/design/index.md) follow Infini · [UI-EP-08](../../.docs/design/index.md) Device Settings · Pen buttons |
| Wave **NOW** | W-follow-apply **done** | Follow toggles + Infini apply-while-following **done** (host). **Paused** for human Infini + Epaper deploy. |
| Queued | — | After human confirm: W3 erase design **or** Device Settings BDD. |

Cameras independent by default. [ADR-0029](../../.docs/adr/ADR-0029-independent-cameras-viewport-follow.md) accepted. [ADR-0023](../../.docs/adr/ADR-0023-viewport-last-writer.md) superseded.

Pen-button map: [REQ-18](../../.docs/modules/epaper/prd.md#pen-buttons) catalogues/dispatch. [REQ-20](../../.docs/modules/epaper/prd.md#device-settings) Settings shell + on-device persist. Infini [REQ-05](../../.docs/modules/infini/prd.md#pen-button-map) persist/restore **retired**. [ADR-0031](../../.docs/adr/ADR-0031-device-settings-persist-on-epaper.md) accepted (supersedes [ADR-0030](../../.docs/adr/ADR-0030-tablet-authors-pen-button-map.md) persist split).

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
personas: none (paused)
forbidden: REQ-15 tables; REQ-08; CHL-0011; CHL-0012; EP-032; AI; last-writer ADR-0023
NOW: PAUSE — human deploy Infini + Epaper; IN-033 done
cursor: WAIT human field test; do not start W3
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
| **W-empty** | **done** 2026-08-20 | **∥** | Designer EP-054 (`hand-touch/` palm vs pan) **∥** Architect Device Settings (ADR-0031) |
| **W2** | **done** 2026-08-20 | serial | Developer + Quality Assurance Engineer EP-038 then EP-039 |
| **W-follow-impl** | **done** 2026-08-20 | **∥** | Developer + Quality Assurance Engineer EP-055 ∥ IN-037 |
| **W-follow-apply** | **done** 2026-08-20 | serial | Quality Assurance Engineer then Developer IN-033 (apply while following) |
| **W-field** | **NOW (human)** | — | Deploy Infini + Epaper; score follow on hardware |
| **W3** | queued | | erase / clipboard design — **do not start until human says go** |
| **W4** | queued | | connector ends / attachments |
| **W5** | queued | | barrel BDD then EP-052 / EP-058 / EP-057. IN-035 cancelled. |
| **W6** | queued | | manual create |
| **Frozen** | — | | REQ-15 · REQ-08 · EP-035 parking |

### Parallelism rules (current wave)

| Lane | Story | Writes | Conflicts |
|---|---|---|---|
| **human** | Field test Infini + Epaper | device + desktop | no agent writes |

Work-in-progress 2 = **0 agent lanes**. Do not spawn Designer / Developer / Quality Assurance Engineer until the human resumes.

### Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group | Progress Detail |
|---|---|---|---|---|---|---|---|---|---|
| F-10 | REQ-10 hand-touch | Must | PRD 10 mm named | EP-037 + EP-054 **done** | EP-038 + EP-039 **done** | W2 | — | — | Host tests verified. Residual: no device/Qt `epaper_bin` in this environment. |
| F-19 | REQ-19 viewport-follow Infini | Must | SRS bound | [STORY-EP-053](./stories/STORY-EP-053.md) **done** | EP-055 **done** | W-follow-impl | human field test | — | Toggle verified on host. Residual: no device/Qt build. |
| F-IN-06 | Infini REQ-06 viewport-follow Epaper | Must | SRS bound | [STORY-IN-036](./stories/STORY-IN-036.md) **done** | IN-037 + IN-033 **done** | W-follow-apply | human field test | — | Toggle + apply-while-following host-verified (`cd infini && npm test` 128 passed). Residual: no live TCP `:9877` / no RM2. |
| F-18 | REQ-18 barrel accelerators | Must | PRD 0.11.0-draft | IN-034 historical; [STORY-EP-056](./stories/STORY-EP-056.md) **done** [UI-EP-08](./design/pen-button-map/) | design done | W-pen-map | Quality Assurance Engineer then [STORY-EP-052](./stories/STORY-EP-052.md) | — | Catalogues/dispatch. Settings shell is REQ-20. **Queued until after field test.** |
| F-20 | REQ-20 Device Settings | Must | PRD 0.11.0-draft; ADR-0031 | EP-056 painted; persist/UI implement draft | bind done | W-pen-map | Quality Assurance Engineer then EP-058 / EP-057 | — | On-device persist. [CHL-0025](./challenges/CHL-0025-pen-map-settings-page.md) adopted. GAP-01 adopted. **Queued until after field test.** |
| F-IN-05 | Infini REQ-05 persist map | — | **retired** | [STORY-IN-035](./stories/STORY-IN-035.md) **cancelled** | cancelled | — | — | — | Persist moved to REQ-20 / EP-057. |
| F-11 | REQ-11 erase | Must | — | EP-040 queued | queued | W3 | designer | — | Do not start until human says go. |
| F-12 | REQ-12 clipboard | Must | — | EP-043 queued | queued | W3 | designer | — | Do not start until human says go. |
| F-13 | REQ-13 endpoint styles | Should | — | EP-045 queued | queued | W4 | designer | — | — |
| F-14 | REQ-14 attachments | Should | — | EP-048 queued | queued | W4 | designer | — | — |
| F-17 | REQ-17 manual create | Should | — | EP-050 queued | queued | W6 | designer | — | — |
| CHORE-1 | BRD-07 pan/zoom amend | — | **done** 2026-08-20 | — | done | — | — | — | Independent cameras + follow. |
| CHORE-2 | PM srs-product BR-D08 always-on viewport | — | — | — | open | after bind | product-manager | — | Architect flagged; not a design blocker. |
| CHORE-3 | PM adopt GAP-01 pen-map entry tile | — | **adopted** 2026-08-20 | UI-EP-08 | done | — | — | — | Leading 10 mm tile in REQ-20. |
| CHORE-4 | PM/architect triage CHL-0025 Settings page | — | **adopted + rebound** | UI-EP-08 | done | — | — | — | ADR-0031; SRS-EP-52/53 no sheets; Infini persist SRS retired. |
| PL-035 | EP-035 enclose A/L | P1 | — | — | ready | parking | Developer later | — | Not this wave. |

### Current-wave sub-agent roster

| Lane | Agent role | Story | Writes | Done when |
|---|---|---|---|---|
| human | Field test | Deploy Infini + Epaper | device + desktop | Score IN-033 + follow toggles on hardware; report back in `/sm` |

Wait: **0 agent spawns** until the human resumes. Handoff: [2026-08-20-sm-to-human-field-test.md](./handoffs/2026-08-20-sm-to-human-field-test.md).

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

**Paused.** [STORY-IN-033](./stories/STORY-IN-033.md) **done**. Human deploys Infini + Epaper and tests follow on real hardware. Do **not** start W3.
