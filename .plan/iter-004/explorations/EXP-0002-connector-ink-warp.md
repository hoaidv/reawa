---
id: EXP-0002
goal: Prove hand-drawn connectors survive a node move (spine-relative warp) and that recognition guards reject ordinary writing — before the connector campaign locks
goal_source: chat 2026-08-14 (human, via /pm) — UX3 "system has to adjust hand-draw connector smoothly"; validation route chosen "EXP before campaign lock"
date: 2026-08-14
driven_by: pm
goal_from: human
initiative: [REQ-09]     # to be minted; connectors on device. Also guards REQ-03 amendment
status: proposed
mode: checkpoint
timebox: 3 hours per round
token_budget: 100k per round
sandbox: git worktree `exp/connector-ink-warp` (throwaway)
brainstorm: BS-0001
participants: [pm, architect, dev, designer, qa]
iter: iter-004
relates-to: [SRS-EP-07, SRS-EP-10, SRS-EP-11, SRS-IN-04, ADR-0010, ADR-0011, ADR-0015, ADR-0017, BS-0001]
---

# EXP-0002 — Connector-ink warp + recognition guards

## Goal (from human)

Verbatim (chat, 2026-08-14, UX3):

> System has to adjust hand-draw connector smoothly. […] But we wanna preserve natural drawing
> experience. Here's the hard path.

and, on sequencing:

> Run the warp EXP on the host harness first, then lock the campaign.

[BS-0001](../brainstorms/BS-0001-auto-connector-ink.md) concluded that both of the human's escape
hatches (EH1 bezier-fit, EH2 physics rope) are special cases of "spine plus offsets", and chose a
third: store the drawn body as a **rest shape in spine coordinates `(s, d)`** and warp it from the two
endpoints with a Hermite tangent blend at the ends (BS-0001 **D3**). Two bets from that session block
the campaign: the warp must **look right**, and the guards must not eat ordinary writing — the latter
became a **ship gate** when the human chose to ship both recognizers armed (BS-0001 **D22**, **D25**).

### What "achieved" means, in numbers

**Initiative 1 — warp (blocks campaign lock)**

| # | Criterion | Bar |
|---|---|---|
| W1 | **Human verdict** on a 20-case move set rendered as SVG | PASS (taste judgment — the only criterion that can fail for a non-numeric reason) |
| W2 | **Round-trip drift** — move a node through 20 successive transforms and back to the original pose | Final geometry equals the original within **±1 px @ 100% zoom**; validates the never-re-bake invariant (D5) |
| W3 | **Determinism** — same rest shape + same endpoints, run twice, and run in both orders | **Byte-identical** sample output; this is what lets the desktop mirror stay consistent (ASR-1) |
| W4 | **Endpoint facing** — departure tangent after the blend vs the required edge normal / centre ray | within **±5°** at both ends, in 100% of cases |
| W5 | **No new cusps** — curvature spikes introduced by the warp that were not in the rest shape | **0** in ≥95% of frames; any case that fails is named, not averaged away |
| W6 | **Cost** — re-warp of a 500-sample connector on the host | p95 **≤2 ms** (the device budget is ≤300 ms after commit, so this must be trivially met) |

**Initiative 2 — guards (blocks ship, per D25)**

| # | Criterion | Bar |
|---|---|---|
| G1 | **False positives** on an ordinary-writing corpus (text, underlines, ticks, brackets, strikethroughs, arrows drawn as decoration) with **both recognizers armed as shipped** | **≤2%** of `pen` strokes |
| G2 | **First-run exposure** — the first 20 strokes of a fresh page | **0** unintended creations, or a named, explained exception |
| G3 | **Recall** on intended connectors (single-stroke UX1 and chained UX2) | **≥90%** |
| G4 | **No regression** — replay the existing enclose / membership fixtures through the new closure-first fall-through pipeline (D20/D21) | **0** changed verdicts on cases that are not the deliberate D21 fall-through |

Loop runs until these are met, or the human stops it.

## Initiatives & approaches (PM)

### Initiative 1 — Does the spine-relative warp look like the creator's line after a move?

Minimal and falsifiable: build the warp on the **host** (no Qt, no device), drive it with scripted
endpoint changes, and dump **SVG** so a human can look at before/after without a deploy.

**Candidate approaches** (tournament — the trade-off is what the endpoint pair is allowed to do to the
drawn shape):

- **A. Similarity warp** — translate + rotate + **uniform** scale derived from the endpoint pair; `d` stored absolute (BS-0001 D8). Simplest; risk is that stretching one axis inflates the whole shape.
- **B. Anisotropic warp** — scale **along the chord** only, leave the normal offsets untouched. Should look better under pure stretch; risk is shear artefacts when the chord rotates.
- **C. A-or-B + Hermite tangent blend** over the first/last ~15% of arc length so each end leaves at its required `facing` (BS-0001 D16, `Bender`-style position+tangent Hermite).
- **D. Piecewise warp with interior pins** at high-curvature vertices — **only if** A–C fail the deliberate-detour case (BS-0001 D12). Do not build speculatively.

**Case set (20)** — 2 rest shapes (a smooth arc and a genuinely wiggly line) × these moves, plus the
specials: translate near · translate far · translate to the opposite side of the peer (chord flips) ·
rotate the node · resize the node (anchor `t` preserved) · both nodes move · near-zero endpoint
separation (degenerate) · a connector with a deliberate detour around a third box · a multi-stroke
chained connector (UX2) · a centre-bound end that must clip at the boundary (D17).

### Initiative 2 — Do the guards reject ordinary writing?

**Candidate approaches:**

- **A. Real corpus (preferred).** Capture a page of genuine handwriting + a genuine hand-drawn diagram off the RM2 through the plumbing that already exists (`RM_SYNC_HOST` stroke TCP `:9877`, or the Device Log channel `:9878` shipped by STORY-IN-029 / STORY-EP-021), and store it as a fixture beside `epaper/tests/fixtures/{enclose,ops}`.
- **B. Synthetic corpus (fallback).** Author strokes by hand if the tablet is unavailable — this has happened before (STORY-EP-014 QA HOLD: RM2 `10.11.99.1` unreachable). Weaker evidence; must be labelled as such in the round entry, and G1/G2 do not count as settled on synthetic ink alone.
- **C. Guard-ladder sweep.** Run the five-rung ladder (BS-0001 R2-I3) over the corpus and print a confusion table; tune only the thresholds that are not already fixed by shipped specs (`MIN_ENCLOSE_WORLD` = 48 world units and the ≥80% membership rule are **not** in scope to retune here).

## Priority check — architect + dev push back

- **Vital to the goal?** **Yes** — Initiative 1 gates the campaign lock by the human's own sequencing choice; Initiative 2 gates ship because recognizers ship armed. Neither is a nice-to-have.
- **Riskiest / most-uncertain assumption?** Initiative 1. It is a **taste** judgment that no amount of further specification can settle, and the entire REQ-09 deformation clause depends on it. It also needs no tablet, so it can start immediately — Initiative 2 may block on RM2 availability.
- **Timeboxable?** Yes. The warp is O(n) arithmetic with no solver; the repo already has the exact harness pattern (`epaper/tests/*.cpp` compiled standalone with `c++ -std=c++17 -I.`, no Qt, driven by `run_device_document_test.sh`). SVG output needs no image library. One round should produce the contact sheet.
- **Verdict**: **proceed** — Initiative 1 in round 1; Initiative 2 in round 2 (or in parallel if the RM2 capture can be done independently).

**Constraints on the sandbox**

- Worktree only. Probe sources may sit under `epaper/tests/` **in the worktree**, and are **discarded by default**.
- **0** changes to `epaper/document/`, `tabletcanvasitem.cpp`, `toolcanvasitem.cpp`, or anything under `infini/`. No spike code reaches a shipping path without being re-implemented docs-first as a story.
- Do **not** retune shipped thresholds (`MIN_ENCLOSE_WORLD` 48, ≥80% membership, handle 28/56 du, LOD 96 du).
- Do **not** amend `.docs/` from this EXP — findings route through PM (see Recommendation & routing).
- Op / envelope names stay the `SRS-IN-09` transmit set; the probe invents no wire aliases.

## Loop log

### Round 1 — approach A/B/C (warp) — *not started*

- **Experiment** (architect + dev, sandbox): _pending_ — build `warp_probe.cpp` (host, no Qt): rest-shape construction → `(s, d)` storage → endpoint-driven warp (A vs B, then + C) → SVG contact sheet of the 20 cases, plus W2/W3/W4/W5/W6 measured to stdout in the style of the existing host tests.
- **Assessment** (PM, vs goal): _pending_
- **Adversarial check** (second lens, not the builder): `/qa` re-runs the numeric bars and checks the SVG set independently; `/designer` judges naturalness separately from `/dev`'s "it works".
- **Feedback gate**: **pause** — W1 is a human taste verdict; the loop cannot self-certify it.
- **Adjust** (PM): _pending_

### Round 2 — approach A/C (guards) — *not started*

- **Experiment**: _pending_ — capture or author the corpus; run the guard ladder; print the confusion table for G1–G4.
- **Assessment**: _pending_
- **Adversarial check**: `/qa` owns the corpus labelling so the builder does not grade their own recognizer.
- **Feedback gate**: pause if G1/G2 miss — a miss means the **default-on** decision (D22) needs revisiting, which is the human's call, not a threshold tweak.
- **Adjust** (PM): _pending_

## Outcome

- **Result**: _pending_
- **Evidence**: _pending_

## Recommendation & routing

Pre-agreed routing so the result cannot become an orphan experiment:

| Result | Route |
|---|---|
| W1–W6 pass | `/architect` writes the connector-ink geometry **ADR** (BS-0001 ASR-1) with the warp as its canonical algorithm + shared fixtures; `/pm` mints **[REQ-09]** including the deformation clause; then the campaign lock flips |
| W1 fails on naturalness | Try approach **D** (interior pins) before abandoning; if D also fails, escalate — the fallback is a **routed** connector (drawn ink kept as decoration only), which is a different product promise and needs the human |
| W2 or W3 fails | Blocker, not a tuning issue — the mirror-consistency argument for one-way sync collapses (ASR-1). Stop and re-open BS-0001 |
| G1/G2 pass | Ship gate cleared for **default-on** (D22) |
| G1/G2 fail | `/pm` decision: revert to opt-in defaults, or tighten guards and re-run. Recorded as a supersession of D22, not a silent threshold change |
| G4 fails | `CHL-*` — the D21 fall-through is regressing EP-016 / EP-017; the pipeline order needs rework before any story |

Also feeds: `ADR-0010` §6 amendment (`facing`, `centre`), and the `/designer` ToolChip story
(3 tools + 2 recognizer toggles, BS-0001 ASR-7) which does **not** depend on this EXP's verdict.

## Code disposition

- [x] Discard sandbox worktree (default) — knowledge is captured here and in the ADR that follows
- [ ] Keep as reference spike — only if the SVG harness proves useful for later geometry work
- [ ] Promote to production via story(ies) — re-implemented docs-first against the geometry ADR, never copy-pasted
