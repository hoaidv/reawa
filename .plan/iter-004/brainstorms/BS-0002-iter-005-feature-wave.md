---
id: BS-0002
topic: Iter-005 feature wave: natural pen-on-paper, gesture, manual authoring, AI
topic_source: chat — /pm take notes for next features wave in iter 05
date: 2026-08-15
facilitator: pm
participants: [pm]
status: concluded
mode: keep-going
rounds_planned: 1
relates-to:
  - [REQ-03]
  - [REQ-05]
  - [REQ-08]
  - [REQ-09]
  - [REQ-10]
  - [BRD-07]
  - [CHL-0011]
  - [CHL-0012]
---

# BS-0002 — Iter-005 feature wave

Parking lot for the **next campaign after TRACK-004 / iter-004 closes**. Not committed.
No `[REQ-NN]` minted here — requirements land in `author-prd.md` after retro-gate.

**Iter-005 must not open** until PM retro-gate passes for iter-004.

## Topic (from human)

Source: chat, 2026-08-15. Verbatim:

> take notes for next features wave in iter 05
>
> # Natural pen-on-paper
> - eraser : pencil's eraser / selection -> erase
> - selection -> copy/cut -> paste
> - connector: endpoint styles
>   + using context toolbar after connector creation (star, empty arrow, fill arrow, one, many...)
>   + auto-recognition of endpoint style (inks run over the endpoints are consider part of the connector endpoints, and are preserved)
> - connector: middle attachment like text, figures. these attachments are moved with the connector as it is warped during drag
> - table recognition
>
> # Gesture
> - Use fingers to pan/zoom on tablet
>
> # Manual
> - Manually create frame (art-board)
> - Manually create ink-box (done), connectors, attachments, primitives (beautiful)
>
> # AI
> *(empty)*

Question to frame: *what is the next outcome wave after on-device connectors, and in what order, without opening iter-005 yet?*

### Definition of enough (checklist — tick at conclusion)

- [x] Human list captured verbatim and clustered into jobs (not solutions-as-REQs)
- [x] Conflicts with current PRD Non-Goals / BRD-07 called out
- [x] Proposed MoSCoW for **iter-005 v1 core** vs later (advisory)
- [x] AI bucket left empty until the human fills it
- [ ] Formal `[REQ-NN]` authored — **explicitly not this note**
- [ ] Campaign lock / track for iter-005 — after retro-gate

## Glossary

| Term | Definition |
|---|---|
| Pencil eraser | Hardware eraser end of the RM2 stylus — distinct from a software Eraser tool |
| Selection-erase | With a selection, a command (or eraser gesture) deletes selected nodes |
| Endpoint style | Visual/semantic marker at a connector end (star, empty/fill arrow, one, many, …) |
| Endpoint ink | Extra strokes drawn over a connector end that are *part of* that end, not ordinary ink |
| Mid-attachment | A node (text, figure, ink cluster) bound to a position along the connector spine; follows warp |
| Frame / artboard | A clip/layout container (REQ-08 already names Frame as a kind) |
| Manual create | Explicit tool/gesture that inserts a node without recognition |
| Table recognition | Best-effort convert of drawn grid/ink into a table node |

## Session history

### Round 1 — converge · technique: JTBD + MoSCoW parking (shape-opportunity)

Human asked for **notes**, not a diverge session. One capture round.

- **R1-I1** (pm): Outcome for the wave: *the tablet keeps feeling like paper* — erase, duplicate, decorate connectors, hang labels on lines, and (later) recognize tables — without leaving the page.
- **R1-I2** (pm): **Natural pen-on-paper** is the candidate **v1 core** for iter-005. Gesture + Manual are adjacent campaigns; AI is unspecified.
- **R1-I3** (pm): Eraser has two jobs: (a) hardware pencil eraser on ink, (b) selection → erase. Do not collapse them into one REQ without checking RM2 eraser button/event availability (architect).
- **R1-I4** (pm): Copy/cut/paste is clipboard + duplicate on-canvas. Device-only first (in-session paste). Cross-app paste is a later bet.
- **R1-I5** (pm): Connector endpoint styles were **explicitly out of REQ-09** (`srs-product` “Arrowheads, dash, double-line”). Two creation paths: context toolbar after create; **auto-recog** of inks that run over endpoints and **preserve** them as endpoint decoration through warp.
- **R1-I6** (pm): Mid-attachments ride the same warp as REQ-09 (BR-C06: warp is a function of rest + endpoints). New: parametric `t` (or arc-length) on the spine so labels/figures move with the line during drag.
- **R1-I7** (pm): Table recognition is a **new node kind** — likely after or with REQ-08 capability descriptor; high false-positive risk next to default-on recognizers (EXP-0002 ship gate still applies).
- **R1-I8** (pm): **Finger pan/zoom on tablet** contradicts current epaper Non-Goals and [BRD-07](../../../.docs/brd.md) (“Infini owns pan/zoom; on-device pan/zoom deferred”). Adopting this **changes session ownership**: tablet can change viewport, not only Infini. Needs a product decision + likely ADR (who publishes `viewport`).
- **R1-I9** (pm): Manual create: ink-box **done** (REQ-05). Remaining: frame/artboard; connectors; attachments; primitives that look **beautiful** (not raw ink stand-ins). This is closer to REQ-08 + a creation palette — conflicts with Non-Goal “no general tool palette.”
- **R1-I10** (pm): **AI** section empty — park; do not invent OCR/LLM scope.
- **R1-I11** (architect lens): ASRs — clipboard model; endpoint-ink membership vs ordinary ink; mid-attachment transform; device viewport vs Infini; table as structured node vs ink-group.
- **R1-I12** (sm lens): Do not slice stories until retro-gate. Carry-overs still parked: REQ-08, CHL-0011 nested enclose, CHL-0012 FREE_FORM.
- **Feedback gate**: paused — human can reorder MoSCoW or fill **AI** before PRD authoring.

## Research & sources

- [R1-I5] `.docs/modules/epaper/prd.md` REQ-09 “Not this REQ: … arrowheads …” — (fact)
- [R1-I5] `.docs/modules/epaper/features/connector-ink/srs-product.md` out of scope: Arrowheads, dash, double-line — (fact)
- [R1-I8] `.docs/modules/epaper/prd.md` Non-Goals: on-device pan/zoom deferred; REQ-10 is hit-test/move only — (fact)
- [R1-I8] `.docs/brd.md` BRD-07: Infini owns pan/zoom; “Out of this BRD (defer): on-device pan/zoom on Epaper” — (fact)
- [R1-I9] REQ-08 already lists Frame, Primitive, Connector re-anchor, Text — (fact)
- [R1-I7] EXP-0002 ≤2% FP ship gate — (fact)
- Analog: paper notebooks — eraser + arrows + labels on lines; whiteboards — tables. Not cited as requirements. (analog)

## Decision Log

| id | decision | status | supersedes | source |
|---|---|---|---|---|
| D1 | Capture iter-005 wave as [BS-0002]; **do not open iter-005**; **do not mint REQs** until retro-gate + `/pm` author-prd | decided | — | R1-I2, lifecycle |
| D2 | Candidate **v1 core** (Must this wave): eraser (hw + selection), copy/cut/paste, connector endpoint styles (toolbar + preserve endpoint ink), connector mid-attachments | decided | — | human list + PM rank |
| D3 | Finger pan/zoom = **Should / product-decision first** — currently a BRD/PRD Non-Goal | decided | — | R1-I8 |
| D4 | Table recognition = **Could** (new kind + FP risk) unless human promotes | decided | — | R1-I7 |
| D5 | Manual frame + beautiful primitives/connectors = **Should**, after or with REQ-08; ink-box manual already done | decided | — | R1-I9 |
| D6 | AI = **Won't** until the human writes the bucket | decided | — | R1-I10 |
| D7 | Nested enclose (CHL-0011) and FREE_FORM (CHL-0012) stay **parked unless human pulls them into this wave** | decided | — | existing lock |

## Assumptions & riskiest bets

| assumption | why critical | validation (EXP) | owner | by-when | status |
|---|---|---|---|---|---|
| RM2 eraser end is reachable from Epaper as a distinct input | Hardware eraser path dies if events aren't distinguishable | device spike | architect | before first eraser REQ | untested |
| Endpoint ink can be bound without stealing enclose/connector FP budget | Default-on recognizers already at ≤2% gate | corpus replay | qa | with endpoint-style SRS | untested |
| Mid-attachments can follow warp without rebaking rest shape (BR-C06) | Wrong model = snap/jump like CHL-0006 | fixture drag | architect | with attachment SRS | untested |
| Device pan/zoom can publish viewport without breaking REQ-02/REQ-07 one-way rules | Session contract change | product + ADR | pm + architect | before gesture REQ | untested |
| In-document clipboard is enough (no OS paste) | Scope explosion | human confirm | pm | PRD authoring | untested |

## Goal prioritisation & metric tree

**Frame:** MoSCoW for *iter-005 candidate wave* (advisory). RICE after REQs exist.

| Cluster | Opportunity (job) | MoSCoW (this wave) |
|---|---|---|
| Eraser | When I mark a mistake, I want it gone like pencil paper | **Must** |
| Copy/cut/paste | When a cluster is right, I want another copy without redrawing | **Must** |
| Connector endpoint styles | When a line means “flows to / many / note”, the end should say so | **Must** |
| Connector mid-attachments | When I label a relationship, the label stays on the line as boxes move | **Must** |
| Finger pan/zoom | When Infini isn't in reach, I still want to move around the page | **Should** — **blocked on BRD/PRD Non-Goal reversal** |
| Table recognition | When I draw a grid, I want it to behave as a table | **Could** |
| Manual frame | When I need an artboard, I place one on purpose | **Should** (REQ-08 adjacent) |
| Manual connector / attachment / primitive | When recognition misses, I still get a beautiful object | **Should** (palette vs Non-Goal) |
| AI | — | **Won't** (empty) |
| CHL-0011 / CHL-0012 | Nested boxes / wrap-content | **Won't** unless pulled in |

- **North Star (provisional):** fraction of a diagram session completed **without** reaching Infini for erase, duplicate, or connector decoration.
- **Input metrics:** erase commit p95; paste fidelity (geometry ±1 px); endpoint style applied without losing ink; mid-attachment follows warp (0 px jump); recognizer FP still ≤2%.
- **Guardrails:** REQ-01 ink latency (≤30 ms); no full-panel invalidation during connector drag; EXP-0002 FP bar not relaxed.

## Goal dependencies

```
REQ-09 warp (shipping) ──┬── endpoint styles (toolbar)
                         ├── endpoint-ink recognition (depends on connector node)
                         └── mid-attachments (parametric spine)
Selection (REQ-05/06) ──┬── selection-erase
                        └── copy/cut/paste
Hardware eraser ────────── independent spike
REQ-08 descriptor ──────── manual primitives / frame / any-node verbs
Device viewport ────────── finger pan/zoom (also Infini REQ-01/02, epaper REQ-02/07)
Table node ─────────────── likely REQ-08 + new recognizer (after FP corpus)
AI ─────────────────────── unspecified
```

## ASRs & quality drivers

| ASR / driver | why significant | provisional measure | pending ADR? |
|---|---|---|---|
| Viewport authority | Finger pan/zoom vs Infini-only viewport | who emits `viewport`; conflict if both | **yes** if gesture Must |
| Clipboard / duplicate ops | New structural ops on device undo ring | 1 undo = 1 paste or 1 cut | likely |
| Endpoint-ink membership | Ink that is “part of” an end vs free ink / enclose | bind at recog time; survive warp | likely |
| Mid-attachment parameter | `t` on rest spine vs world offset | follow warp ≥5 Hz, 0 jump | likely (extends ADR-0020) |
| Table as node vs ink group | Structured cells vs SmartGroup of lines | new kind vs CHL-0011 nesting | yes if Could promoted |
| Creation palette vs chip Non-Goal | Manual connectors/primitives need UI | context toolbar vs 4th tool | design + ADR |

## Conclusion

- **Answer / framing:** Next wave after connectors is **natural pen-on-paper** (erase, clipboard, connector ends + labels). Gesture pan/zoom and manual beautiful primitives are **explicit intents** but collide with current Non-Goals / Infini ownership — reverse those in product docs before slicing. **AI is blank.** Table recognition is later unless promoted.
- **Why conclude now:** Human asked to take notes, not to author PRD. Enough to park a sequenced opportunity map.
- **Definition-of-enough:** capture + conflicts + MoSCoW done; REQ minting and iter-005 lock remain open by design.
- **Rejected / parked:** inventing AI features; opening `.plan/iter-005/`; pulling CHL-0011/0012 without a human ask.

## Outputs / next actions

- [x] This file [BS-0002](./BS-0002-iter-005-feature-wave.md)
- [x] MASTER **Forward** pointer (thin)
- [x] Epaper PRD Open Questions pointer (no new REQ)
- [ ] After iter-004 retro-gate: `/pm` author-prd for Must cluster → `/architect` → `/sm` new iter
- [ ] Human: fill **AI** or confirm empty; confirm pan/zoom Non-Goal reversal; confirm table Could vs Must
- [ ] Architect spike (when wave starts): RM2 eraser events
