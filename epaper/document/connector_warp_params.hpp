#pragma once
/**
 * Connector warp tunables — Morph (Ink) and Always-cubic (Curve).
 * @implements [SRS-EP-18] named constants for rest shape, facing, Morph mix
 * @implements [ADR-0020] production reimplementation; Local G1 is not a style
 *
 * Edit values here while tuning. Each constant documents unit, direction, and
 * the visual effect on the final connector. No algorithm lives in this file.
 *
 * World unit `u` = document millimetre-class world space (ink samples /
 * SmartGroup bounds).
 *
 * Not tunables of ADR-0020 (unshipped Local G1 end-blend — do not wire):
 *   stubRatio, turnFactor, blendCap.
 */

namespace epaper {
namespace document {

// ---------------------------------------------------------------------------
// kRestResampleWorld — rest spine sample spacing
// Default: 2.0
// Unit: world u (arc length between consecutive samples of S)
// Direction: higher = coarser (fewer samples, longer chords). Lower = finer
//   (more samples).
// Visual: Finer captures small wiggles into S and into (s,d) so Morph at rest
//   looks closer to the raw stroke; Cubic still redraws onto a Hermite but the
//   wiggle rides as denser d. Coarser drops high-frequency ink (kinks become
//   polygonal), Morph identity still holds but the reconstruction is a
//   simplified line; live warp is cheaper. Below ~1 u is usually below ink
//   width — more CPU, little visible gain.
// ---------------------------------------------------------------------------
constexpr double kRestResampleWorld = 2.0;

// ---------------------------------------------------------------------------
// kRestSigmaWorld — rest-spine smoothing
// Default: 6.0
// Unit: world u (Gaussian σ; binomial passes ≈ round(2·σ² / spacing²), pinned
//   ends)
// Direction: higher = coarser / rounder S (more blur). Lower = finer (closer
//   to resampled raw).
// Visual: Higher σ irons handwriting: rest Morph looks smoother, inflection
//   count drops → more connectors auto-pick Curve. Lower σ keeps tremor and
//   corners on S; more Ink (Morph) auto-picks; Cubic then has more d offset so
//   Curve looks “ink around a tidy bezier.” Too high (≫ ink width) melts the
//   stroke into a near-chord; too low (~spacing) barely smooths.
// ---------------------------------------------------------------------------
constexpr double kRestSigmaWorld = 6.0;

// ---------------------------------------------------------------------------
// kInflectionCubicMax — style auto-pick cutoff
// Default: 1 (≤1 inflections on S → cubic, else morph)
// Unit: count (dimensionless), inflections of rest spine S only — never of
//   cubic C
// Direction: higher = more connectors classified Curve (looser “smooth”
//   test). Lower = finer / stricter (more stay Ink/Morph). 0 means only a
//   monotonic-curvature (or straight) spine is Curve.
// Visual: Does not change how a given style warps; it changes which style is
//   stored at recognition. Raising it makes wiggly handwriting get
//   Always-cubic (~10 u tidy at rest). Lowering it keeps more lines as Morph
//   (identity at rest, spends ink as soon as a box turns).
// ---------------------------------------------------------------------------
constexpr int kInflectionCubicMax = 1;

// ---------------------------------------------------------------------------
// kCentreConeDeg — centre-end facing vs peer
// Default: 60
// Unit: degrees, half-angle of a cone about the peer-centre ray (this end’s
//   centre → other end’s centre)
// Direction: higher = looser (drawn leave angle may deviate more from the
//   peer). Lower = tighter (leave more facing the counterpart). 0 = always
//   the peer ray (drawn direction unused while the end is centre). Probe used
//   negative as “pure ray”; 90° overshot 12–14×.
// Visual: Centre bind: the cubic/Morph handle at that end points within this
//   cone. Tight/0: when you move/scale/rotate the box, the connector always
//   enters toward the other node, so initial doodle direction does not
//   matter. Loose: the original leave angle is kept until the peer ray and
//   doodle disagree by more than the cone, then it clamps — can look like a
//   hook that refuses to face the peer. Does not apply to edge binds.
// ---------------------------------------------------------------------------
constexpr double kCentreConeDeg = 60.0;

// ---------------------------------------------------------------------------
// kEdgeFacingPerpendicular — NEWS/TRBL leave vs drawn face
// Default: false
// Unit: boolean (not a magnitude)
// Direction: false = preserve drawn leave in the edge local frame (n, e).
//   true = force perpendicular to the attached face = that face’s outward
//   normal in the node’s local NEWS, then rotated with the node into world.
// Frame (load-bearing, both values): Leave is always node-local, never
//   world-axis NEWS.
//   - Node rotation = 0: node NEWS coincides with page NEWS.
//   - Node rotated / scaled / moved: the same local leave is re-expressed
//     with the box (n, e from current corners / transform.rotation).
//   - false: a stroke that left a side at 30° off-normal keeps that 30°
//     relative to the box (stored drawnEdgeLocal; world tangent =
//     drawnN·n' + drawnE·e'). Not 30° vs world-north.
//   - true: leave is 0° off that face’s outward normal (node-local NEWS).
//     A 30° box still leaves 90° off its own edge, which is 30° off
//     world-north if that edge is the node’s “top”. Never snap to world ±X/±Y.
// Visual: false (ship): handwriting leave angle rides the face through
//   move/resize/rotate; a U-turn after the box passes its peer is accepted.
//   true: tidier local “port” (always perpendicular to the attached face in
//   node space). ADR measured ~7.8 u of the creator’s ink moving at rest
//   (unrotated) with this on. Corners/t are unchanged; only the outgoing
//   tangent used by Hermite/Morph mix changes. Implement from current face
//   n,e — never world-axis snap.
// ---------------------------------------------------------------------------
constexpr bool kEdgeFacingPerpendicular = false;

// ---------------------------------------------------------------------------
// kMorphSatDeg — Morph mix saturation
// Default: 90
// Unit: degrees of turn (max of angle between each end’s facing and U’s
//   tangent at that end)
// Direction: higher = Morph stays Ink longer (need more turn before m→1).
//   Lower = Morph becomes Cubic sooner (small box rotations already look
//   like Curve).
// Visual: m = 0.5·(1−cos(π·min(1, turn/sat))). At turn=0, m=0 exactly
//   (bitwise rest reconstruction). Lower sat: a 15° rotate already spends
//   most of the stroke onto the cubic (wiggle flattened). Higher sat:
//   handwriting holds through moderate rotates, but a hard facing change
//   still shows a kink vs cubic in the middle. Versine m'(0)=0 so a 2° nudge
//   does not pop. Does not affect Curve style (V=C always).
// ---------------------------------------------------------------------------
constexpr double kMorphSatDeg = 90.0;

// ---------------------------------------------------------------------------
// kHandleModeRestSpeed — cubic handle length rule
// Default: true (Hermite speed = L' = arc length of similarity-warped spine U)
// Unit: boolean. Alternate (when false): speed = chord |P1'−P0'| / 3
// Direction: Rest-speed handles are longer than chord/3 on a bent rest shape.
//   Chord/3 is shorter / tighter.
// Visual: Longer handles (true): Curve and Morph-at-hard-turn leave along f'
//   with a generous loop, matching rest parametric speed; less cusp at the
//   box. Chord/3: the bezier hugs the chord, under-steers the facing, and
//   can cusp or flatten the ends. Identity of Morph at rest is independent
//   of this (skip V=U). Toggle only for A/B; ADR picked rest-speed.
// ---------------------------------------------------------------------------
constexpr bool kHandleModeRestSpeed = true;

// ---------------------------------------------------------------------------
// kCentreVsBoundary — centre vs boundary-ink attach
// Default: 1.0
// Unit: dimensionless ratio. Centre bind when d(center) < ratio · d(boundary ink).
// Direction: higher = more ends classified centre (looser “near the middle”).
//   Lower = stricter; more ends stay on the drawn boundary point.
// Visual: Does not move the endpoint. Centre only drops drawn leave and faces
//   the counterpart. Boundary keeps the pen-up point and the drawn leave in
//   the node frame. Never snap to the fitted AABB.
// ---------------------------------------------------------------------------
constexpr double kCentreVsBoundary = 1.0;

// ---------------------------------------------------------------------------
// kMinInkRadiusWorld — documented bend floor (not a live clamp)
// Default: 12.0
// Unit: world u (minimum radius the warp is expected to express).
//   Scoreability floor in EXP was 2 × this (~24 u connector length).
// Direction: higher = coarser / less allowed tightness (short connectors are
//   “below the bar”). Lower = finer (tighter bends are considered in-spec).
// Visual: Does not clamp scale or mix in v1 (ADR: no scale floor). It is the
//   comment/metric for “this cubic cannot hold a 12 u radius below ~24 u
//   length; ~70 u still holds at 45° rotate, 30 u does not.” Raising it only
//   changes how we talk about short-connector failure; lowering it does not
//   magically make a tiny cubic look like handwriting.
// ---------------------------------------------------------------------------
constexpr double kMinInkRadiusWorld = 12.0;

} // namespace document
} // namespace epaper
