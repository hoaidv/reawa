# Connector ink warp — canonical algorithm

Output of EXP-0002 rounds 1–3. This is the implementation-ready description of the final
model for an architect to lift into an ADR. Every constant below is the value the round-3
probe ships and measures; the evidence for each is in `report.txt`.

All lengths are in **world units** (u). Angles are degrees unless a step says radians.

---

## 0. Vocabulary and tunables

| Product name | Code name | Value | What it means to a person |
|---|---|---|---|
| Blend length | `blendLength` | `0.15` | The share of each end of the drawn line the system may reshape, before any turn is taken into account. |
| Departure stub ratio | `departureStubRatio` | `1.5` | How straight the line leaves the box face, as a multiple of the blend arc. Higher is straighter off the face. |
| Turn room | `turnRoomFactor` | `7.0` | How much extra line a hard turn earns itself. Multiplies `minimum ink radius × turn`. |
| Blend cap | `blendCap` | `0.40` | The most of each end the system may ever reshape, so at least 20% of the drawn line is always untouched. |
| Minimum ink radius | `minInkRadius` | `12.0` u | The tightest curve the warp is allowed to bend the ink into. |

There are exactly five. Nothing else in this document is tunable.

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

The rest shape is `{P.size() pairs of (s, d)} + {rest chord}`. It is written once and never
rewritten. See invariant I1.

---

## 2. Anchors and facings

An anchor is one of exactly two kinds. The kind is chosen when the connector is recognised
and is only ever changed by the creator.

**Edge anchor.** Stores `(edge, t)`: which of the box's four edges, and a position `t ∈ [0,1]`
along it. Both the anchor point and the facing are re-derived from the box on every change,
so the anchor follows a move, a resize and a rotation.
- Anchor point: `corner[edge] + (corner[(edge+1) mod 4] − corner[edge]) · t`.
- Facing: that edge's outward normal, rotated by the box's rotation. **The edge is never
  re-selected.** If the creator moves the box past its peer, the facing ends up opposing the
  chord and the connector performs a U-turn. That is accepted behaviour, not a defect.

**Centre anchor.** Stores no edge.
- Anchor point: the box centre.
- Facing: the unit ray from the box centre toward the peer anchor point. It therefore can
  never oppose the chord.
- The warped sample chain is **clipped at the box boundary**: drop the leading (or trailing)
  samples that fall inside the box's axis-aligned bounding box, and move the first surviving
  sample onto the boundary intersection. **Keep the whole rest shape.** The hidden ink is what
  allows correct re-clipping when the box is later moved or resized.

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
   handle lengths.

3. **Apply the blend.** Let `t0`, `t1` be the two blend lengths as fractions of `L'`, and let
   `cum(i)/L'` be each spine point's normalized arc position `s` along `U`.
   - `Q0 = U(t0)` with unit tangent `T0`; `Q1 = U(1 − t1)` with unit tangent `T1`.
   - For `s < t0`: `V[i] = Hermite(P0', f0'·outer0, Q0, T0·inner0, s / t0)`
   - For `s > 1 − t1`: `V[i] = Hermite(Q1, T1·inner1, P1', −f1'·outer1, (s − (1 − t1)) / t1)`
   - Otherwise `V[i] = U[i]`.
   - Force `V[0] = P0'` and `V[last] = P1'`.

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
   is ≤ 0.03 u across the whole case set.

   `d` is **never scaled**. It stays in world units at every scale.

5. **Clip** each end that is a centre anchor, per section 2.

---

## 4. The adaptive blend

The blend arc each end needs is set by the turn that end has to absorb. It is an **absolute
arc length**, not a fraction of the connector.

For each end (shown for end 0; end 1 mirrors with `−f1'` and the far end of the spine):

1. **Measure the turn.** Read the unit tangent of the unblended spine `U` at arc
   `blendLength · L'`. The turn is the angle between `f0'` and that tangent, in radians:
   `turn0 = angle(f0', U'(blendLength · L'))`.

   Measuring at the *base* blend length, not at the final one, is what keeps this a
   single-pass computation with no iteration and no fixed point.

2. **Compute the demanded arc.**
   `arc0 = max(blendLength · L', turnRoomFactor · minInkRadius · turn0)`
   = `max(0.15 · L', 7.0 · 12.0 · turn0)`.

3. **Apply the cap.** `t0 = min(arc0 / L', blendCap)` = `min(arc0 / L', 0.40)`.
   Both ends are capped independently, so at least `1 − 2 · 0.40 = 20%` of the drawn line is
   always left untouched.

4. **Handles.**
   - inner handle `inner0 = t0 · L'` (matched to the blend arc, so the curve arrives at the
     junction at the spine's own speed);
   - outer handle `outer0 = departureStubRatio · inner0 = 1.5 · t0 · L'`.

   Only the outer handle carries the ratio. Applying it to the inner handle as well was
   measured in round 2 and is worse on every metric.

**Why this form.** The alternative in the brief — `blendArc = blendLength · L' · (1 + α · turn)`
— was tested and rejected. Asking the same question at five connector lengths, the multiplier
needed to reach the radius bar varies by 4–6.5× while the *arc* needed varies by under 10%
(≈72 u at 90° of turn, ≈102 u at 100°, at every length from 90 u to 500 u). The requirement is
absolute because turning `turn` at radius `R` costs `R · turn` of arc no matter how long the
line is. `turnRoomFactor` is the Hermite's curvature-concentration factor over the ideal
circular value; 7.0 is the envelope of the measured 3.5–6.8, not a mean.

The base blend acts as the floor, so the rule is inert below the turn where the demand exceeds
it — about 52° on a 400 u connector, about 13° on a 100 u one. No separate threshold constant
is needed.

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
  the ink anyway.
- Round 1 attributed a ~140 u spike here to `d` staying absolute. That was wrong. The spike
  belonged to the anisotropic warp variant, which left the spine's perpendicular extent
  unscaled. That variant is dropped, and with it the entire clamp question.

---

## 6. Invariants

- **I1 — never re-bake.** The rest shape is derived once, from the creator's ink, and is never
  rebuilt from a warped result. Every warp starts from the original rest shape. Measured: 200
  successive poses returning to the start reproduce the first output to **0.000000000 u** under
  this rule, and drift to **60 u** without it.
- **I2 — pure function.** The warped output is a function of the rest shape and the two
  endpoint states alone. No global state, no history, no time, no iteration to a fixed point.
  Measured: byte-identical across repeat runs and across running the case set in reverse order.
- **I3 — the blend is local.** Ink outside the two blend regions is untouched by the blend, to
  ≤ 0.03 u. This is guaranteed by the parameterization rule in step 3.4, not by luck.
- **I4 — the middle survives.** The blend cap guarantees at least 20% of the drawn arc is never
  reshaped, at any obliquity and any connector length.
- **I5 — the drawn attachment is sacred.** An edge anchor's edge is never re-selected by the
  system.

Cost: **≈5.5 µs median, under 8 µs p95** for a full re-warp of a 500-sample connector, single
threaded, host build. The blend adds ≈0.7 µs over the similarity warp alone.

---

## 7. Known limitations, stated not hidden

1. **The model does not reproduce the creator's own line at rest.** An edge anchor's facing is
   the edge normal, and a hand-drawn line does not leave the face along its normal — measured
   36–46° off on the two probe rest shapes. So the blend fires even when nothing has moved,
   moving the ink by up to **7.8 u** against the ink the creator actually drew. This is a
   consequence of the facing definition, not of any constant, and it is the single largest
   open question for the naturalness verdict.
2. **A backtrack is forced above about 90° of departure-chord angle.** When the required facing
   points more than ~90° away from the straight line across the blend region, the ink must set
   off away from where it has to arrive. No combination of blend arc and stub ratio removes it;
   a 5×5 grid of both at 108° of turn overshoots in every cell. This is the anchor's geometry,
   not a tuning failure.
3. **Short connectors at high obliquity cannot meet the radius bar.** Below roughly 150 u of
   arc with more than 80° of turn, the rule demands 1.5–3.0 of the arc per end and receives
   0.40. Behaviour degrades smoothly with the shortfall — no cliff — but the bar is missed.
4. **Interior pins (approach D) are not built**, so a connector drawn to detour around a third
   box drags its detour along with the warp rather than keeping it anchored.
