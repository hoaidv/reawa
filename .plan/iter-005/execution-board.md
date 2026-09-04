---
title: Execution board — hand-on-paper
iter: iter-005
track: TRACK-005
owner: sm
date: 2026-09-04
lock: vertical · verified · wip 2
wave: W-field-latency
verdict: "STORY-EP-069 done. Field follow-ups EP-070…072 ready. Clipboard STORY-EP-044 done (human-verified 2026-09-04). STORY-EP-073 later."
---

# Execution board — hand-on-paper

**Canonical board** for [TRACK-005](../tracks/TRACK-005-hand-on-paper.md). Track **active**. Tool-system interrupt [TRACK-006](../tracks/TRACK-006-tool-system-refactor.md) **done** 2026-08-27. Inverse-undo implement **done**. Erase product bound 2026-08-29. Erase implement **human-verified** 2026-08-31. [STORY-EP-069](./stories/STORY-EP-069.md) **done** (human-verified 2026-08-31). Camera LatestJob field: **better**. Field follow-ups [STORY-EP-070](./stories/STORY-EP-070.md)…[STORY-EP-072](./stories/STORY-EP-072.md) **ready**. Clipboard [STORY-EP-044](./stories/STORY-EP-044.md) **done** (human-verified 2026-09-04). [STORY-EP-073](./stories/STORY-EP-073.md) later.

---

## Summary (as of 2026-09-04)

| Band | Count | Meaning |
|---|---|---|
| Design **done** | 5 current | [UI-EP-06](../../.docs/design/index.md) hand-touch (EP-054; field-test delta 20 mm + HT) · [UI-EP-07](../../.docs/design/index.md) follow Epaper · [UI-IN-04](../../.docs/design/index.md) follow Infini · [UI-EP-08](../../.docs/design/index.md) Device Settings · Pen buttons |
| Wave **NOW** | 3 ready | [STORY-EP-070](./stories/STORY-EP-070.md)…[STORY-EP-072](./stories/STORY-EP-072.md) field latency / probes. Not in-progress. |
| Closed interrupt | TRACK-006 | Tablet/Tool split + [ADR-0033](../../.docs/adr/ADR-0033-tool-abstraction.md) + pointer roles. Do **not** continue. |
| Queued | — | [STORY-EP-073](./stories/STORY-EP-073.md) clipboard clipops split later. Device Settings still wait. Follow hardware score still outstanding. Infini undo apply parked (IN-038 cancelled). |

Cameras independent by default. [ADR-0029](../../.docs/adr/ADR-0029-independent-cameras-viewport-follow.md) accepted. [ADR-0023](../../.docs/adr/ADR-0023-viewport-last-writer.md) superseded.

Pen-button map: [REQ-18](../../.docs/modules/epaper/prd.md#pen-buttons) catalogues/dispatch. [REQ-20](../../.docs/modules/epaper/prd.md#device-settings) Settings shell + on-device persist. Infini [REQ-05](../../.docs/modules/infini/prd.md#pen-button-map) persist/restore **retired**. [ADR-0031](../../.docs/adr/ADR-0031-device-settings-persist-on-epaper.md) accepted (supersedes [ADR-0030](../../.docs/adr/ADR-0030-tablet-authors-pen-button-map.md) persist split).

Tool system: [ADR-0033](../../.docs/adr/ADR-0033-tool-abstraction.md) accepted. Default DeviceMap Primary=Pen, Secondary=Finger. `InkMode` + `SecondaryDeviceModifier`. EraserMode body is W3, not TRACK-006.

---

## Lock (copy into every sub-agent brief)

```
direction: vertical
stop_line: verified
autonomy: bounded
wip: 2
out_of_scope: backlog
modules: epaper, infini
features: epaper/ink-box; epaper/tool-modes; epaper/connector-ink; epaper/region-sync; epaper/local-pen-ink; epaper/device-document; epaper/erase; infini/infinity-canvas; infini/tablet-sync; infini/vector-document
personas: developer, human
forbidden: REQ-15 tables; REQ-08; CHL-0011; CHL-0012; EP-032; AI; last-writer ADR-0023; TRACK-006 reopen; DeviceMap invert UI; Mouse DragHandler; STORY-IN-038; infini undo apply; tablet-to-desktop undo sync
NOW: field follow-ups EP-070…072 ready; clipboard product done (EP-044); do not start EP-073
cursor: pick EP-070 or EP-071 or EP-072 (WIP 2); do not continue TRACK-006; IN-038 cancelled
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
| **W-field** | **partial** | — | Hand-touch **approved**. Remaining: Infini + Epaper follow score (human, outstanding) |
| **W-tool-sys** | **done** 2026-08-27 | serial | Expedite [TRACK-006](../tracks/TRACK-006-tool-system-refactor.md): Tablet/Tool split, ADR-0033, dissolve bags, pointer roles. Closed by human. |
| **W-undo-adr** | **done** 2026-08-27 | serial | Product Manager adopt [CHL-0026](./challenges/CHL-0026-inverse-op-undo.md). Architect accept [ADR-0032](../../.docs/adr/ADR-0032-inverse-op-undo.md). Scrum Master sliced EP-059…061 + IN-038. |
| **W-undo-impl** | **done** 2026-08-27 | serial | Quality Assurance Engineer then Developer [STORY-EP-059](./stories/STORY-EP-059.md) **done** |
| **W-undo-skip** | merged into W-undo-local | — | EP-060 |
| **W-undo-wire** | **dropped** | — | Infini apply cancelled (IN-038). Device queue is EP-061 in W-undo-local |
| **W-undo-local** | **done** 2026-08-27 | serial (one Quality Assurance Engineer then one Developer; both stories) | [STORY-EP-060](./stories/STORY-EP-060.md) + [STORY-EP-061](./stories/STORY-EP-061.md) **done**. Human verified device undo/redo 2026-08-27. No `infini/` |
| **W3** | **done** 2026-08-31 | | erase EP-062…068 **done**; [STORY-EP-069](./stories/STORY-EP-069.md) **done** (human-verified) |
| **W-field-latency** | **NOW** (ready, not started) | **∥** (different write sets; WIP 2) | [STORY-EP-070](./stories/STORY-EP-070.md) residual ink lag · [STORY-EP-071](./stories/STORY-EP-071.md) selection settle probe · [STORY-EP-072](./stories/STORY-EP-072.md) camera stress probe |
| **W-clipboard** | **done** 2026-09-04 | | [STORY-EP-044](./stories/STORY-EP-044.md) in-document copy/cut/paste. [STORY-EP-043](./stories/STORY-EP-043.md) cancelled. |
| **W-clipboard-refactor** | later (draft) | | [STORY-EP-073](./stories/STORY-EP-073.md) split clipops — not NOW |
| **W4** | queued | | connector ends / attachments |
| **W5** | queued | | barrel BDD then EP-052 / EP-058 / EP-057. IN-035 cancelled. |
| **W6** | queued | | manual create |
| **Frozen** | — | | REQ-15 · REQ-08 · EP-035 parking · DeviceMap invert UI · Mouse DragHandler |

### Parallelism rules (current wave)

| Lane | Story | Writes | Conflicts |
|---|---|---|---|
| **A** | [STORY-EP-070](./stories/STORY-EP-070.md) | ink-path / TabletCanvas sample path | same `tabletcanvasitem.cpp` as C — not parallel with EP-072 without a stitch owner |
| **B** | [STORY-EP-071](./stories/STORY-EP-071.md) | SelectionOverlay / select Operations settle | overlay files; can run beside A if A stays in ink-path |
| **C** | [STORY-EP-072](./stories/STORY-EP-072.md) | rasterize probe / camera job log | `tabletcanvasitem.cpp` + `rasterize_probe` — conflicts with A |

Work-in-progress 2 = campaign capacity. Start at most two of A/B/C; **do not** run A and C in parallel (same canvas rasterize file).

### Full task table

| Id | Feature / chore | Pri | Docs | Design story | Status | Wave | Next owner | Parallel group | Progress Detail |
|---|---|---|---|---|---|---|---|---|---|
| F-10 | REQ-10 hand-touch | Must | PRD 0.12.0-draft: 20 mm / 178 du; ≥3 contacts; HT toggle default on | EP-037 + EP-054 **done** (UI-EP-06 amended 2026-08-20) | EP-038 + EP-039 **done**; **human-approved** 2026-08-20 | W2 / W-field | Product Manager on CHL-0027 | — | Host tests verified. Field-test lock: palm **20 mm**, ≥3 contacts = palm, **HT** kill-switch. Open: [CHL-0027](./challenges/CHL-0027-palm-travel-not-contact-count.md) (drop 3-contact eat; rely on travel) — Software Requirements Specification still says ≥3. Residual: no device/Qt `epaper_bin` in this environment. |
| F-19 | REQ-19 viewport-follow Infini | Must | SRS bound | [STORY-EP-053](./stories/STORY-EP-053.md) **done** | EP-055 **done** | W-follow-impl | human field test | — | Toggle verified on host. Residual: no device/Qt build. |
| F-IN-06 | Infini REQ-06 viewport-follow Epaper | Must | SRS bound | [STORY-IN-036](./stories/STORY-IN-036.md) **done** | IN-037 + IN-033 **done** | W-follow-apply | human field test | — | Toggle + apply-while-following host-verified (`cd infini && npm test` 128 passed). Residual: no live TCP `:9877` / no RM2. |
| F-18 | REQ-18 barrel accelerators | Must | PRD 0.11.0-draft | IN-034 historical; [STORY-EP-056](./stories/STORY-EP-056.md) **done** [UI-EP-08](./design/pen-button-map/) | design done | W-pen-map | Quality Assurance Engineer then [STORY-EP-052](./stories/STORY-EP-052.md) | — | Catalogues/dispatch. Settings shell is REQ-20. **Queued until after field test.** |
| F-20 | REQ-20 Device Settings | Must | PRD 0.11.0-draft; ADR-0031 | EP-056 painted; persist/UI implement draft | bind done | W-pen-map | Quality Assurance Engineer then EP-058 / EP-057 | — | On-device persist. [CHL-0025](./challenges/CHL-0025-pen-map-settings-page.md) adopted. GAP-01 adopted. **Queued until after field test.** |
| F-IN-05 | Infini REQ-05 persist map | — | **retired** | [STORY-IN-035](./stories/STORY-IN-035.md) **cancelled** | cancelled | — | — | — | Persist moved to REQ-20 / EP-057. |
| F-11 | REQ-11 erase | Must | [prd-erase.md](../../.docs/modules/epaper/prd-erase.md); [SRS-EP-54](../../.docs/modules/epaper/features/erase/srs-logic.md)…59; [ADR-0034](../../.docs/adr/ADR-0034-erase-clip-remnants.md); [ADR-0036](../../.docs/adr/ADR-0036-toolcanvas-live-overlay.md) | Icons only (no EP-040 package) | EP-062…068 **done** (human-verified 2026-08-31) | W3-erase-ids | — | — | Path A/B cancelled. Human is Quality Assurance Engineer. Brush 2026-08-29; generateNodeId, overlay, area, object 2026-08-31. Gotcha: do not restroke the live object-erase polyline — see [.docs/memory/object-erase-and-connector-ux2.md](../../.docs/memory/object-erase-and-connector-ux2.md). |
| F-12 | REQ-12 clipboard | Must | [SRS-EP-31](../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-31-clipboard); [ADR-0037](../../.docs/adr/ADR-0037-device-clipboard-singleton.md); [CHL-0031](./challenges/CHL-0031-clipboard-tap-paste.md) adopted | EP-043 **cancelled** | [STORY-EP-044](./stories/STORY-EP-044.md) **done** (human-verified 2026-09-04) | W-clipboard | — | — | Product complete. Structure follow-up [STORY-EP-073](./stories/STORY-EP-073.md) later. |
| F-13 | REQ-13 endpoint styles | Should | — | EP-045 queued | queued | W4 | designer | — | — |
| F-14 | REQ-14 attachments | Should | — | EP-048 queued | queued | W4 | designer | — | — |
| F-17 | REQ-17 manual create | Should | — | EP-050 queued | queued | W6 | designer | — | — |
| CHORE-1 | BRD-07 pan/zoom amend | — | **done** 2026-08-20 | — | done | — | — | — | Independent cameras + follow. |
| CHORE-2 | PM srs-product BR-D08 always-on viewport | — | — | — | open | after bind | product-manager | — | Architect flagged; not a design blocker. |
| CHORE-3 | PM adopt GAP-01 pen-map entry tile | — | **adopted** 2026-08-20 | UI-EP-08 | done | — | — | — | Leading 10 mm tile in REQ-20. |
| CHORE-4 | PM/architect triage CHL-0025 Settings page | — | **adopted + rebound** | UI-EP-08 | done | — | — | — | ADR-0031; SRS-EP-52/53 no sheets; Infini persist SRS retired. |
| CHORE-5 | Inverse-op undo (not snapshots) | Must | [CHL-0026](./challenges/CHL-0026-inverse-op-undo.md) **adopted**; [ADR-0032](../../.docs/adr/ADR-0032-inverse-op-undo.md) **accepted** | EP-059…061 **done**; IN-038 **cancelled** | **done** (device local) **human-verified** 2026-08-27 | W-undo-local | human | — | Quality Assurance Engineer PASS then human confirm. Resize-after-move undo fixed (shared `sst-N` op id). Tablet→desktop undo apply deferred. |
| STORY-EP-059 | Device inverse undo ring and lastOpId | P0 | SRS-EP-07 / SRS-EP-09 | — | **done** | W-undo-impl | — | — | Host suite green. 12 undo-ring scenarios. 0 restore_snapshot on undo. Human verified 2026-08-27. |
| STORY-EP-060 | Undo fail-safe skip and no-op catalogue | P0 | SRS-EP-13 | — | **done** | W-undo-local | — | A | Quality Assurance Engineer PASS 2026-08-27. 4 fail-safe scenarios. Human verified 2026-08-27. |
| STORY-EP-061 | Device undo queue is counterpart compound not restore snapshot | P0 | SRS-EP-08 | — | **done** | W-undo-local | — | A | Quality Assurance Engineer PASS 2026-08-27. 5 queue scenarios. `infini/` not edited. Human verified 2026-08-27. |
| STORY-IN-038 | Infini applies compound and set ink samples | P0 | SRS-IN-09 / IN-07 / IN-06 | — | **cancelled** | — | — | — | Human 2026-08-27: skip tablet→desktop undo sync until independent sync algorithm. |
| CHORE-6 | Tool-system refactor (ADR-0033) | — | ADR-0033 **accepted** | none (no story) | **done** 2026-08-27 | W-tool-sys | — | — | [TRACK-006](../tracks/TRACK-006-tool-system-refactor.md) closed. Pointer roles Primary/Secondary landed (`dc379d6`). Finger lasso/marquee off (`dd57741`). Do not reopen. |
| STORY-EP-062 | Eraser mode, ToolChip, barrel last-used | P0 | SRS-EP-54 | icons delivered (no design story) | **done** | W3-erase-ids | — | — | Human-verified 2026-08-29 with brush path. |
| STORY-EP-063 | Geometric clip, remnant split, boundary polyline | P0 | SRS-EP-55 | — | **done** | W3-erase-ids | — | — | Human-verified 2026-08-29. Remnant `{id}_rN` skip-taken is a stopgap for EP-067. |
| STORY-EP-064 | Brush erase capsule clip | P0 | SRS-EP-56 | — | **done** | W3-erase-ids | — | — | Human-verified 2026-08-29: ghost + commit. Commit failure was duplicate remnant ids, not clip miss. |
| STORY-EP-065 | Area erase clip and fully-inside remove | P0 | SRS-EP-57 | — | **done** | W3 | — | — | Human-verified 2026-08-31. |
| STORY-EP-066 | Object erase 80 percent table | P0 | SRS-EP-58 | — | **done** | W3 | — | — | Human-verified 2026-08-31. Append-only dashed raster; deletion-rect outline dirty. [ADR-0036](../../.docs/adr/ADR-0036-toolcanvas-live-overlay.md). |
| STORY-EP-067 | Singleton generateNodeId for all tree nodes | P0 | SRS-EP-07 / EP-08 / EP-55 / IN-09 | — | **done** | W3-erase-ids | — | A | Human-verified 2026-08-31. Folded remnant / stroke / connector / enclose mint sites. Opaque `n-N`. |
| STORY-EP-068 | Operations own overlay paint; ToolCanvasContext stays generic | P0 | SRS-EP-04 / EP-12 / EP-56 | — | **done** | W3-erase-ids | — | B | Human-verified 2026-08-31. Hover + waveform on Operations. |
| STORY-EP-069 | ToolContextImpl host ports and SelectionOverlay | P0 | SRS-EP-04 / EP-12 | — | **done** | W3-erase-ids | — | — | Human-verified 2026-08-31. [ADR-0035](../../.docs/adr/ADR-0035-tool-context-is-host-ports.md); `SelectionOverlay` host-owned; grep clean. Seeing `selection_overlay.cpp` **is** this story — already delivered. |
| STORY-EP-070 | Residual pen-to-ink lag on moderately dense pages | P0 | SRS-EP-01 / EP-03 | — | **ready** | W-field-latency | developer | A | Human 2026-08-31: ~4 sentences + ink-boxes; small random pen-to-ink gaps. Stroke op fine. Do not FullClear ink ([CHL-0029](./challenges/CHL-0029-settle-is-not-fullclear-on-ink.md)). |
| STORY-EP-071 | Instrument sel_rect and sel_freeform settle to knobs | P0 | SRS-EP-12 / EP-04 | — | **ready** | W-field-latency | developer | B | Human: Selecting overlay smooth; pointer-up → settled rect + knobs slow. Instrument first. |
| STORY-EP-072 | Camera-change stress probe on a full handwriting page | P0 | SRS-EP-03 / EP-24 | — | **ready** | W-field-latency | developer | C | Human: LatestJob camera **better**; need stress log on a full handwriting + ink-box page. Conflicts with EP-070 on `tabletcanvasitem.cpp`. |
| STORY-EP-044 | In-document copy cut paste and tap-origin paste | P0 | SRS-EP-31 / EP-32 / EP-33 / EP-11 | EP-043 **cancelled** | **done** | W-clipboard | — | — | Human-verified 2026-09-04. Slot is process-global singleton. Paste on normal toolbar; no long-press. |
| STORY-EP-073 | Split clipboard clipops into document helpers and actions | P2 | SRS-EP-31 / EP-07 | — | **draft** later | W-clipboard-refactor | developer (when human picks) | — | Document query/mutation → DeviceDocument. Copy/cut/paste → actions. Slot stays in clipboard.hpp. Do not add DocContext clipboard verbs. |
| CHORE-7 | Three exclusive erasers (CHL-0028) | Must | [CHL-0028](./challenges/CHL-0028-eraser-three-tools.md) **adopted**; [ADR-0034](../../.docs/adr/ADR-0034-erase-clip-remnants.md) **accepted** | icons only | EP-062…068 **done** (human-verified 2026-08-31) | W3-erase-ids | — | — | Path A/B retired. Human is Quality Assurance Engineer. |

### Current-wave sub-agent roster

| Lane | Agent role | Story | Writes | Done when |
|---|---|---|---|---|
| A | Developer | [STORY-EP-070](./stories/STORY-EP-070.md) | ink-path / sample path | Device log names residual steal; human confirms gaps gone or e-ink only |
| B | Developer | [STORY-EP-071](./stories/STORY-EP-071.md) | SelectionOverlay settle | Pointer-up → knobs stages + ms in a tail-able log |
| C | Developer | [STORY-EP-072](./stories/STORY-EP-072.md) | camera job / raster probe | Stress recipe + log on a full handwriting page |

Wait: do **not** start A and C together (same `tabletcanvasitem.cpp`). Do **not** start [STORY-EP-073](./stories/STORY-EP-073.md) until the human picks it. Do **not** implement IN-038. Do **not** reopen TRACK-006.

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
| DeviceMap invert user interface | TRACK-006 leftover — parked 2026-08-27 |
| Mouse DragHandler | TRACK-006 leftover — parked 2026-08-27 |
| Infini undo apply (`compound` / `set_ink_samples`) | [STORY-IN-038](./stories/STORY-IN-038.md) **cancelled** 2026-08-27 — waits independent sync algorithm |

---

## Verdict

[STORY-EP-059](./stories/STORY-EP-059.md)…[STORY-EP-061](./stories/STORY-EP-061.md) **done** (human-verified 2026-08-27). [STORY-EP-062](./stories/STORY-EP-062.md)…[STORY-EP-069](./stories/STORY-EP-069.md) **done** (human-verified 2026-08-31). Field follow-ups [STORY-EP-070](./stories/STORY-EP-070.md)…[STORY-EP-072](./stories/STORY-EP-072.md) **ready**. Clipboard [STORY-EP-044](./stories/STORY-EP-044.md) **done** (human-verified 2026-09-04). [STORY-EP-073](./stories/STORY-EP-073.md) later. [CHL-0028](./challenges/CHL-0028-eraser-three-tools.md) **adopted**. [ADR-0034](../../.docs/adr/ADR-0034-erase-clip-remnants.md) **accepted**. [ADR-0035](../../.docs/adr/ADR-0035-tool-context-is-host-ports.md) **accepted**. [ADR-0036](../../.docs/adr/ADR-0036-toolcanvas-live-overlay.md) **accepted**. [ADR-0037](../../.docs/adr/ADR-0037-device-clipboard-singleton.md) **accepted**.
