---
id: EXP-0002
goal: Prove hand-drawn connectors survive a node move (spine-relative warp) and that recognition guards reject ordinary writing — before the connector campaign locks
goal_source: chat 2026-08-14 (human, via /pm) — UX3 "system has to adjust hand-draw connector smoothly"; validation route chosen "EXP before campaign lock"
date: 2026-08-14
driven_by: pm
goal_from: human
initiative: [REQ-09]     # to be minted; connectors on device. Also guards REQ-03 amendment
status: paused-for-feedback   # warp Initiative 1 closed by W1 → ADR-0020; Initiative 2 (guards) not started
mode: checkpoint
timebox: 3 hours per round
token_budget: 100k per round
sandbox: git worktree `exp/connector-ink-warp` (throwaway) — `~/.cursor/worktrees/exp0002-warp-a3f19c2d/reawa-6bcab1d3a018`, contact sheet at `out/index.html`
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
| W4 | **Endpoint facing** — departure tangent after the blend vs the required edge normal / centre ray. **Corrected in round 1:** the Hermite tangent magnitude is `T·L'` (arc length), **not** `T·|c'|` (chord); and the far-end departure direction is the **negated** incoming tangent, since the spine tangent at `s=1` points into the box | within **±5°** at both ends, in 100% of cases |
| W5 | **No new cusps** — curvature spikes introduced by the warp that were not in the rest shape | **0** in ≥95% of frames; any case that fails is named, not averaged away. **Round-1 defect:** a rest-relative threshold is blind on kinked rest shapes (a 27.3° joint in a chained connector lifts its own bar to 46°). Round 2 scores the blend region against an **absolute** bar and keeps the rest-relative bar only for the middle |
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

### Round 1 — approach A/B/C (warp) — **in flight** (started 2026-08-14)

- **Experiment** (architect + dev, sandbox): `warp_probe.cpp` + `run_warp_probe.sh` in an isolated worktree (host C++17, no Qt, no deps, no network): rest-shape construction → `(s, d)` storage → endpoint-driven warp, all four variants (A, B, A+C, B+C) rendered per case → SVG per case + `out/index.html` for the human's W1 verdict, with W2…W6 printed to stdout.
  - **Tournament, not a single approach** — A and B are rendered side by side per case so the uniform-vs-chord-only trade-off is judged on evidence rather than argued. **D (interior pins) deliberately not built** this round.
  - **Added to the brief:** a deliberately wrong **re-bake mode** is measured alongside W2, so the never-re-bake invariant (BS-0001 D5) is backed by a number instead of an assertion.
  - Fenced: 0 changes to `epaper/document/`, `tabletcanvasitem.cpp`, `toolcanvasitem.cpp`, `CMakeLists.txt`, `infini/`, `.docs/`, `.plan/`.
- **Result** (host probe, 20 cases × 4 variants, `epaper/tests/run_warp_probe.sh`):

| # | Measured | Bar | Verdict |
|---|---|---|---|
| W2 never-re-bake | **0.000000000 u** (0 ulp), all 8 shape×variant combos | ±1 u | **PASS — exact, not merely in tolerance** |
| W2 **re-bake counter-test** | 1.80 u after 20 moves; 5.17 u after 200 | ±1 u | **fails as intended** — see mechanism below |
| W3 same input twice | byte-identical (`memcmp` on doubles) | byte-identical | **PASS** |
| W3 case set in reverse order | byte-identical per case | byte-identical | **PASS** |
| W4 facing, A / B (no blend) | 160.6° / 170.1°, 20/20 over | ±5° | fails **by design** — this is what C exists for |
| W4 facing, A+C **as briefed** (`T·|c'|`) | 5.42° max, 3/20 over | ±5° | FAIL, marginal |
| W4 facing, A+C **arc-matched** (`T·L'`) | **4.41° max, 0/20 over** | ±5° | **PASS** (brief was wrong — see below) |
| W4 facing, B+C arc-matched | 6.60° max, 1/20 over | ±5° | FAIL |
| W5 zero-new-cusp share (per-vertex turn) | A 20/20 · B 18/20 · **A+C 14/20** · B+C 11/20 | ≥95% | **FAIL for every blended variant** |
| W5 zero-new-cusp share (turn per unit arc) | A 18/20 · B 18/20 · A+C 12/20 · B+C 12/20 | ≥95% | **FAIL for all four** under the sampling-independent reading |
| W6 cost | p50 5.5–6.1 µs · **p95 6.3–6.9 µs** (500 samples) | p95 ≤2000 µs | **PASS — ~290× margin** |
| Variant separation | A vs B is **≥6.35 u in 20/20 cases** (max 82.95 u) | — | A and B are never visually equivalent; the choice is real |

- **Assessment** (PM, vs goal): **PARTIAL — the warp model is validated; the anchor model is not.**
  - **The architectural claim holds.** W2 exact-zero plus W3 byte-identical in both orders is as strong as a host probe can make the pure-function argument (ASR-1): two peers can recompute this independently with 0 divergence, and no geometry needs to cross the wire (D4).
  - **D5 earned its place, and the mechanism is now known.** Re-baking is nearly harmless for the unblended variants (0.04 u after 200 moves — a similarity warp followed by resample-and-smooth is close to idempotent) and **destructive for the blended ones** (1.80 u after 20 moves), because each re-bake freezes the *forced* end tangents into the rest shape. So never-re-bake is load-bearing precisely for the variant we intend to ship, not a general hygiene rule.
  - **W6 relocates a constraint.** At ~7 µs, re-warp cost is irrelevant to the frame budget, so "warp at commit, not per frame" (ASR-6) is justified **only** by e-ink panel refresh (`BR-N06`), not by CPU. If the panel can take it, a live warped preview during the drag is back on the table as a product option.
  - **W4's failure was my brief's error, not the model's.** I specified the Hermite tangent magnitude against the **chord** (`T·|c'|`); it must be against **arc length** (`T·L'`), because the blend spans a fraction of arc length. Where `L' ≫ |c'|` the tangent was far too short to steer. Corrected, A+C clears the bar outright. The far-end measurement convention also needed fixing: the spine tangent at `s=1` points *into* the box, so the departure direction is the negated incoming tangent — as briefed it was unsatisfiable.
  - **W5 is the real open item, and its failures have one dominant root cause.** Of A+C's 6 failing cases, five (`chord-flip`, `rotate-a`, `translate-far`, `both-move`, `detour-third-box`) spike at `s≈0.01`–`0.99`, i.e. **inside the blend region**, because the required facing is far from the drawn tangent and the blend has to hairpin. And the reason the facing is far from the drawn tangent is that **an anchor stores `(edge, t)` and re-resolves its position but never re-selects its edge** — so when A rotates, or moves past its peer, the connector is forced to leave the box on the wrong side and turn around, crossing its own target. `arc/detour-third-box` is the separate, already-predicted case (D12 interior pins). **Anchor edge re-selection is an unspecified product decision, and it is now the highest-value thing to settle before the campaign locks.**
  - **W5's metric is also partly at fault.** `arc/chain-3-stroke` carries a 27.3° turn at a stroke joint in the *rest* shape, which lifts its own threshold to 46°, so every variant scores 0 new cusps even where the joint visibly sharpens. The metric is blind on exactly the rest shapes most likely to have kinks — chained UX2 connectors. Round 2 must score the blend region against an absolute bar, not a rest-relative one.
- **Adversarial check** (second lens, not the builder): PM re-ran `run_warp_probe.sh` independently in the worktree — W2, W3, W4 (both magnitudes), W5 (both metrics) and W6 reproduce exactly. The `/designer` naturalness lens and the human W1 verdict are **outstanding**.
- **Feedback gate**: **PAUSED** — two things only the human can settle: **W1** naturalness (A vs A+C on the contact sheet) and the **anchor edge re-selection** policy, which changes what the creator's drawn attachment means and is therefore product, not geometry.
- **Human decisions on the pause gate (2026-08-14):**
  - **Anchor edge policy — NEVER re-select. The drawn attachment is sacred.** *"When user wanna avoid U-turn and self-crossing, they just need to connect center-2-center, this automatically remove the fixed edge/face of the anchor."*
  - **Round 2 = all five adjustments**, plus a new parameter: *"when the connector goes out of a node perpendicularly to a chosen face, I want the out-going tangent to be higher → this smooths-out the connector at its ends."*
- **PM reading of the edge policy (D26 below):** this is not "accept the U-turn" — it makes the **two anchor kinds semantically different**, and gives centre-binding a job rather than an aesthetic:
  - **Edge anchor** — stores `(edge, t)`; facing is **fixed** to that edge's outward normal; sacred; *may* U-turn if the creator moves the box past its peer. That is the creator's choice and the creator's problem.
  - **Centre anchor** — stores no edge; facing is **derived** as the ray toward the peer, so it **can never oppose the chord**; clipped at the boundary (D17).
  So the U-turn is not a defect to be engineered away — it is the honest consequence of a sacred attachment, and the remedy is a creator action (switch that end to centre), not a silent system correction.
- **Consequence for W5:** `chord-flip` and `rotate-a` edge-bind U-turns become **expected behaviour**, not failures. Round 2 must partition the case set by whether the stored facing still faces the peer, apply the W5 bar to the **aligned** class only, and add **W7** below to prove the escape hatch actually works.
- **New criterion W7 — centre-bind escape hatch:** for every case whose edge facing opposes the new chord, re-running that end as a **centre** bind must yield **0** new cusps and pass W4. If the escape hatch does not clean up the geometry, the policy has no remedy behind it.
- **Affordance gap this policy creates (for `/pm` at PRD time, not an experiment):** the remedy is a creator action, so *"switch this end to centre"* must be **reachable on the device in v1** — otherwise the escape hatch is theoretical. Candidates: decided at draw time by where the pen lands (near the edge → edge, well inside → centre), an end-handle toggle on the selected connector, or both. BS-0001 D16 said "centre available" without naming a mechanism; it now needs one.
- **Adjust** (PM) — round 2 scope, **approved by the human**:
  1. **Do not implement edge re-selection.** Instead partition aligned vs reversed cases, and add the centre-bind run of every reversed case (W7).
  2. **Degenerate clamp** — BS-0001 R3-I11 asked for one and the brief deliberately omitted it so the failure would be measurable. It is: at scale 0.009 the spine collapses to a ~3 u stub while `d` stays absolute (D8), leaving a ~140 u spike across a 3 u chord. Try scale-floor, `d`-taper-under-compression, and both; report which fixes it without touching normal cases.
  3. **Blend length `T` sweep** — fixed at 0.15 in round 1; sweep it, since the cusps sit inside the blend region.
  4. **Departure stub — the human's new parameter.** In round 1 the Hermite end-handle length was tied to the blend length as `k·T·L'` with `k = 1`. "Out-going tangent higher" means a **larger end-handle**, which makes the connector leave the face straighter for longer and round off the transition. Round 2 sweeps it **independently of `T`**, and additionally tests an **absolute** parameterization in world units (capped as a fraction of `L'` so short connectors cannot explode) — because "the line leaves the box straight for ~12 world units" is better product vocabulary and more stable than a dimensionless multiplier. Must also detect **overshoot / self-intersection** in the blend region, which is the failure mode of too large a handle.
  5. **Fix the W5 metric** — absolute bar in the blend region; keep the rest-relative bar only for the middle.
  6. Adopt `T·L'` as the canonical tangent-magnitude reference and the outward-departure convention for the far end.

### Round 2 — warp refinement (aligned/reversed partition · clamp · `T` sweep · departure stub · fixed W5 metric) — **in flight** (started 2026-08-14)

- **Experiment**: extended the round-1 probe in the same worktree. Approved scope was the six adjustments in Round 1's *Adjust* block. **Approach D (interior pins) deliberately not built** — the human chose "all five", not "+ pins", so `detour-third-box` remains a known limitation under BS-0001 D12. Stdout archived to `out/report.txt`.
- **Result** — recommended defaults `T = 0.15`, departure stub `k = 1.5 × T·L'` on the **outer handle only**, samples parameterized on the **pre-blend** spine, **no clamp**:

| # | Measured | Verdict |
|---|---|---|
| **W7** centre-bind escape hatch | Both reversed cases (`arc/`, `wiggle/chord-flip`, obliquity 170.6°) convert to **0 new cusps**, facing 1.6° / 0.2° | **PASS — the human's policy has a working remedy** |
| W5 aligned-only, new metric | A 18/18 · **A+C 17/18 (94%)** · B 16/18 · B+C 15/18 | **1 case short** — `arc/rotate-a` |
| W5 aligned-only, old metric | A 18/18 · A+C 14/18 · B 16/18 · B+C 14/18 | metric change audited both ways |
| Partition | aligned **18/20**, reversed **2/20**; `rotate-a` at 68.6° obliquity is classed **aligned**, so the partition excuses only genuine U-turns | sound |
| W2 never-re-bake | still **0.000000000 u** | **PASS** |
| W2 re-bake counter-test | **39–45 u** after 20 moves (round 1: 1.8 u) | D5 far more load-bearing than round 1 showed |
| W3 determinism | byte-identical both orders | **PASS** |
| W6 cost | p95 ~5–6 µs | **PASS** |
| Fidelity at defaults | middle-70% deviation **0.02 u**; blend-region mean 3.2–16.7 u (~1–2% of length) | the measured price of the human's "higher outgoing tangent" |

- **Assessment** (PM, vs goal): **PARTIAL — one failure remains, it is now fully diagnosed, and three of my own spec errors were corrected by measurement.**
  - **W7 is the headline: the "sacred attachment" policy (D26) is safe to ship.** Converting a reversed end to a centre bind cleans up completely, so the creator's remedy is real, not theoretical. Two caveats that the pass/fail hides, both product-level: the conversion **clips ~48% of the drawn ink** inside the boxes (worst case — `chord-flip` passes through *both* boxes; converting one end in ordinary use hides far less), and for a centre bind the briefed `s=0` facing measurement samples a point **inside the box** that the creator cannot see. Measured at the boundary crossing the departure is 26–32° off the centre ray — no cusp, no U-turn, but not ±5°. **The ±5° bar does not apply to centre binds**; their requirement is "no U-turn, no cusp", which is exactly what W7 measures.
  - **`arc/rotate-a` is not a tuning gap and the mechanism is visible.** At `L' = 404.8 u`, `T = 0.15` gives a blend span of ~60.7 u while `k = 1.5` gives a handle of **92.8 u** — the handle is **longer than the arc it has to fit inside**, so with a 68.6° turn to absorb the Hermite must overshoot and hook back (6.3 u min radius against a 12 u bar, plus 1 overshoot). That is why it fails at **all 16** `(T, k)` grid points: every point in the grid has `k ≥ 1.2`, i.e. handle ≥ span. **The governing quantity is handle-length ÷ blend-span, and it must fall as the required turn rises** — a fixed `T` gives a 1° correction and a 69° correction the same arc budget, which cannot be right.
  - **Obliquity is the clean predictor.** Every aligned case at obliquity ≤ 37.6° passes; the only case above it (68.6°) fails. That is a threshold worth *characterizing* rather than a case worth tuning.
  - **My spec errors, corrected by measurement (third round running).** (1) The clamp task was built on my own round-1 mis-attribution — max `|d|` is 1.1 u, so `d` cannot produce the ~140 u spike; it belongs to **variant B's** unscaled perpendicular spine extent, which none of the three briefed policies can reach. (2) My "absolute world-unit stub" prior is **wrong for a structural reason**: the handle fills a span that is itself `T·L'`, so only their *ratio* controls shape — an absolute stub scores 5/18 at 12 u versus 17/18 relative, and making the span absolute too still tops out at 8/18. The honest product vocabulary is therefore *"the line leaves straight for about a fifth of its own length"*, not *"for 12 world units"*. (3) The middle-70% fidelity check was measuring a **defect, not a property**: blending changes total arc length, so re-placing samples by post-blend arc fraction shifted ink along the whole connector (15.28 u). Parameterizing on the pre-blend spine fixes it to 0.02 u with identical cusp/facing numbers — **this is an algorithm correction and belongs in the ADR**, because "the blend is local" is the entire selling point.
  - **Variant B should be dropped outright.** It has not won a single criterion across two rounds (round 1 cusps 18/20 vs A 20/20; worse facing; the degenerate spike is B's alone), and the *only* reason a clamp policy exists is to rescue it. Dropping B makes "no clamp" unconditional and removes a whole clause from the spec.
  - **Two bar-definition defects to fix**, both the builder's fair criticism: "≥95%" on an 18-case aligned set can only mean 18/18, so it must be restated as an absolute count with named exceptions; and the overshoot test must inherit the aligned-only partition, since a U-turn *is* a backtrack.
- **Adversarial check** (second lens, not the builder): PM recompiled and re-ran the probe independently — every substantive table is **byte-identical** to `out/report.txt`; the only differences are W6 microsecond jitter (p95 4.96 vs 5.04 µs), which is itself a small corroboration of W3. The `arc/rotate-a` render was inspected directly and the handle-vs-span arithmetic above read off it.
- **Feedback gate**: **PAUSED** — W1 naturalness is judgeable now on 17 of 18 cases, and two product calls are open (below).
- **Adjust** (PM) — proposed round 3, pending the human:
  1. **Characterize instead of tune.** Sweep box-A rotation 0°→180° in ~15° steps and plot min blend radius against obliquity, for a grid of handle÷span ratios. Output a **rule**, not a pass — e.g. `blendArc = T·L' · (1 + α·turn)`, or equivalently cap `k` as a function of turn — such that min radius ≥ 12 u across the whole obliquity range.
  2. **Drop variant B** and with it the clamp question entirely.
  3. **Add high-obliquity cases** so the 68.6° result stops being a single data point.
  4. **Restate the W5 bar** as an absolute count plus an obliquity condition; overshoot inherits the aligned-only partition.
  5. Adopt as canonical: pre-blend sample parameterization, relative stub, outer handle only.

### Round 3 — adaptive blend + characterization (final warp round) — **in flight** (started 2026-08-14)

**Human decisions closing round 2's gate (2026-08-14):**

- **Adaptive blend span — approved.** Grow the blend with the required turn, accepting that a harder turn spends more of the drawn line on the transition, *"because that is what a person redrawing it would do"*.
- **Centre conversion keeps the full rest shape.** The hidden portion is the price of correct re-clipping on later moves/resizes. Confirms D5 over tidiness.
- **Round 3 = full scope** (all five adjustments).

- **Experiment**: same worktree. (1) Characterize min blend radius against obliquity by sweeping box-A rotation 0°→180°, across handle÷span ratios, and **derive the adaptive rule** (`blendArc = T·L'·(1 + α·turn)` or a better-fitting form) that holds the 12 u radius bar at every angle. (2) Quantify what the adaptation **costs** — share of arc inside the blend, and deviation there — at each obliquity, since drawn character is the thing being spent. (3) Cap the blend per end so the untouched middle never vanishes, and report whether the extreme degrades smoothly or falls off a cliff. (4) **Drop variant B** and the clamp sweep with it; keep unblended A as the control. (5) Restate W5 as an absolute count with an obliquity condition, overshoot inherits the aligned-only partition, and centre binds are measured at the **boundary crossing** with no facing bar. (6) Canonical: pre-blend parameterization, relative stub, outer handle only, no clamp, A+C only. (7) Final contact sheet at the final defaults for the W1 verdict. (8) **New deliverable — the canonical algorithm summary**, implementation-ready for the architect to lift into the geometry ADR, plus proposed product-vocabulary **names** for the tunable parameters (blend length, departure stub ratio, adaptive factor, radius bar).
- **Result** — all restated bars pass; the adaptive rule works but **not in the form I proposed**:

| # | Measured | Verdict |
|---|---|---|
| Adaptive rule (canonical) | `blendArc_per_end = min( max(0.15·L', 7.0 · 12u · turn_rad), 0.40·L' )` | holds min radius ≥12 u across the whole aligned obliquity range |
| `arc/rotate-a` (round-2 failure) | 14.9 u radius, **0 cusps** | **fixed** |
| W4 facing ±5°, **edge binds only** | max 2.64° | **PASS** |
| W5 zero new cusps, aligned, `L' ≥ 24 u` | 0 of 16 failing (obliquity to 68.6°, turn to 109.1°) | **PASS** |
| W5b zero backtrack where facing faces the chord | 0 of 17 | **PASS** |
| W7 centre-bind remedy | 0 cusps, clipped and unclipped | **PASS** |
| W2 never-re-bake / re-bake | **0.000000000 u** / 48–60 u | **PASS**; D5 evidence stronger again |
| W3 determinism | byte-identical, both directions | **PASS** |
| W6 cost | 5.5 µs p50, <8 µs p95 | **PASS** |
| Adaptation cost | blend share 30–32% (low turn) → 52% (high); deviation in blend 3.9 u → 14.9–17.7 u; untouched middle ≤0.01 u throughout | the measured price of the human's adaptive-blend choice |
| **Identity check (not in the brief)** | unblended warp **is** the identity (0.000 / 0.072 u) — but **A+C deviates 7.69 / 7.82 u with nothing moved at all** | **the finding of this round** |

- **Assessment** (PM, vs goal): **the warp track is technically done, and it has surfaced one product problem that must be settled before the ADR.**
  - **My functional form was wrong, for a reason that is a geometric fact rather than a tuning preference.** `blendArc = T·L'·(1 + α·turn)` makes the blend a *fraction* of the connector; the demand is **absolute**, because turning through `turn` at radius `R` costs `R·turn` of arc regardless of how long the line is. Asked at five connector lengths, the multiplier needed swings 4–6.5× while the arc needed holds within 10%. No single `α` exists. This is the fourth of my premises the probe has corrected, and it is the one that most needed correcting, because a constant with the wrong units would have gone straight into the ADR.
  - **My "handle ÷ span must fall" diagnosis was half wrong, and the wrong half mattered.** Growing the span at a fixed ratio grows the handle in lockstep, so nothing falls; and above ~108° of turn a 5×5 grid of span × ratio overshoots in **every cell**. The backtrack is not caused by the handle — it is caused by the facing pointing away from where the line must arrive. Ratio was never going to be the fix.
  - **The real finding: the model already alters the creator's ink when nothing has moved.** The unblended warp is a true identity (0.000 u), so the *blend* is what breaks it — by 7.7–7.8 u — because an edge anchor's facing is defined as the **edge normal** while a hand-drawn line leaves a face **36–46° off** that normal. Every blend on the contact sheet is therefore absorbing a turn that the creator never introduced, and the W1 naturalness verdict would be passed on a line that was altered at recognition time. This is a **facing-definition** problem, not a constant.
  - **It also inflates two of the three stated limitations.** The "short connectors below ~150 u cannot meet the bar" and "backtrack forced above ~90° departure-chord angle" limits both start from a 36–46° baseline turn that exists only because of the normal-facing definition. Removing that baseline should shrink both, and shrink the adaptation cost table with it.
  - **The trade the human has to settle is between two things they have both asked for**: *"preserve natural drawing experience"* (BS-0001 topic) and *"make the rope attach to A and C naturally — perpendicular with face/edge or ray-through centre"* (BS-0001 EH2 properties). Measurement now shows these are in direct conflict at the attachment point: perpendicular attachment costs 7.8 u of drawn fidelity from the moment of recognition. Supersedes D16's "perpendicular leave" either way — it needs restating with the cost known.
- **Adversarial check**: PM recompiled and re-ran independently — every substantive table **byte-identical** to `out/report.txt`; only W6 µs jitter differs (third round running, which is itself corroboration of I2). `CANONICAL-ALGORITHM.md` reviewed: implementation-ready, five tunables, and it names its own largest defect in §7.1 rather than burying it.
- **Feedback gate**: **PAUSED** — the facing definition is a product call, and the W1 verdict should not be spent on a render that has a 7.8 u baseline error in it.
- **Adjust** (PM) — proposed round 4 (final warp round), pending the human:
  1. Implement the chosen facing definition; add **invariant I6 — identity**: with nothing moved, output is byte-identical to the drawn ink (no blend fires when there is no turn to absorb). This is a property QA can test trivially and it is what makes the document trustworthy.
  2. Re-derive `turnRoomFactor` and re-measure the whole case set against the new baseline.
  3. Re-decide `blendCap` (0.40 vs 0.35 — the builder flags that 0.40 leaves only 20% untouched and 0.35 costs almost nothing) **after** the facing change, since it moves the distribution.
  4. Re-check whether the short-connector and forced-backtrack limitations survive.
  5. Update `CANONICAL-ALGORITHM.md` and regenerate the contact sheet for the W1 verdict.

### Round 4 — approach A+C, drawn-departure facing (Initiative 1, final)

- **Change**: implement D26/D27 — edge facing becomes the **drawn departure** carried rigidly in the edge frame; make identity at rest exact; re-derive constants on the new baseline; re-check the two limitations; re-decide the cap.
- **Result**:

| # | Measured | Verdict |
|---|---|---|
| **I6 identity at rest** | **0.000000000 u**, bitwise, both rest shapes, no float tolerance needed | **PASS** — achieved by **deleting `blendLength`**, not by a short circuit or deadband |
| Identity is continuous, not a cliff | 0.000 u at 1° rotation, 0.043 u at 2°, 0.31 u at 4° (deadband alternative jumps **4.1 u** at its threshold) | the reason the no-floor form was chosen |
| `turnRoomFactor` | **7.0 → 5.0**; it is a **window (4.5–6.5), not a floor** — round 3's 7.0 now **fails** (overshoots: the window grows long enough to swallow the creator's own curvature) | constant *and* re-tune method are ADR clauses |
| `blendCap` | **0.40 → 0.35**; binds 15% of aligned ends, **11% of healthy ends**; costs one extra healthy end out of 140 vs 0.40, buys 10 more points of guaranteed untouched middle | accepted on the frequency data |
| Tunables | **four, not five** (`blendLength` deleted) | simpler than round 3 |
| Adaptation cost | **6.5% of arc at 15° rotation → 23% at 75°** (was 30–52%); deviation in blend 0.8 → 10.8 u; leakage into middle ≤0.06 u | **roughly halved** by the facing change |
| Short-connector floor | **~150 u → ≈50 u** ordinary, ≈120 u comfortable at high obliquity; identity holds at *every* length incl. 21 u | limitation largely evaporated |
| Forced backtrack >~90° | survives unchanged; fails at every `turnRoom` 2.0–8.0 and every cell of a 5×5 arc × stub grid | **confirmed geometry, not tuning**; remedy is the centre bind (W7 PASS) |
| W2 / W3 / W5 / W5b / W6 / W7 | 0.000000000 u · byte-identical incl. all 22 SVGs · 0 of 18 · PASS · 5.5 µs p50 · PASS | **PASS** |
| **W4 flat ±5° facing** | 4 of 20 exceed, worst 5.68° — but analytic deviation is **exactly zero** by construction; the number is a **secant artifact** of sample spacing | **the bar is wrong, not the model** |

- **Assessment** (PM, vs goal): **Initiative 1 is done. The model is simpler, twice as faithful, and its remaining limits are geometric rather than tunable.** Three things are worth carrying forward beyond the numbers:
  - **Deleting the base blend was the right answer and I did not think of it.** I offered three ways to special-case the identity; the builder removed the thing that broke it instead, making identity a *consequence of the sizing rule* rather than a case handled beside it. The deadband alternative would have shipped a 4.1 u pop on the smallest drag a creator can perform. This is the difference between a spec that handles a case and a spec that does not have the case.
  - **The drawn-departure decision paid for itself twice over.** It was taken to protect fidelity, and it also halved the adaptation cost, cut the short-connector floor by two thirds, and removed a tunable. The 7.8 u at rest was not a blemish on the model — it was mis-specified geometry generating work everywhere downstream.
  - **`turnRoomFactor` being a window, not a floor, is a trap for whoever re-tunes it.** Round 3's 7.0 was derived as an envelope of per-point minima — structurally the wrong estimator, and it now fails. The ADR must record **how** to re-tune (sweep the constant, count new cusps over the population) or the wrong method will silently pick a failing value again.
- **PM rulings on the two items the builder escalated** (both accepted, one amended):

| Item | Ruling |
|---|---|
| **D27 unachievable as worded** — the `(s, d)` store loses **0.0717 u** on a wiggly line before any blend runs (variant A, which never blends, sits the same distance out) | **D27 amended**: identity is byte-identical to the **rest-shape reconstruction**, not the raw ink. The 0.07 u is *representation fidelity*, documented as such — it is an order of magnitude below ink width. Closing it means storing a tangential component per sample: a larger body store and a changed warp for no visible gain. **Named as a v2 representation path, not taken now.** The builder was right to refuse the phrasing — it would have failed its own first unit test |
| **Centre-anchor facing never specified** in my brief — it derives from the peer ray, sits 8–23° off the ink at rest, and moves the ink up to 2.5 u with nothing moved | **Adopted as the builder measured it (D29): drawn departure clamped to a 60° cone about the peer ray.** Restores the identity exactly and still rescues every U-turn (0 new cusps, 0 overshoot, ink re-enters the box 0 times, same 51–54% of ink kept). A 90° cone overshoots 12–14 times, so the cone angle is load-bearing, not cosmetic. My brief specified the edge anchor and forgot the kind that exists to rescue it |
| **W4 flat ±5° secant bar** is mathematically incompatible with the 12 u radius bar — a curve satisfying one must violate the other | **Flat ±5° retired.** The production test is **analytic**: assert the constructed departure derivative equals the facing (exactly zero by construction). Any sampled check must be radius-implied, `max(5°, 28.6·baseline/R)` — zero failures there. Recorded so the production test author does not chase a non-bug |

- **Adversarial check**: PM recompiled and re-ran independently — fourth consecutive round with every substantive table byte-identical; only the µs cost row jitters.
- **Feedback gate**: **W1 naturalness verdict** — the one criterion only the human can answer. Contact sheet rebuilt around the "your ink, moved" claim: drawn ink in grey, the same ink carried rigidly as a pale blue halo, shipped output thin in red, and a sand band marking the only stretch the blend may touch. Red inside halo = untouched.
- **Adjust**: **superseded.** Before the W1 verdict, the human identified a deeper conflict than G1 vs G2: preserving the drawn body in the middle while absorbing new facings at the ends *cannot* look globally smooth — a 12 u blend will always sit next to a 400 u drawn curve. They directed a three-way tournament: local blend vs always-cubic vs rest→cubic morph. Approach D still deferred.

### Round 5 — spine-model tournament: local blend vs always-cubic vs rest→cubic morph (Initiative 1)

- **Change**: the human restated the deformation job. Local end-blending (D3) keeps drawn character but cannot absorb a new facing without a kink, because the middle is sacred. Global smoothness wants the body to **straighten toward a cubic bezier** that already satisfies both facings (original EH1). Identity at rest (D27) forbids always-clamping. The candidate that could keep both goals in proportion is a **morph**: rest spine at zero turn, cubic at hard turn, mix in between. `d` rides along. Quadratic bezier is **not** a candidate — one interior control point cannot represent two independent facings.
- **Experiment**: same worktree. Three overlays on the same case set. Characterize the morph mix `m(turn)` rather than tuning it. Contact sheet is the W1 artifact.
- **Result**: **W1 call, not a geometry win.** Local stays the shipped default. Always-cubic is globally smooth and forbidden by D27. Morph cannot keep both “small nudge looks like ink” and “hard turn looks like a cubic” with one mix factor.

| Bar | Local (R4 control, not retuned) | Always-cubic (EH1) | Morph (versine, sat 90°) |
|---|---|---|---|
| I6 identity vs U | **PASS bitwise** | **FAIL** — arc 19.2 max / 10.4 mean, 85% of samples >1 u; wiggle 25.6 / 11.3, 89% | **PASS bitwise** (`m=0` is a true skip) |
| W5 new cusps, aligned, `L'≥24` | 0/18 PASS | 0/18 PASS | 0/18 PASS |
| W5b backtrack, facing faces chord | 0/18 PASS | 0/18 PASS | 0/18 PASS |
| D30 analytic facing | 0 when a blend fires | **0 by construction** | inherits U’s mismatch at low m (17° at 15°); ~0 at high m — **buys facing by spending ink** |
| W2 / W3 / W6 / W7 | PASS / PASS / 7.7 µs p95 / PASS | PASS / PASS / 7.0 / PASS | PASS / PASS / 6.3 / PASS |

Ink spent vs unblended U (mean u / fraction of samples moved >1 u):

| Pose | turn | m | Local | Cubic | Morph |
|---|---|---|---|---|---|
| arc 15° | 19° | 0.11 | **0.05 / 3%** | 19.2 / 89% | 2.03 / **71%** |
| arc 45° | 55° | 0.67 | **0.72 / 10%** | 39.5 / 97% | 26.4 / 95% |
| arc 75° | 87° | 1.00 | **2.21 / 15%** | 59.8 / 98% | 59.6 / 98% |
| min radius 15/45/75 | | | 20.6 / 18.6 / 13.5 | 188 / 108 / 79 | 165 / 165 / 79 |

`m(turn)` proposed for the sheet, **not as a win**: `m = 0.5 · (1 − cos(π · min(1, turn/90°)))` with `turn = max(turn0, turn1)`. Continuous at 0 (`m'(0)=0`; 0.001 u at 0.5°, 0.022 u at 2°). Cubic handle: **rest-spine end-speed `L'`**, not chord/3 (ambiguous in the brief; as Hermite speed, chord/3 cusps and under-steers). Mix: world-space lerp.

- **Assessment** (PM, vs goal): **the conflict you named is now a measurement, not a hunch, and a mix function cannot dissolve it.**
  - **The poison is the rest-fit, not the mix.** Always-cubic is already ~10 u mean / ~19 u max from the rest spine *with nothing moved*. Morph is a lerp toward that curve. At 15° of rotation, C is ~19 u mean away; even the slowest-starting form (versine, `m=0.11`) has already moved **71%** of the line more than 1 u. A form slow enough to keep ink at 15° (`m ~ 0.002`) is still `m ~ 0.04` at 75° and is not a cubic. There is no constant that is Local at a nudge and Cubic at a hard turn, because Cubic is far from Local **before anyone moves**.
  - **That rest-fit is EH1's real price, and I under-specified it.** I treated the 10 u residual as a side note on Always-cubic. It is why Morph cannot work. Round 3's 7.8 u was a facing-definition error we could delete; this 10 u is the cubic not being the line you drew. Deleting a parameter will not remove it.
  - **A cubic does buy two things Local cannot.** (1) A hard-but-aligned turn (~90–105° of box rotation) S-curves with 0 backtrack and min radius 44–59 u; Local backtracks and drops below 12 u. The centre bind is no longer the only remedy *if* the product accepts redrawing as a cubic. Past ~120°, and on `chord-flip`, the cubic loops — centre bind still required for a genuine U-turn. (2) Short connectors: at 70 u / 45° Local has 4 cusps / 8.2 u radius, Cubic 0 cusps / 17.5 u. Floor lowered, not removed (30 u still fails both).
  - **I do not pick a shipped model.** D27 forbids Always-cubic. Morph only matches Cubic's smoothness after it has already spent the ink. Local remains the algorithm in `CANONICAL-ALGORITHM.md`. The sheet has all three; W1 is which *look* you want under a hard turn, knowing the cost.
- **Adversarial check**: PM recompiled and re-ran independently — fifth consecutive round, every substantive table byte-identical; only the µs cost row jitters.
- **Feedback gate**: **PAUSED for W1.** Three product-coherent choices, not a parameter to retune.
- **Adjust**: _pending the human_

### Round 6 — approach A/C (guards, Initiative 2) — *not started*

- **Experiment**: _pending_ — capture or author the corpus; run the guard ladder; print the confusion table for G1–G4.
- **Assessment**: _pending_
- **Adversarial check**: `/qa` owns the corpus labelling so the builder does not grade their own recognizer.
- **Feedback gate**: pause if G1/G2 miss — a miss means the **default-on** decision (D22) needs revisiting, which is the human's call, not a threshold tweak.
- **Adjust** (PM): _pending_

## Outcome

- **Result**: Initiative 1 (warp) **closed** by W1 — both cubic and morph as stored styles, recorded as [ADR-0020](../../.docs/adr/ADR-0020-connector-ink-geometry.md). W2–W6 passed on the probe. Initiative 2 (guards) still blocks ship (D25).
- **Evidence**: worktree `exp/connector-ink-warp` `out/report.txt` + `out/index.html`; ADR-0020 is the docs-first restatement.

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
| **Round-1 finding — anchor edge re-selection** | **RESOLVED by the human (D26):** never re-select — the drawn attachment is sacred; an edge anchor's facing is fixed and may U-turn, and the creator's remedy is to switch that end to a **centre** bind, which has no stored edge and therefore cannot oppose the chord. Routes to the geometry ADR as two distinct anchor semantics, and to `/pm` as the **affordance** question (how a creator switches an end to centre in v1) |
| ~~**Round-1 finding — degenerate clamp still unspecified**~~ | **Withdrawn in round 2 — the round-1 attribution was wrong.** Max `|d|` is 1.1 u, so D8 cannot produce a spike of any size; the ~140 u reach belongs to **variant B's** unscaled perpendicular spine extent. With B dropped, **no clamp is needed** and there is no ADR clause to write. BS-0001 R3-I11's "clamp the similarity scale" is superseded: a scale floor makes the degenerate case ~14× worse and detaches variant A's far endpoint by 46.8 u |
| **Round-2 finding — sample parameterization must be pre-blend** | **Geometry ADR clause (algorithm correction).** Blending changes total arc length, so re-placing samples by *post*-blend arc fraction leaks the correction along the entire connector (15.28 u in the middle 70%). Parameterizing on the pre-blend spine confines it (0.02 u) with identical cusp/facing results. Without this, "the blend is local" — the property the whole model is sold on — is false |
| **Round-2 finding — the departure stub must be relative, not absolute** | **Geometry ADR + product vocabulary (`BR-N08`).** The Hermite handle fills a span that is itself `T·L'`, so only the *ratio* governs shape: an absolute 12 u stub scores 5/18 against 17/18 for the relative form, and making the blend span absolute too still tops out at 8/18. Canonical vocabulary is *"leaves straight for about a fifth of its own length"* |
| **Round-2 finding — drop variant B** | **Geometry ADR: name A+C as the only model.** B won no criterion across two rounds and is the sole reason a degenerate clamp was ever considered |
| **Round-2 finding — centre binds need their own bar** | **`/pm` + geometry ADR.** For a centre bind, `s=0` is inside the box; measured at the boundary crossing the departure is 26–32° off the centre ray. The ±5° facing bar applies to **edge** binds only; a centre bind's requirement is "no U-turn, no cusp" |
| **Round-2 finding — conversion to centre clips ~48% of the drawn ink** (worst case) | **RESOLVED by the human:** keep the full rest shape. The hidden portion is what allows the connector to **re-clip correctly** as the box later moves or resizes — trimming to the visible span looks tidier once and breaks re-clipping forever. Document the hidden ink as expected behaviour in the PRD |
| **Round-2 finding — fixed blend span cannot serve every turn angle** | **RESOLVED by the human: adaptive blend approved.** Round 3 derived it — and in an **absolute** arc form, not the fractional form the brief proposed: `blendArc_per_end = min(max(0.15·L', 7.0 · minInkRadius · turn_rad), 0.40·L')`. Geometry ADR clause, with units |
| **Round-3 finding — perpendicular attachment costs 7.8 u of drawn fidelity at rest** | **`/pm` product decision, blocking the ADR.** The unblended warp is a true identity (0.000 u); the blend breaks it because an edge facing is the **edge normal** while a hand-drawn line leaves a face **36–46° off** it. Two things the human asked for are in conflict at the attachment point — *"preserve natural drawing"* vs *"attach perpendicular to the face"*. Supersedes BS-0001 **D16** either way |
| **Round-5 finding — Morph cannot dissolve the preserve-vs-smooth conflict** | **RESOLVED by the human (D32): both styles, stored.** Always-cubic and Morph are two `warpStyle` values on the connector ([ADR-0020](../../.docs/adr/ADR-0020-connector-ink-geometry.md)). Local G1 end-blend is not a creator-facing option. Spike kept on `exp/connector-ink-warp`. Production re-implements the ADR. |
| **Round-1 finding — warp cost is negligible (~7 µs)** | `/pm` + `/designer` option: "warp at commit, not per drag frame" (ASR-6) is now justified **only** by e-ink panel refresh, not CPU. A live warped preview during the drag becomes a product question rather than a performance one |

Also feeds: `ADR-0010` §6 amendment (`facing`, `centre`), and the `/designer` ToolChip story
(3 tools + 2 recognizer toggles, BS-0001 ASR-7) which does **not** depend on this EXP's verdict.

## Code disposition

- [ ] Discard sandbox worktree (default)
- [x] Keep as reference spike — human: working code on `exp/connector-ink-warp`; numeric source for ADR-0020
- [ ] Promote to production via story(ies) — re-implemented docs-first against [ADR-0020](../../.docs/adr/ADR-0020-connector-ink-geometry.md), never copy-pasted
