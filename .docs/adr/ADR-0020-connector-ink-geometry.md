---
id: ADR-0020
title: Connector-ink geometry: rest shape, cubic and morph warps
status: accepted
date: 2026-08-14
deciders: [architect, pm, human]
supersedes: null
amends: [ADR-0010]
source: BS-0001 D32 / EXP-0002 R1–R5
---

# ADR-0020 — Connector-ink geometry: rest shape, cubic and morph warps

## Context

A connector whose body is the creator's own ink must stay attached when either bound
SmartGroup moves, resizes, or rotates. Two quality goals compete:

| Goal | Why it matters |
|---|---|
| **Natural ink** | The campaign exists to keep handwriting, not to replace it with a routed path |
| **Smooth under a new facing** | A node rotation introduces a turn the drawing never had; a 12 u bend sitting next to a 400 u drawn curve looks broken |
| **Mirror parity** ([ADR-0015](./ADR-0015-one-way-sync-contract.md)) | Device owns the working document; Infini must derive the same pixels from the same ops. The warp must be a **pure function** of rest shape + endpoints — no `dt`, no solver state |
| **Zero extra ops on move** | A node move emits only `set_smart_transform`; the connector is derived ([BS-0001](../../.plan/iter-004/brainstorms/BS-0001-auto-connector-ink.md) D4) |

[EXP-0002](../../.plan/iter-004/explorations/EXP-0002-connector-ink-warp.md) measured three
warps on a host C++17 probe (20-case set, two rest shapes). Local G1 end-blend preserves ink
and kinks at the ends. Always-cubic is globally smooth and redraws the line on recognition
(~10 u mean at rest, 85% of samples moved >1 u). Rest→cubic morph is Local at rest and Cubic
at a hard turn, with nothing in between that is both: at 15° of box rotation it has already
moved 71% of the line, because the cubic is already ~10 u from the rest spine *before anyone
moves*. No mix function dissolves that. Quadratic bezier is mathematically out (one interior
control cannot represent two independent facings).

The human's W1 verdict: **keep Always-cubic and Morph as two stored styles** on the connector.
Local end-blend is not a creator-facing option.

This ADR also amends [ADR-0010](./ADR-0010-tree-of-vectors.md) §6: a `centre` anchor kind is
now in force, and an edge anchor stores a **drawn departure** in the edge frame rather than
snapping to preferred ports as the live facing.

Evidence lives in the throwaway worktree `exp/connector-ink-warp`
(`~/.cursor/worktrees/exp0002-warp-a3f19c2d/reawa-6bcab1d3a018`). Production re-implements
this spec; it does not copy the probe.

## Decision

A `Connector` is the existing kind ([ADR-0010](./ADR-0010-tree-of-vectors.md)), extended with
an ink **body** (raw strokes as children, draw order) plus a derived **rest shape**. It is
still not a spatial parent of other document nodes. Visible geometry is a **pure function**
of `{rest shape, two endpoint states, warp style}`.

### 1. Rest shape — once, at recognition, never rewritten

From the concatenated raw polyline `P`, in draw order, no joining segments inserted:

1. Resample `P` at **2.0 u** arc-length spacing.
2. Smooth with repeated binomial `[1,2,1]/4` equivalent to Gaussian **σ = 6.0 u**
   (`round(2·σ²/spacing²) = 18` passes). Pin the first and last samples. Resample at 2.0 u.
   This is the **rest spine** `S`.
3. For every raw point `p ∈ P`, store `(s, d)`: `s` = normalized arc-length of the nearest
   point on `S`; `d` = signed perpendicular offset in **world units** (never scaled later).
4. Store the rest chord and, at each end, the **drawn departure** taken from `S` (first
   segment tangent; last segment tangent negated so both point outward), expressed in that
   end's anchor frame (below).

Invariant **I1**: the rest shape is never rebuilt from a warped result. Measured: 200
successive poses returning to start reproduce the first output to 0.000000000 u under this
rule, and drift tens of units without it.

The `(s, d)` projection itself loses ~0.07 u on a wiggly line. That is representation
fidelity, an order of magnitude below ink width. Identity claims are against the
**rest-shape reconstruction**, not the raw ink. Closing the 0.07 u (a tangential component
per sample) is a named v2 path, not taken.

### 2. Anchors — two kinds, facing is drawn departure

Kind is chosen at recognition and only ever changed by the creator. v1 targets:
**SmartGroup only**.

**Edge.** Stores `(edge, t ∈ [0,1], drawnEdgeLocal)`. The edge is **never re-selected**.

- Point: `corner[edge] + (corner[(edge+1) mod 4] − corner[edge]) · t`.
- Frame `(n, e)`: `n` is the current outward normal of that edge; `e = leftNormal(n)`.
- At recognition, `drawnEdgeLocal = (drawn · n, drawn · e)`.
- Facing on every later change: `drawnEdgeLocal.x · n + drawnEdgeLocal.y · e`.
- A U-turn after the box passes its peer is accepted behaviour. The creator's remedy is
  switching that end to centre. The system never does this silently.

**Centre.** Stores no edge.

- Point: box centre.
- Facing: drawn departure, **clamped to a 60° cone** about the peer-centre ray. The cone
  angle is load-bearing (90° overshoots 12–14 times on the probe). The raw peer ray sits
  8–23° off the ink and would move it ~2.5 u at rest.
- Clip the warped samples at the box boundary (`Avoid`). **Keep the whole rest shape**;
  the hidden ink is what re-clips correctly on a later resize. Do not trim to the visible
  span.

Perpendicular leave (edge normal as facing) is **abandoned**. EXP-0002 R3 measured it at
7.8 u of the creator's ink, moved on a connector nobody had touched.

### 3. Two warp styles, stored on the connector

| Style | Code | What the creator sees |
|---|---|---|
| **Morph** | `warpStyle: morph` (**Ink**) | Your ink at rest. As the required turn grows, the body straightens toward the cubic that already satisfies both facings. At a hard turn it *is* that cubic. **Auto-picked** when the rest spine is wiggly. |
| **Cubic** | `warpStyle: cubic` (**Curve**) | A cubic bezier through the two endpoints with the two facings, from the moment of recognition. Wiggle survives only as `d` around that cubic. Recognition visibly tidies the line (~10 u mean). **Auto-picked** when the rest spine is smooth. |

These are **deformation spines**, not the `ml-mindmap` routing-style picker (squared /
rounded / orthogonal bezier). That picker, and obstacle-aware matrix routing, stay out of
v1. Quadratic is not a style.

The style is a persisted field on the connector. At recognition it is **chosen from the stroke**, not left as a single default:

- **Smooth** rest spine (low-frequency: at most one inflection on `S`) → `cubic`
- **Wiggly** rest spine (two or more inflections on `S`) → `morph`

The test is on the rest spine `S`, never rest-fit to `C`. `C` sits ~10 u from even a smooth arc at rest, so fit-error cannot tell the two apart. Inflection count is the separator the two EXP rest shapes were built to illustrate (arc = one bulge; wiggle = extra wave). The exact cutoff (≤1 vs ≥2) is the v1 rule; SRS may replace the instrument (e.g. high-frequency energy of `S`) if QA's corpus shows a better split, but not the *intent*.

The creator can override after the fact. Switching style does not rewrite the rest shape; it changes which function is evaluated. One undo. Changing style or end-kind is **selection chrome** on a selected connector (two-state style control; per-end Edge/Centre), not a ToolChip tool and not a draw-time toggle.

**Local G1 end-blend is not a stored style.** It remains a measured control in EXP-0002.
Morph at mix 0 *is* the unblended similarity warp; it does not include a tangent blend at
the ends.

### 4. The two functions

Shared prelude, every endpoint change:

1. Resolve both anchors → `P0'`, `P1'`, `f0'`, `f1'` (outward unit facings).
2. Similarity-warp the rest spine: `scale = |c'|/|c|`, `theta` from rest chord to new chord,
   `U[i] = P0' + Rot(theta) · (scale · (S[i] − S[0]))`. No scale floor, no clamp.
   `L'` = arc length of `U`.
3. Build cubic `C` sampled at the same normalized-arc parameter as `U`:
   - Handle speed at each end = `L'` (**rest-spine end-speed**, not chord/3; chord/3 as
     Hermite speed under-steers and cusps).
   - `C(s) = Hermite(P0', f0'·L', P1', −f1'·L', s)` over `s ∈ [0,1]`.
   - Pin `C[0] = P0'`, `C[last] = P1'`.

**Cubic style.** Warped spine `V = C`. Re-place each `(s, d)` using `C`'s geometry at
parameter `s`.

**Morph style.**

- Turn per end = angle between that end's facing and `U`'s tangent at that end.
- `turn = max(turn0, turn1)` in degrees.
- `m = 0.5 · (1 − cos(π · min(1, turn / 90)))` (versine, saturates at 90°). `m(0) = 0`
  exactly and `m'(0) = 0`, so a 2° drag does not pop.
- If `m = 0`, `V = U` by a **true skip**, not a zero-parameter mix. This is identity
  against the rest-shape reconstruction (bitwise on the probe).
- Otherwise `V[i] = (1−m)·U[i] + m·C[i]` (world-space lerp). Pin endpoints to `P0'`, `P1'`.
  At `m = 1`, `V` is exactly `C`.
- Re-place `(s, d)` by locating `s` on `U` and reading position + tangent from `V` at that
  same segment (so `m = 0` cannot slide ink). `d` never scaled.
- Clip centre-bound ends.

`turnRoomFactor`, `blendCap`, and `departureStubRatio` belong to the unshipped Local blend.
They are not tunables of this ADR.

### 5. Invariants

| Id | Rule | Probe |
|---|---|---|
| I1 | Never re-bake the rest shape from a warped result | 0.000000000 u round-trip vs 48–60 u if re-baked |
| I2 | Pure function of rest shape + endpoints + style | Byte-identical across repeats and reversed case order |
| I3 | Morph identity at rest | Bitwise `V = U` at `m = 0` |
| I4 | Drawn attachment is sacred | Edge never re-selected |
| I5 | Analytic facing on `C` | Cubic (and Morph at `m = 1`) leave exactly along `f'`. Do not test with a flat ±5° secant — it conflicts with any finite radius |

Cost: a 500-sample re-warp is ~6–8 µs p95 on the host. Device budget is dominated by e-ink
refresh, not CPU. Warp runs **live during the drag** of a bound node (human R10), on a
**partial-refresh path**: damage the connector's old∪new AABB each frame; never a full-panel
invalidation; never a Pen blit of the whole document. Same refresh class as live lasso
([ADR-0019](./ADR-0019-selection-chrome-layers.md) ToolCanvasLayer / CanvasLayer live ink).
A missed frame degrades to the last warped pose, then a commit warp on pen-up.

### 6. Known limits, stated not hidden

1. **Cubic redraws the line on recognition** (~10 u mean / 19 u max on the arc rest shape).
   That is the style, not a defect.
2. **Morph spends most of the line as soon as it starts looking like a cubic.** At 15° of
   box rotation, versine `m ≈ 0.11` already moves 71% of samples >1 u. There is no mix that
   is faithful at a nudge *and* cubic at a hard turn. Creators who want both looks use both
   styles, not a sharper `m`.
3. **A cubic S-curves through ~90–105° of aligned rotation** with 0 backtrack (Local could
   not). Past ~120°, and on a chord-flip, it loops. Centre bind remains the remedy for a
   genuine U-turn.
4. **Short connectors.** The 12 u radius bar is not expressible below `L' ≈ 24 u`. A cubic
   lowers the practical floor (~70 u at 45° rotation still holds; 30 u does not) but does
   not remove it.
5. **Interior pins** (a connector drawn to detour around a third box) are not built. The
   detour rides the warp. Named v2 (BS-0001 D12).
6. **v1 targets SmartGroup only.**
7. **Delete keeps the connector** (D39). A missing bound node does not mark the connector
   `invalid` and does not delete it. That end keeps its `NodeAnchor` (same `nodeId`); warp
   uses the end's **last live world pose** (point + facing), a derived cache persisted with
   the connector so a reload after delete still draws. Updating the cache is not an op —
   same class as a cached render path. Undo of the delete restores the SmartGroup with the
   same id; the end resolves live again and glues back. If both bound nodes are gone, the
   connector remains a selectable stroke between two frozen poses.

## Consequences

- `create_connector` must carry body strokes, rest shape (or enough to derive it once), both
  anchors including kind / edge / `t` / `drawnEdgeLocal`, `warpStyle`, and a reserved
  `terminal` slot per end. Device becomes an author of this op ([SRS-EP-07](../modules/epaper/features/vector-document/srs-logic.md)
  amendment).
- A node `set_smart_transform` does **not** emit connector ops. Both ends re-derive.
- Shared fixtures (device + desktop) are the parity test, same pattern as ink-box geometry.
- [ADR-0010](./ADR-0010-tree-of-vectors.md) §6's "centre is a later kind" and port-snap-as-facing
  are amended by this document. Preferred-port *snap at recognition* may still bias `t`; it
  does not define facing.
- Follow-up ADRs, not this one: ToolChip 3+2 (supersede [ADR-0017](./ADR-0017-four-tool-chip.md));
  closure-first recognizer dispatch.
- Creator-facing names: **Ink** / **Curve**. Selection chrome and recognition blink are
  `/designer`. Delete-keeps-connector (D39) retires `SRS-IN-04` “mark invalid” when REQ-09
  is minted — do not silently rewrite that SRS before the REQ.
- Spike worktree kept as the numeric reference; production is a docs-first reimplementation.

Trade-off point this pins: **global smoothness vs identity at rest** is no longer a single
algorithm. It is a per-connector style. The mix function is not a third way.

## Alternatives Considered

| Approach | Natural ink | Smooth hard-turn | Identity at rest | Mirror/determinism | Why |
|---|---|---|---|---|---|
| Status quo (no on-device connector) | n/a | n/a | n/a | + | Rejected — this is the campaign |
| Routed path, ink as decoration | − | + | − | + | Rejected for v1 — abandons the drawing |
| Live physics rope (EH2) | − | + | − | − | Rejected — not a pure function; extra ops or divergent mirrors; e-ink cost |
| Local G1 end-blend (EXP-0002 R4) | + | − | + | + | Rejected as a *style* — kink at the ends is the conflict the human named; kept as measured control |
| G2 / quintic local blend | + | − | + | + | Not built — still leaves the middle sacred, so a new facing still fights the drawn bulge |
| Always-cubic only | − (~10 u at rest) | + | − | + | Rejected as the *only* style — forbids identity; kept as the `cubic` option |
| Morph only | + at rest, − in between | + at high turn | + | + | Rejected as the *only* style — 71% of the line already spent at 15°; kept as the `morph` default |
| **Both styles, stored (this ADR)** | per style | per style | Morph yes, Cubic no | + | Winner — the W1 call |
| Quadratic "bezier 2" | − | − | − | + | Impossible — one control cannot hold two facings |
| Interior pins (approach D) | + for detours | 0 | + | + | Deferred v2 |

Sensitivity: **rest-fit of `C` vs `U`** (~10 u at rest) drives Morph's ink spend. No
constant in `m(turn)` moves that number. Trade-off: two styles vs one algorithm that
pretends to be both.
