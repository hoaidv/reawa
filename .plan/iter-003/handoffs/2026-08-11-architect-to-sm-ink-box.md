---
from: architect
to: sm
iter: iter-003
date: 2026-08-11
subject: ink-box-decomposition
cc: [pm, designer, qa]
verdict: READY-WITH-CONCERNS
---

# Architect → SM — ink-box decomposed; ADR-0013 written, 7 SRS sections to slice

PM adopted tool-armed ink-box creation with a tablet toolbar
([PM handoff](./2026-08-11-pm-to-architect-ink-box-ux.md)). I recorded the decisions, amended
ADR-0011, and decomposed REQ-04 + epaper REQ-03 into specs. `adlc validate`: **no violations**.

## Decision — [ADR-0013](../../../.docs/adr/ADR-0013-ink-box-tool-modes.md)

Six clauses, each with its alternatives recorded:

1. **Tool mode is device-local UI state** — never synced. A remote mode change on a panel that
   refreshes ≥250 ms late is unattributable to the creator.
2. **Intent rides the stroke** — `stroke_begin.intent: ink | enclose`, additive and default-safe.
   Intent attached to the stroke cannot desynchronise from the mode that produced it; a separate
   "set mode" message can.
3. **Infini is the single writer** of the tree. Epaper never fits a rect or tests containment —
   this keeps the epaper Non-Goal intact and avoids a second geometry authority in C++.
4. **Epaper picks locally** against a new `doc_snapshot.pickables[]`, drags a local ghost, and
   emits a narrow `tool_intent` message. Deliberately **not** opening bidirectional `doc_op` —
   that migration stays backlogged; `tool_intent` retires when it lands.
5. **Undo is snapshot-based**, ring depth 20, using the existing `snapshotString()`. Inverse-op
   algebra fails exactly on reparent and coordinate-space changes, which is all this feature does.
6. **The enclose size guard is in world units** (48), not screen px and not a setting — otherwise
   the same gesture succeeds or fails by zoom and differs per device.

ADR-0011 now carries an amendment banner: §4A's propose/accept step is **withdrawn**.

## What you can slice

| SRS | Where | What it covers |
|---|---|---|
| [SRS-IN-10] *(revised)* | infini/vector-document srs-logic | Tool-armed enclose + guards; no proposal step |
| [SRS-IN-11] *(new)* | same | Selection, hit-testing, move/resize, LOD cutoff, op economy |
| [SRS-IN-12] *(new)* | same | Undo ring |
| [SRS-IN-13] *(new)* | infini/tablet-sync srs-logic | `stroke_begin.intent`, `pickables[]`, `tool_intent` |
| [SRS-IN-14] *(new)* | infini/vector-document srs-ui | Desktop tool strip, selection overlay, the two box appearances |
| [SRS-EP-04] *(new)* | epaper/tool-modes srs-logic | Device tool state, input routing, intent emission |
| [SRS-EP-05] *(new)* | epaper/tool-modes srs-ui | Toolbar contract — **needs design** |
| [SRS-EP-06] *(new)* | epaper/tool-modes srs-quality | Tool latency + **ink-latency non-regression** |

`epaper/tool-modes` is a new feature folder (index + 3 SRS). `adlc audit` reports these as orphan
SRS — expected until you slice; that is the ask, not a defect.

## Sequencing I recommend

**Nothing in REQ-04 can be built first.** `CanvasStage.rebuildWithRmInk` converts strokes into
flat WorldLayer primitives, so `VectorDocument` never sees ink and SRS-IN-10/11 have nothing to
operate on. Slice in this order:

1. **Prerequisite (implement):** tree-backed ink ingestion — `append_ink` on the live stroke path,
   paint via the existing `syncFromVectorDoc`. No new UI. Everything else depends on it.
2. **Spike (timeboxed):** RM2 capacitive touch reachable from Qt? This is the single assumption
   that can invalidate epaper REQ-03's design. Do it in parallel with (1) — it blocks the design
   story, not the desktop work.
3. **Design story:** `infini` ink-box UI (SRS-IN-14) — desktop first, where selection and undo
   have to be built anyway.
4. **Implement:** SRS-IN-12 undo → SRS-IN-11 selection/manipulation → SRS-IN-10 enclose.
   Undo first so every later story is safe to exercise by hand.
5. **Design story:** `epaper` toolbar (SRS-EP-05), gated on the spike in (2).
6. **Implement:** SRS-IN-13 transport, then SRS-EP-04 device tool modes.

Both design stories are required by the gate — `adlc gate` currently FAILs
"Needs-design REQs have a design story" for `infini/REQ-04` and `epaper/REQ-03`.

## Concerns (why not plain READY)

1. **Touch is an assumption, not a fact.** ADR-0013's device clauses collapse if the capacitive
   layer is unreachable from the Qt app. The fallback (pen-on-strip or hardware button) is a
   different design, not a tweak. Do not let the epaper design story start before the spike.
2. **Two intent paths coexist** (`stroke_begin.intent` and `tool_intent`) until the ADR-0009
   op-log migration. Accepted debt, logged in `architecture.md` risks.
3. **Rotation is specced out of the pilot**, and designers will want it. The reason is real:
   `anchors.ts` resolves SmartGroup world AABB for translate + scale only, so a rotation handle
   would place connector ports wrong. Landing rotation means doing the anchor math first.
4. **The 48-world-unit guard is a first guess.** QA should treat it as tunable against the ≥80%
   first-try metric rather than as a fixed contract.
5. **`epaper/tool-modes` has no BDD folder yet** — QA owns that after you slice.

## Ask

Plan the waves above, open the two design stories, and set `depends_on` from the implement stories
onto them. Then **`/designer`** for SRS-IN-14, and **`/qa`** for BDD on the revised SRS-IN-10 —
its old propose/accept scenarios are void.
