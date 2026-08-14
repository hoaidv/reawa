---
id: TRACK-003
slug: smart-group-pilot
kind: planned
status: active
iter: iter-003
cursor: "W12 done — /pm gate + human campaign verify (REQ-04…07)"
goal: "Ink-box rework: epaper REQ-04…REQ-07 (device owns the document) + infini REQ-03 one-way sync"
owner: sm
---

# TRACK-003 — Smart Group pilot → document ownership rework

## Goal

Epaper owns the working document in-session: on-device ink-box create + manipulate, one-way sync.
ADRs: [ADR-0014](../../.docs/adr/ADR-0014-document-ownership-inversion.md),
[ADR-0015](../../.docs/adr/ADR-0015-one-way-sync-contract.md).
Pilot ADRs [ADR-0011](../../.docs/adr/ADR-0011-smart-group.md) (semantics) and
[ADR-0013](../../.docs/adr/ADR-0013-ink-box-tool-modes.md) (tool arming) survive where not reversed.

## Stories (re-slice)

| Id | Kind | Status | Notes |
|---|---|---|---|
| [STORY-EP-012](../iter-003/stories/STORY-EP-012.md) | design | **done** | W8 — `[UI-EP-02]`; CHL-0010 + du confirm remain |
| [STORY-EP-013](../iter-003/stories/STORY-EP-013.md) | implement | **done** | W8 — RM2 latency gate passed |
| [STORY-EP-014](../iter-003/stories/STORY-EP-014.md) | implement | **done** | W9 — RM2 ingest p95=231 µs; arrival→flush 1392 µs |
| [STORY-EP-015](../iter-003/stories/STORY-EP-015.md) | implement | **done** | W9 — snapshot ring depth 20; no chrome (CHL-0010) |
| [STORY-IN-027](../iter-003/stories/STORY-IN-027.md) | implement | **done** | W9 — desktop applier; WorldLayer from mirror |
| [STORY-EP-016](../iter-003/stories/STORY-EP-016.md) | implement | **done** | W10 — enclose; human PASS 2026-08-13 |
| [STORY-IN-029](../iter-003/stories/STORY-IN-029.md) | implement | **done** | W10b — Device Log; human accepted |
| [STORY-EP-021](../iter-003/stories/STORY-EP-021.md) | implement | **done** | W10b — log shipper; human accepted |
| [STORY-EP-017](../iter-003/stories/STORY-EP-017.md) | implement | **done** | W10 — draw-into membership (host PASS) |
| [STORY-EP-022](../iter-003/stories/STORY-EP-022.md) | design | **done** | W11a — UI-EP-03 marquee + Enclose |
| [STORY-EP-018](../iter-003/stories/STORY-EP-018.md) | implement | **done** | W11a — human PASS 2026-08-14 |
| [STORY-EP-019](../iter-003/stories/STORY-EP-019.md) | implement | **done** | W11b — live manip + descriptor |
| [STORY-EP-023](../iter-003/stories/STORY-EP-023.md) | design | **done** | W11b — UI-EP-02 four-tool rebase |
| [STORY-EP-025](../iter-003/stories/STORY-EP-025.md) | implement | **done** | W11c — chrome layers; human PASS; CHL-0018 option 1 |
| [STORY-EP-020](../iter-003/stories/STORY-EP-020.md) | implement | **done** | W12 — device sync; human confirm 2026-08-14 |
| [STORY-IN-028](../iter-003/stories/STORY-IN-028.md) | implement | **done** | W12 — desktop `doc_load`; human confirm 2026-08-14 |

Pilot stories (IN-010…019, EP-003…006) remain **done** as history. Residue EP-007…011 / IN-020…026 stays **blocked** — not re-sliced (desktop ink-box authoring is out; device behaviour is the new ids above).

## Board

[execution-board](../iter-003/execution-board.md)

## Freeze note (2026-08-11)

- **Trigger:** Human restored `Epaper/` + `infini/` to latest commit; directed **total architecture rework**.
- **In-flight abandoned:** CHL-0007 hotfixes (not in tree); W7 verify-fix (EP-006 / IN-019) and any CHL-0004…0006 code paths beyond HEAD.
- **Plan residue (uncommitted):** challenges CHL-0004…0007, stories EP-008…011 / IN-023…026, related handoffs — keep as evidence; do not schedule until PM/architect.
- **Risks if resumed blindly:** patch stack on ADR-0013 selection/ghost/snapshot model already failed human verify repeatedly (residue, snap-back, enclose desync, GUI freeze from mid-drag rasterize).
- **Resume checklist:** PM resolves CHL-0008 → Architect redesign/ADR → SM re-slice → QA → Dev. Re-run gates on both modules before calling pilot verified.

**2026-08-13:** Checklist completed through SM re-slice. Track **unpaused**. W7 still void.

## Log

| Date | Note |
|---|---|
| 2026-08-11 | Opened after iter-002 retro-gate |
| 2026-08-11 | PM ink-box UX; scope + epaper; WIP 2 |
| 2026-08-11 | Architect ADR-0013 + SRS; then confirm IN-15/16 + UV |
| 2026-08-11 | SM sliced 11 stories; cursor → designer ∥ qa→dev |
| 2026-08-11 | PM adopted CHL-0001/2/3 (create_refused, epaper platform, floating ToolChip) |
| 2026-08-11 | Human verify FAILED — toolbar touch + late connection; sliced EP-006 + IN-019 |
| 2026-08-11 | **Paused** — CHL-0008 architecture rework; code at HEAD |
| 2026-08-13 | PM **adopted** CHL-0008 — Epaper owns the document; one-way sync. Pilot slicing void; await architect |
| 2026-08-13 | Architect delivered ADR-0014 + ADR-0015, the shared domain doc, `SRS-EP-07`…`SRS-EP-14`, device BDD, and both architecture views — [handoff to SM](../iter-003/handoffs/2026-08-13-architect-to-sm-device-document.md) |
| 2026-08-13 | SM re-sliced W8–W12 (EP-012…020, IN-027…028). Opened [CHL-0009](../iter-003/challenges/CHL-0009-missing-device-document-srs-logic.md) for missing `srs-logic.md`. Track **active**. Cursor → designer EP-012 ∥ dev EP-013 |
| 2026-08-13 | W8 launched in parallel: EP-012 (designer) ∥ EP-013 (dev) ∥ CHL-0009 (PM adopt + architect `srs-logic.md`) |
| 2026-08-13 | CHL-0009 **adopted** — `srs-logic.md` landed (`SRS-EP-07` / `SRS-EP-08`). SM flipped EP-014/015/020 `blocked` → `draft`. W9 still waits on EP-013. |
| 2026-08-13 | EP-013 **in-review** — host probe (hit-test ~1 µs, 0 drops, paint loop clean). Device pen-down → pixel p95 **not claimed**. Cursor → `/qa`. Do not ungate W9. |
| 2026-08-13 | EP-012 **done** `[UI-EP-02]`. EP-018/019 stay `draft`. Opened [CHL-0010](../iter-003/challenges/CHL-0010-undo-vs-selection-create-chrome.md). Handle 28/56 du + LOD 96 du await architect. |
| 2026-08-13 | EP-013 **done** — RM2 arrival→flush p95=1298 µs, hit-test p95=22 µs, dropped=0. SM ungated W9: EP-014 ∥ IN-027 `ready`. EP-015 stays draft. |
| 2026-08-13 | EP-012 spike **closed**: CHL-0010 **deferred** (EP-018 frozen; enclose is create path). Architect accepted 28/56/96 du for EP-019. |
| 2026-08-13 | W9 BDD walk **done** — thin `ingest-stroke.feature` for EP-014; IN-027 stays on `session-channels.feature`. Devs already in-progress. |
| 2026-08-13 | IN-027 **in-review** — `receiveDocChange` → VectorDocument; WorldLayer from mirror. Cursor → `/qa`. IN-028 still draft. |
| 2026-08-13 | IN-027 **done** (QA PASS, 80/80). Leftover `@future` on epoch `doc_load` stays for IN-028. Cursor → EP-014 only. |
| 2026-08-13 | EP-014 **in-review** — DeviceDocument + pen-up `append_ink`; host ingest p95 ~12 µs; `ops/` 100% vs Infini. Cursor → `/qa`. EP-015 stays draft. |
| 2026-08-13 | EP-014 QA **HOLD** — host PASS (p95=15 µs); RM2 `10.11.99.1` unreachable. Status stays in-review. Do not flip EP-015. Re-run `/qa` when tablet is on USB. |
| 2026-08-13 | EP-014 **done** — RM2 USB via `en7`: ingest p95=231 µs, ink_nodes=40, arrival→flush p95=1392 µs, dropped=0. SM flipped EP-015 `draft` → `ready`. Cursor → `/qa` then `/dev`. No chrome (CHL-0010). IN-028 / EP-016+ stay draft. |
| 2026-08-13 | W9 **auto** — QA walk READY-FOR-DEV; Dev shipped `commitOp` + depth-20 ring; QA verify **PASS** (undo p95=2 µs host). EP-015 **done**. W9 complete. EP-016 stays `draft`. |
| 2026-08-13 | W10 **auto** opened — EP-016 `ready` only. Human stop after ship for RM2 drawing check. EP-017 stays draft. |
| 2026-08-13 | EP-016 **in-review** — host PASS (enclose + fixtures 100% vs Infini). Non-synth epaper pid 5532 on RM2. **STOP** for human draw. EP-017 draft. |
| 2026-08-13 | Human: Device Log (option 2, `:9878`) before continuing W10. Architect SRS-IN-17…19 / SRS-EP-15…16. SM sliced IN-029 ∥ EP-021 **ready**. No designer (needs_design: false). |
| 2026-08-13 | W10b **in-review** — Infini overlay + `:9878`; Epaper worker shipper + `[enclose]` qInfo. Host tests PASS. RM2 deploy WAIT (`en7` link-local, no `10.11.99.12`). EP-016 stays in-review. |
| 2026-08-13 | PM **CHL-0011 Adopt → future** — nested enclose (Smart Groups as content) out of this campaign. |
| 2026-08-13 | Human enclose PASS → EP-016 **done**. SM continues W10: EP-017 membership **ready** (flat only). |
| 2026-08-13 | PM **CHL-0012 Adopt → future** — FREE_FORM / WRAP_CONTENT sizing + align-content. EP-017 unchanged. |
| 2026-08-13 | W10 auto — Device Log **done** (human accept). EP-017 membership **done** (host PASS). Opened CHL-0013 for EP-018 UX → `/pm`. |
| 2026-08-13 | CHL-0013 **Adopted**; ADR-0016; SRS thickened; EP-022 design **ready**. Cursor → `/designer`. |
| 2026-08-14 | EP-022 **done**. EP-018 **in-review** (four-chip, marquee/lasso, Enclose, ≥80% membership). Cursor → `/qa` on-panel. W11b gated. |
| 2026-08-14 | EP-018 **done** (human PASS). W11a closed. W11b: EP-019 **ready**. Cursor → `/qa`. |
| 2026-08-14 | W11b QA walk + EP-023 four-tool rebase **done**. EP-019 **ready** → `/dev`. |
| 2026-08-14 | EP-019 **in-review** — descriptor router + live transform; host `manipulate_test` PASS. Cursor → `/qa`. |
| 2026-08-14 | EP-019 **done** — human RM2 confirm. W11b closed. Cursor → W11c `/dev` EP-025. |
| 2026-08-14 | EP-025 **done** — human PASS (ToolCanvas Pen in-flight / Mono settled; option 1 live node). CHL-0018 adopted. W11c closed. W12: EP-020 ∥ IN-028 **ready**. Cursor → `/qa`. |
| 2026-08-14 | W12 **done** — EP-020 ∥ IN-028 QA PASS; human confirm tablet↔desktop (ink-box create / membership / move-resize). Cursor → `/pm` gate + campaign verify. No further implement wave in lock. |
