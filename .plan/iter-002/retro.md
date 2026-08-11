---
iter: iter-002
date: 2026-08-11
status: complete
participants: analyst, pm, architect, sm, designer, dev, qa
---

# Iter 002 Retrospective

> SM close-iter 2026-08-11. PM runs retro-gate before iter-003.

## What went well

- Vertical Infini↔Epaper campaign delivered Must path: infinity canvas → vector library → live tablet sync (W4 + W5).
- Human-in-the-loop hardware fixes landed fast (digitizer Round 19, vector `doc_snapshot`, gut orientations, stroke×zoom).
- Code-truth rewrite aligned PRD/SRS/protocol with shipped wire (`viewport` / `doc_snapshot` / `stroke_*`) instead of drifting on aspirational `append_ink`.
- Clear park decisions: DocChrome cancelled; Smart Group Could parked (IN-010); no Must from `@future` BDD.
- QA automated evidence (vitest + regionsync_test) + PASS-WITH-CONCERNS gates kept momentum without fake green.

## What to improve

- Dual SoT (WorldLayer live vs `VectorDocument` library) and dual Epaper path (Qt vs unwired `regionsync/`) confused docs until late rewrite — name interim earlier.
- ADR-0009 target op-log vs interim `stroke_*` should have been an explicit amendment sooner.
- Repo-wide `adlc gate --check` noise (`reawa/*`) obscures campaign gate signal.
- Smart Group REQ-04 Needs design unpaid while library ops existed — sequence design before Could implement next time.
- Human “explicit all-green ack” vs bug-fix loop as validation — tighten gate language.

## Iter memory reviewed

- _none written_ under `.plan/iter-002/memory/`

## Memory captured

- **Project** → _none promoted this close_
- **ADLC** → _none proposed this close_

## Upstream signals

- Gate engine mixes lock-scoped and repo-wide FAILS (`reawa/*`) — hard to read campaign green.
- “Code is source of truth” mid-campaign worked but needs a named PM ritual (docs sync wave) in the playbook.
- `_none further_`

## Persona reflections

- **analyst**: EXP-0001 → Infini/Epaper REQs held; interim wire vs ADR target was the main discovery debt to label earlier.
- **pm**: W4/W5 gates READY-WITH-CONCERNS correct; Must exit met; IN-010/DocChrome park decisions held; next iter awaits Smart Group requirements from human.
- **architect**: Code-truth SRS/ADR-0009 amendment restored trust; keep WorldLayer vs tree and Qt vs `regionsync/` as explicit migration ADRs next.
- **sm**: Board/track discipline worked once W5 closed; no new Must from `@future` BDD was the right freeze; carry IN-010 only.
- **designer**: F1 infinity-canvas package done; DocChrome (IN-006) correctly cancelled — no orphan design WIP to promote (`_none — no system WIP beyond F1 already accepted_`).
- **dev**: Live path = CanvasStage + StrokeSync + gut UV + world stroke width; library VectorDocument unit-tested but not live-wired — next wave must pick one SoT.
- **qa**: Automated suites green; hardware concerns reduced via bug loop; still prefer explicit human checklist for gut poses + settle sharpness on future waves.

## Carry-over to iter-003

| Item | Disposition |
|---|---|
| STORY-IN-010 Smart Group | **carry** draft → iter-003 (await PM requirements) |
| STORY-IN-006 DocChrome | stays **blocked**/cancelled |
| `doc_op` / `regionsync/` migration | backlog — not auto-committed |
| Dual SoT / interim wire | known debt — architect when PM opens wave |
