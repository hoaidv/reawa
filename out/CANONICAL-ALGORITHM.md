# Connector ink warp — canonical algorithm

Output of EXP-0002 rounds 1–4. This is the implementation-ready description of the final
model for an architect to lift into an ADR. Every constant below is the value the round-4
probe ships and measures; the evidence for each is in `report.txt`.

All lengths are in **world units** (u). Angles are degrees unless a step says radians.

The one-sentence version: **the creator's ink is carried rigidly by a similarity transform, and
the only thing the system is allowed to reshape is as much arc at each end as the turn at that
end demands — which is nothing at all when nothing has turned.**

---

## 0. Vocabulary and tunables

| Product name | Code name | Value | What it means to a person |
|---|---|---|---|
| Departure stub ratio | `departureStubRatio` | `1.5` | How straight the line leaves the anchor, as a multiple of the blend arc. Higher is straighter off the face. |
| Turn room | `turnRoomFactor` | `5.0` | How much line a turn earns itself. Multiplies `minimum ink radius × turn (radians)`. |
| Blend cap | `blendCap` | `0.35` | The most of each end the system may ever reshape, so at least 30% of the drawn line is always untouched. |
| Minimum ink radius | `minInkRadius` | `12.0` u | The tightest curve the warp is allowed to bend the ink into. |

There are exactly **four**. Round 3's fifth, `blendLength` (a base blend of 0.15 of the arc per
end), **is deleted**. It was the only reason the system touched ink that had no reason to move,
and removing it is what makes invariant I6 exact rather than approximate. See section 4.

---

## 1. Rest-shape construction — once, at recognition time

Input: the connector body as an ordered list of raw strokes, each a polyline of
world-space points, in draw order.

1. Concatenate the strokes in draw order into one raw polyline `P[0..m-1]`. Do not
   de-duplicate across stroke boundaries and do not insert joining segments; a multi-stroke
   connector with small gaps between strokes is handled correctly by steps 2–3.
2. Resample `P` at a uniform arc-length spacing of **2.0 u** → `R`.
3. Smooth `R` with repeated binomial `[1,2,1]/4` passes equivalent to a Gaussian of
   **sigma 6.0 u** — that is `round(2·sigma²/spacing²) = 18` passes at the spacing in step 2.
   Pin `R[0]` and `R[last]` through every pass. Resample the result at 2.0 u again. This is
   the **rest spine** `S[0..k-1]`.
4. For every point `p` of the **raw** polyline `P` (not `R`), find the nearest point on `S`
   across all segments and store a pair:
   - `s` = arc length at that projection ÷ total arc length of `S`, in `[0,1]`;
   - `d` = the signed perpendicular distance from the projection to `p`, in world units,
     signed by the cross product of the segment tangent with `p − projection`.
5. Store alongside: the rest chord `S[k-1] − S[0]`, its length, and the maximum turn angle
   between consecutive segments of `S`.
6. **Capture the drawn departure at each end** (new in round 4, decision D27):
   - `drawn0` = the unit tangent of `S`'s **first segment**;
   - `drawn1` = the unit tangent of `S`'s **last segment, negated**, so both point outward.

   Store each one in its anchor's own local frame, per section 2. Take these from the **spine**
   `S`, not from the first two points of the raw ink: the spine is what the warp reconstructs and
   therefore what an identity has to reproduce, and a smoothed centreline is far more stable
   than the tangent between two adjacent raw samples of a hand-drawn stroke.

The rest shape is `{P.size() pairs of (s, d)} + {rest chord} + {two stored departures}`. It is
written once and never rewritten. See invariant I1.

---

## 2. Anchors and facings

An anchor is one of exactly two kinds. The kind is chosen when the connector is recognised
and is only ever changed by the creator.

**Edge anchor.** Stores `(edge, t, drawnEdgeLocal)`: which of the box's four edges, a position
`t ∈ [0,1]` along it, and the drawn departure of section 1 step 6 expressed in that edge's own
frame. Anchor point and facing are both re-derived from the box on every change, so the anchor
follows a move, a resize and a rotation.

- Anchor point: `corner[edge] + (corner[(edge+1) mod 4] − corner[edge]) · t`.
- **Facing: the drawn departure, carried rigidly with the edge.** The edge frame is
  `(n, e)` where `n` is that edge's outward normal rotated by the box's rotation and
  `e = leftNormal(n)` runs along the edge. At recognition, store
  `drawnEdgeLocal = (drawn · n, drawn · e)`. On every later change, recover
  `facing = drawnEdgeLocal.x · n + drawnEdgeLocal.y · e` from the box's *current* `n`, `e`.
  This is invariant under translation and resize and rotates rigidly with the box.
- **The edge is never re-selected.** If the creator moves the box past its peer, the facing ends
  up opposing the chord and the connector performs a U-turn. That is accepted behaviour, not a
  defect (D26).

This replaces round 3's rule, which used the edge's outward normal. The two are structurally
identical — both store a direction in the edge frame and carry it rigidly — so this is not an
architectural change; it changes only what the stored direction is derived from. It was chosen
by the human over perpendicular attachment, with the price of perpendicular attachment measured
and on the table: **7.8 u of the creator's ink, moved on a connector nobody had touched.**

**Centre anchor.** Stores no edge.

- Anchor point: the box centre.
- Facing: the unit ray from the box centre toward the peer anchor point. It therefore can
  never oppose the chord.
- The warped sample chain is **clipped at the box boundary**: drop the leading (or trailing)
  samples that fall inside the box's axis-aligned bounding box, and move the first surviving
  sample onto the boundary intersection. **Keep the whole rest shape.** The hidden ink is what
  allows correct re-clipping when the box is later moved or resized.
- **Open decision.** A centre anchor's facing is derived, not drawn, so it does not satisfy I6:
  at rest the derived ray sits 8–23° off the ink and the blend fires, moving the ink by up to
  2.5 u with nothing moved. A **60° cone rule** — keep the drawn departure, clamped to a 60° cone
  about the ray — was built and measured: it restores I6 exactly on both probe shapes and still
  rescues every U-turn case (0 new cusps, 0 overshoot, the ink re-enters the box 0 times, the
  same 51–54% of ink kept). A 90° cone does **not**: it overshoots 12–14 times. This is the one
  thing in the model the probe recommends but the human has not ruled on. Until they do, a centre
  bind is exempt from I6 and the exemption must be stated wherever I6 is.

Switching a U-turning end to a centre anchor is the creator's remedy for a U-turn. The system
never applies it silently.

---

## 3. The warp — on every endpoint change

Inputs: the rest shape, the two new anchor points `P0'`, `P1'`, and the two outward facing
unit vectors `f0'`, `f1'`.

1. **Similarity transform of the spine.** With `c` the rest chord and `c'` the new chord:
   - `scale = |c'| / |c|`
   - `theta = atan2(c'.y, c'.x) − atan2(c.y, c.x)`
   - `U[i] = P0' + Rot(theta) · (scale · (S[i] − S[0]))`

   `U` is the **unblended spine**. There is no scale floor and no clamp of any kind.
   Let `L' = ` arc length of `U`.

2. **Plan the blend** (section 4). This yields, for each end, a blend arc and two Hermite
   handle lengths. **If both blend arcs are zero, skip steps 3 and set `V = U`.** This is not an
   optimisation; it is the identity path, and it must be a true skip rather than a blend with
   zero-length parameters — see section 4.

3. **Apply the blend.** Let `t0`, `t1` be the two blend lengths as fractions of `L'`, and let
   `cum(i)/L'` be each spine point's normalized arc position `s` along `U`.
   - `Q0 = U(t0)` with unit tangent `T0`; `Q1 = U(1 − t1)` with unit tangent `T1`.
   - For `s < t0`: `V[i] = Hermite(P0', f0'·outer0, Q0, T0·inner0, s / t0)`
   - For `s > 1 − t1`: `V[i] = Hermite(Q1, T1·inner1, P1', −f1'·outer1, (s − (1 − t1)) / t1)`
   - Otherwise `V[i] = U[i]`.
   - Force `V[0] = P0'` and `V[last] = P1'`.

   Both comparisons are **strict**, which is what makes `t = 0` degenerate to a no-op.

   `Hermite(p0, m0, p1, m1, u) = h00·p0 + h10·m0 + h01·p1 + h11·m1` with the standard cubic
   basis. `m0`, `m1` are derivatives over the unit interval, so a handle length of `L` means
   the curve leaves at speed `L`.

   `V` is the **warped spine**.

4. **Re-place the samples.** For each stored `(s, d)`:
   - Locate `s · L'` **on `U`**, the unblended spine, giving a segment index `i` and a
     fraction `u` within it. This is the parameterization; it is deliberately not `V`'s.
   - Read the position and unit tangent from `V` at that same `(i, u)`.
   - The warped sample is `position + leftNormal(tangent) · d`.

   Parameterizing on `U` and reading geometry from `V` is what makes the blend **local**: the
   blend changes total arc length, and parameterizing on `V` would let that change shift ink
   in the middle of the connector. Measured leakage into the untouched middle with this rule
   is ≤ 0.06 u across the whole case set, at every obliquity including 180°.

   `d` is **never scaled**. It stays in world units at every scale.

5. **Clip** each end that is a centre anchor, per section 2.

**A note for whoever writes the departure test.** Step 3 sets the Hermite's start derivative to
`f0' · outer0`, so the curve leaves the anchor *exactly* along the required facing; the analytic
deviation is zero by construction and there is nothing to measure. Any non-zero number comes from
measuring a **secant** — the direction from the anchor to the first rendered sample — and a secant
over a baseline `b` on a curve of radius `R` is unavoidably `≈ 28.6 · b / R` degrees off the
tangent. On this model that is up to 9° over the 5–6 u baselines the sample spacing produces, so a
flat ±5° secant bar is in direct conflict with the 12 u radius bar: a curve that satisfies one
must violate the other. Test the departure analytically, or state the bar as
`max(5°, 28.6 · baseline / minInkRadius)`. Round 4 measures 5.7° worst case against the flat bar
and 0 failures against the radius-consistent one.

---

## 4. The adaptive blend

The blend arc each end needs is set by the turn that end has to absorb. It is an **absolute
arc length**, not a fraction of the connector, and it has no floor.

For each end (shown for end 0; end 1 mirrors with `−f1'` and the far end of the spine):

1. **Measure the turn at the anchor.** `turn0 = angle(f0', U'(0))` — the angle between the
   required facing and the unblended spine's own departure direction at the anchor, in radians.

   Round 3 measured this at `blendLength · L'` along the spine, because the blend arc had a base
   value to measure at. With the base gone there is nothing to measure at but the anchor itself,
   which is also the honest place: it is exactly the mismatch the blend exists to absorb. For a
   rigidly-carried facing this quantity has a plain reading — **it is how much the box rotated
   relative to how much the chord rotated.** Move a box without rotating it and both ends still
   see a turn, because the chord rotated under a facing that did not.

2. **Compute the demanded arc.** `arc0 = turnRoomFactor · minInkRadius · turn0`
   = `5.0 · 12.0 · turn0` = `60 · turn0`, with `turn0` in radians. No `max`, no floor.

3. **Apply the cap.** `t0 = min(arc0 / L', blendCap)` = `min(arc0 / L', 0.35)`.
   Both ends are capped independently, so at least `1 − 2 · 0.35 = 30%` of the drawn line is
   always left untouched.

4. **Handles.**
   - inner handle `inner0 = t0 · L'` (matched to the blend arc, so the curve arrives at the
     junction at the spine's own speed);
   - outer handle `outer0 = departureStubRatio · inner0 = 1.5 · t0 · L'`.

   Only the outer handle carries the ratio. Applying it to the inner handle as well was
   measured in round 2 and is worse on every metric. Re-swept in round 4 on the new
   distribution against 0.5, 1.0, 2.0 and 2.5: 1.5 gives the loosest minimum radius in every row
   inside the serviceable turn range. The three rows it loses are at 102° of turn, past the
   backtrack boundary of limitation 2, and it loses them by 0.1–0.5 u.

### 4.1 Why the arc is absolute, and why 5.0

Round 2's form — `blendArc = blendLength · L' · (1 + α · turn)` — was tested and rejected.
Asking the same question at five connector lengths, the *multiplier* needed to reach the radius
bar varies by 4–6.5× while the *arc* needed varies by under 10%. The requirement is absolute
because turning through `turn` at radius `R` costs `R · turn` of arc no matter how long the line
is, so no dimensionless coefficient of `L'` can exist. Re-measured on the round-4 baseline, the
ratio `arc*/(R · turn)` is constant to ±2% across a 6× range of connector length at fixed box
rotation (2.82–2.94 at 15° of rotation, 2.97–3.01 at 30°), and rises with the turn itself,
because a Hermite absorbing a bigger turn concentrates its curvature more.

`turnRoomFactor` is therefore an envelope over the turn range served — but round 4 found that
**the constant is bounded above as well as below**, which round 3 missed. Sweeping it and
counting new cusps over the 40–59 connectors in scope at each value:

| turn room | 2.0 | 3.0 | 4.0 | **4.5** | **5.0** | **6.0** | **6.5** | 7.0 | 8.0 |
|---|---|---|---|---|---|---|---|---|---|
| cases failing | 47 | 21 | 4 | **0** | **0** | **0** | **0** | 1 | 1 |

The clean window is **4.5–6.5**. Round 3's 7.0 — an envelope of per-point minima, which is the
wrong estimator here — sits just outside it. The two edges fail for different reasons. Below 4.5
the rest shape inside the blend window is nearly straight (413 u, 720 u and 1221 u of radius in
three of the four failures) and the tight radius is the blend's own bend: it has too little arc
for the turn. At 7.0 and above, the window has grown long enough to swallow the creator's own
curvature (23 u) and the blend adds its bend on top of it. **Arc is not a free resource: too much
of it is its own failure mode.** The model ships 5.0, inside the window with margin on both sides
rather than on an edge.

### 4.2 Why this makes identity at rest exact

With no floor, `arc = 0` whenever `turn = 0`, so the blend does not exist when nothing has
turned, and the output is the similarity warp of the creator's own ink — bitwise. Identity is a
**consequence of the sizing rule**, not a special case bolted onto it. Four mechanisms were
built and measured at rest (`report.txt` section 1b):

| mechanism | at-rest deviation from the ink | bitwise identical? | jump on the first 2° of rotation |
|---|---|---|---|
| M0 round 3 (edge normal, base blend) | 7.69 / 7.82 u | no | 8.4 u |
| M1 drawn facing, base blend kept | 4.05 / 4.77 u | no | 4.1 u |
| M2 M1 with the stub ratio collapsed to 1.0 | 0.01 / 2.91 u | no | 0.4 u |
| M3 drawn facing, base blend, 1° turn deadband | 0.00 / 0.07 u | **yes** | 4.1 u |
| **M4 drawn facing, no floor (shipped)** | **0.00 / 0.07 u** | **yes** | **0.04 u** |

M2 fails because a matched-direction Hermite still does not reproduce the spine unless the
handles match the spine's own speed, and on a wiggly shape it does not. M3 reaches the identity
but is a **cliff**: it is exact at zero and jumps 4 u the moment the deadband is crossed, and the
creator sees that jump on the smallest drag they can perform. M4 has no threshold to cross, so
the correction ramps from nothing — 0.000 u at 1° of rotation, 0.043 u at 2°, 0.31 u at 4°.

Two properties of M4 worth knowing before implementing it:

- **No exact-zero float comparison is required.** If the round trip through the edge frame leaves
  a residual turn of, say, `1e-14` degrees, the demanded arc is far shorter than one sample
  spacing, so the only vertex inside the blend window is the first — where the Hermite basis
  evaluates to `p0` exactly. The output is still bitwise the similarity warp. Do not add a
  tolerance; it is not needed and it would reintroduce a cliff.
- **There is a natural deadband of one sample spacing, and it costs nothing.** The smallest turn
  whose demanded arc exceeds the 2.0 u resample spacing is **1.91°**. Below that the blend has no
  vertex to act on and the departure is simply left as drawn — which is inside the 5° facing bar
  by a factor of 2.6. This is a free consequence of the sizing rule, not a constant. It reaches
  further than literal rest: a **resize** of either box turns the facing by only 1.6° on the probe
  cases, so a resize also ships the ink carried rigidly, bit for bit, with no blend at all.

---

## 5. Degenerate behaviour

No clamps. The similarity scale is used as computed, including at `scale ≈ 0.009`.

- As the two anchors converge, `L' → 0`. The blend hits its cap at both ends, the connector
  collapses to a short smooth stub between the anchors, and the stored offsets `d` — which are
  absolute — dominate what remains.
- A connector whose warped arc `L'` is below **24 u** (twice the minimum ink radius) cannot be
  held to a radius bar at all, because the whole connector is shorter than two of the radii it
  is being asked not to bend tighter than. Such connectors are outside the cusp criterion. They
  do not misbehave; they are just not scoreable, and at that size they are below the width of
  the ink anyway. Round 3 instead relaxed the bar in proportion to the span, which let a capped
  blend hide its own shortfall; round 4 uses a flat 12 u bar plus this scoreability floor.
- Round 1 attributed a ~140 u spike here to `d` staying absolute. That was wrong. The spike
  belonged to the anisotropic warp variant, which left the spine's perpendicular extent
  unscaled. That variant is dropped, and with it the entire clamp question.

---

## 6. Invariants

- **I1 — never re-bake.** The rest shape is derived once, from the creator's ink, and is never
  rebuilt from a warped result. Every warp starts from the original rest shape. Measured: 200
  successive poses returning to the start reproduce the first output to **0.000000000 u** under
  this rule, and drift to **21 u** without it.
- **I2 — pure function.** The warped output is a function of the rest shape and the two
  endpoint states alone. No global state, no history, no time, no iteration to a fixed point.
  Measured: byte-identical across repeat runs and across running the case set in reverse order.
- **I3 — the blend is local.** Ink outside the two blend regions is untouched by the blend, to
  ≤ 0.06 u. This is guaranteed by the parameterization rule in step 3.4, not by luck.
- **I4 — the middle survives.** The blend cap guarantees at least 30% of the drawn arc is never
  reshaped, at any obliquity and any connector length.
- **I5 — the drawn attachment is sacred.** An edge anchor's edge is never re-selected by the
  system.
- **I6 — identity at rest.** With no endpoint moved, the output is **bitwise** the similarity
  warp of the creator's own ink: no blend fires, because there is no turn to absorb. Measured
  residual between blended and unblended output: **0.000000000 u** on both probe rest shapes.
  This is the invariant to write a unit test for first, and it is cheap to test: warp with
  unchanged boxes and `memcmp` the result.
  - Two caveats an implementer must know. **(a)** I6 is an identity against the *rest-shape
    reconstruction*, which is what the system stores and can reproduce — not against the raw
    input polyline. The `(s, d)` representation itself loses **0.07 u** on the wiggly probe shape,
    because a nearest-point projection drops the tangential component of any sample whose foot
    falls outside its segment. The unblended warp sits exactly the same 0.07 u from the raw ink,
    so this residual is the price of the store, not of the warp, and no blend change can reach
    it. If the product wants 0.000 u against the raw ink, that is a rest-shape representation
    question — keep a tangential offset as well as `d` — and it should be raised as such.
    **(b)** I6 covers edge anchors. A centre bind is exempt until the cone decision in section 2
    is made.

Cost: **≈5.5 µs median, 6.3 µs p95** for a full re-warp of a 500-sample connector, single
threaded, host build. The blend adds ≈0.7 µs over the similarity warp alone — and at rest it adds
nothing, because there is no blend.

---

## 7. Known limitations, stated not hidden

Round 3 listed four. Round 4 retired the first, shrank the third to a fifth of its size, and
confirmed the second is geometry rather than tuning.

1. ~~**The model does not reproduce the creator's own line at rest.**~~ **Retired.** This was the
   largest limitation in the model and the reason round 4 exists. The facing is now the drawn
   departure and the blend has no floor, so the at-rest output is bitwise the creator's ink. See
   I6, including its two caveats — the 0.07 u representation residual and the centre-bind
   exemption — neither of which is a warp defect.

2. **A backtrack is forced above about 90° of departure-chord angle.** Unchanged, and now
   confirmed as fundamental. When the required facing points more than ~90° away from the
   straight line across the blend region, the ink must set off away from where it has to arrive.
   No combination of blend arc and stub ratio removes it: a 5×5 grid of both at 102° of turn
   overshoots in every cell, and the turn-room sweep fails in this region at every value from 2.0
   to 8.0. The measured boundary is sharp and sits where the geometry says it should — the radius
   bar holds at 87° of turn (13.5 u) and fails at 102° (10.8 u). **This is the anchor's geometry,
   not a tuning failure, and the creator's remedy is the centre bind (W7), which passes.**

3. **Short connectors at high obliquity cannot meet the radius bar** — but the floor moved a long
   way down. Round 3 put it at "below roughly 150 u of arc with more than 80° of turn", measured
   against a baseline that already spent 36–46° of turn on the facing mismatch. With that spent
   turn gone, only motion the creator actually applied costs anything, and:

   | drawn arc | at rest | box rotated 15° (20–44° of turn) | box rotated 45° (56–96° of turn) |
   |---|---|---|---|
   | 21 u | exact | not scoreable | not scoreable |
   | 32 u | exact | fails (7.6 u) | fails (3.4 u) |
   | 49 u | exact | **passes (12.7 u)** | fails (5.2 u) |
   | 76 u | exact | **passes (17.8 u)** | fails (8.2 u) |
   | 120 u | exact | **passes (20.9 u)** | **passes (14.0 u)** |
   | 152 u | exact | **passes (20.6 u)** | **passes (18.5 u)** |

   The turn ranges in the column headings are the point: the *same* box rotation produces a much
   larger turn on a short line than a long one, because the chord swings further relative to a
   facing that is carried rigidly. A short connector is not penalised for being short; it is
   penalised because the same gesture asks more of it.

   **The practical floor is ≈50 u of drawn arc for ordinary handling, and ≈120 u before the model
   is comfortable at high obliquity** — against ~150 u for any turn at all in round 3. Identity
   holds at every length, including 21 u. For the recognition guard's minimum-length constant in
   the other initiative: the warp does not justify a floor above **50 u**, and below **24 u** the
   connector is shorter than two minimum radii, so no radius bar can be stated for it at all.
   Note this is the *warp's* floor only — recognition may well want a larger one for its own
   reasons, and it should not cite this number as if it were the binding constraint.

4. **Interior pins (approach D) are not built**, so a connector drawn to detour around a third
   box drags its detour along with the warp rather than keeping it anchored.

5. **The blend cap binds on 11% of healthy ends** — 15 of 140 aligned, scoreable, non-backtracking
   ends across the whole population. Where it binds, the end receives as little as 14% of the arc
   it asked for, and it is the short-connector rows in limitation 3 that it binds on. The cap is
   the mechanism that converts "this connector is too short for this rotation" from a
   self-intersecting mess into a merely-too-tight curve, so its binding is a symptom of
   limitation 3 and not an independent defect. 0.35 rather than 0.40 costs **one** additional
   healthy end out of 140 and buys 10 more points of guaranteed untouched middle.

---

## 8. What the human still has to decide

1. **W1, naturalness.** The contact sheet (`index.html`) is the artifact. Read the two `identity`
   panels first: one line, no blend band, nothing touched.
2. **The centre-bind cone rule** in section 2. The probe recommends the 60° cone: it is the only
   option measured that gives a centre bind the same at-rest identity an edge bind now has,
   without losing the U-turn rescue.
