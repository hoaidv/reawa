---
id: BS-0001
topic: Automatic recognition of hand-drawn connectors between ink-boxes, and how they deform when a node moves
topic_source: chat 2026-08-14 (human, via /pm) — "Connector tool on device" priority above REQ-08 / CHL-0011 / CHL-0012
date: 2026-08-14
facilitator: pm
participants: [pm, architect, designer, sm, dev, qa]
status: concluded
mode: keep-going          # human: "Keep brainstorming" — batched R2…R5, then decision rounds R6/R7
rounds_planned: open      # ran 7
relates-to: [REQ-05, REQ-06, REQ-08, SRS-EP-07, SRS-EP-08, SRS-EP-10, SRS-EP-11, SRS-IN-04, SRS-IN-09, ADR-0010, ADR-0011, ADR-0015, ADR-0017, CHL-0011, CHL-0012]
---

# BS-0001 — Auto-recognized hand-drawn connectors (connector-ink)

## Topic (from human)

Verbatim intent (chat, 2026-08-14):

> Keep brainstorming. I want to make the drawing on tablet as natural as possible. That's why
> adopted the feature semi-automatic "ink-box". Keep going this way. I want a feature to
> automatically detect connectors.

- **UX 1 — single polyline connector.** Creator makes ink-box A, B, C; then draws one polyline from
  a point near A to a point inside C. System recognizes the polyline as a connector: registers the
  new ink as a connector with raw inks, and recognizes 2 endpoints with proper attributes on A and C
  (node, anchor type, anchor position — as in the `ml-mindmap` model). Endpoint *shapes* (arrowheads)
  are detected from other inks near the polyline — define later.
- **UX 2 — multiple polylines connector.** Creator draws X1 from near A into void; X2 continuing
  from X1's end into void; X3 continuing from X2's end to C. System recognizes the 3 polylines as one
  connector A→C: 3 inks merged into 1 polyline, registered as a connector with raw inks, endpoints
  bound to A and C.
- **UX 3 — moving a node that has connectors.** System must adjust the hand-drawn connector smoothly.
  A pre-defined routed connector is easy; preserving the *natural* drawing is the hard path.
  Two escape hatches offered by the human:
  - **EH1** — if the connector is mostly smooth (bezier-like), find an algorithm that deforms the raw
    inks onto a transparent pre-defined bezier (2/3); on move/resize/rotate recompute the bezier and
    deform the raw inks along it. Partially preserves natural ink; document stays dynamic, not a
    raster.
  - **EH2** — if the connector is wiggly, treat it as a floating rope on water; pulling one end
    gradually stretches the rope (physics). Does not preserve natural ink but feels smooth. Rope
    properties: attach naturally to A and C (perpendicular to face/edge, or ray through centre); no
    sharp corners anywhere on the rope.

Prior round (R1) in the same chat asked for connector options: line style (single/double/dotted),
width; routing style (straight/squared/bezier 2,3); and whether an end binds perpendicular to a node
edge or to the node centre.

### Definition of enough (checklist — tick at conclusion)

- [ ] A recognition rule for UX1 that cannot be satisfied by ordinary writing (guard ladder + arbitration order vs draw-into / enclose)
- [ ] A rule for UX2 chaining (which strokes join, in which direction, and what the intermediate document state is)
- [ ] One chosen deformation model for UX3 with a stated reason the alternative was rejected
- [ ] The document model change named (node kind / op / anchor fields) so `/architect` can write it
- [ ] Wire + mirror consequences stated (ops per move, divergence risk)
- [ ] Riskiest bet identified with a cheap validation route
- [ ] Product-rule conflicts with shipped `.docs/` surfaced (not silently overridden)

## Glossary

| Term | Definition |
|---|---|
| **connector-ink** | A `Connector` node whose visible body is the creator's own ink strokes, not a synthetic routed path |
| **body** | The raw `Ink` strokes the creator drew, kept as the connector's children in draw order |
| **spine** | The smooth centreline curve that parameterizes the connector; derived, never drawn |
| **rest shape** | The body as drawn, stored once in spine-relative coordinates; never re-derived from a deformed state |
| **(s, d)** | Per-sample storage: `s` = normalized arc-length position along the rest spine, `d` = signed perpendicular offset in world units |
| **warp** | Pure function `(rest shape, endpoint anchors) → world samples`. No time, no iteration state |
| **facing** | The outward direction an endpoint leaves its node (edge normal, or centre ray) — from `ml-mindmap` `NodeAnchor.facing` |
| **chain** | Several strokes recognized together as one connector body (UX2) |
| **dangling end** | A connector end bound to a free canvas point instead of a node (`PointAnchor`) |
| **terminal** | Arrowhead or decoration at an end, recognized from short strokes near that end |
| **recognizer chain** | The ordered set of pen-up recognizers (enclose · draw-into · connector) with a single verdict |

## Session history

### Round 1 — diverge · technique: option enumeration (chat, before this file)

- **R1-I1** (pm): Connectors deserve a **new REQ (REQ-09)**, ranked above REQ-08 / CHL-0011 / CHL-0012 — REQ-08's connector row is *re-anchor / routing on an existing connector*, which is a different job from *create one*.
- **R1-I2** (architect): Attachment options — edge+facing (perpendicular leave), centre, loose point, interior free point. `ADR-0010` §6 already reserves boundary attachment and names "centre" as a possible later kind. [R1-I2]
- **R1-I3** (architect): Routing options from `ml-mindmap` — `Straight | Squared | Rounded | Bezier`, where **bezier 2/3 is not user-chosen control points** but a presentation of a 2- or 3-vertex orthogonal route (V-corner vs Z-corner). Obstacle-aware matrix routing exists there but is heavy. [R1-I3]
- **R1-I4** (designer): RM2 has no modifier keys, and `ADR-0017` locks the ToolChip at **four** exclusive tools — so `ml-mindmap`'s Shift/Option/Cmd style switching cannot port. Style must be a node property chosen at commit.
- **R1-I5** (pm): Recommended v1 = create + delete, edge bind with perpendicular leave, straight path, re-resolve on move; centre bind and dashed/width as Should; obstacle routing and loose ends as Won't.
- **Feedback gate**: paused → human redirected: **do not build a manual connector tool**. Recognition must be automatic (semi-automatic like ink-box), and the hard problem is UX3 deformation. R1-I5's "Won't: loose ends" is **contested** by UX2 (see R2-I6).

### Round 2 — diverge · technique: cross-domain analogy + first principles (recognition, UX1/UX2)

- **R2-I1** (analyst): The closest shipped analogue is **MyScript Nebo / Interactive Ink diagram mode**: draw a shape connector between two shapes to link them; once linked, moving or resizing one moves the connector accordingly; **"draw two shape connectors close to each other to link them"** — i.e. UX2 chaining is already a shipped behaviour in a commercial pen product, not a novel bet. Nebo also links a connector to the *shape*, not to the text inside it — which maps exactly onto our SmartGroup vs `role: content` ink. [R2-I1]
- **R2-I2** (architect): The academic core rule is trivially simple and matches UX1: **"if the end points of an edge fall within two different node shapes, those nodes are connected"** (InkKit connector semantics). Recognition is endpoint-overlap, not shape classification. [R2-I2]
- **R2-I3** (qa): Guard ladder for `pen`-armed pen-up, in the style of the existing enclose guards (`SRS-EP-10`):
  1. **Open** — first and last sample far apart relative to path length (a closed stroke is not a connector).
  2. **Path-like** — low self-intersection count and few direction reversals; hull area small relative to length² (rejects scribbles and letters).
  3. **Long enough** — arc length ≥ `MIN_CONNECTOR_WORLD` (start from the enclose precedent, 48 world units).
  4. **Two bindings** — first sample within `R_SNAP` of node A's world `bounds`, last sample within `R_SNAP` of node B's `bounds`, and **A ≠ B**.
  5. **Body mostly outside** — ≤20% of samples inside A, ≤20% inside B, ≥60% outside every box (this is what separates a connector from draw-into content and from an underline).
- **R2-I4** (dev): Precedence must be explicit or we regress EP-017. Proposed **recognizer chain** at pen-up: `ink_box armed → enclose` else `pen armed → draw-into membership → connector → ordinary ink`. Draw-into is evaluated **first** because it is shipped behaviour and its ≥80%-inside test is nearly disjoint from guard 5 anyway; ties must never flip an existing outcome.
- **R2-I5** (dev): Reuse the `[enclose]` log-token pattern — one `qInfo` `[connector] outcome=created|stayed_ink guard=<reason> …` line per pen-up. Costs nothing, makes the recognizer inspectable in the Device Log panel already shipped (IN-029 / EP-021).
- **R2-I6** (architect): UX2 needs a decision about the **intermediate** state after X1. Two shapes:
  - **(a) Pending half-connector** — X1 immediately becomes a connector with one `NodeAnchor` and one `PointAnchor` (a dangling end). Matches `ml-mindmap`'s `PointAnchor`; gives live feedback; but creates a document node the creator never asked for, needs a timeout/abandon policy, and publishes an op that may be retracted.
  - **(b) Retro-chaining at completion** — X1, X2 stay ordinary ink. When a stroke *lands on a second node*, walk back through recent top-level ink for a chain that is endpoint-adjacent (within `R_JOIN`) and tangent-continuous (turn ≤ ~60°), in either draw direction; if the far end of the chain binds a different node, merge the whole chain into **one** connector in **one** op with **one** undo entry.
- **R2-I7** (pm): Prefer **(b)** — no speculative nodes, no timeout policy, no retraction on the wire, and undo semantics stay "one gesture, one entry". Bound the search (last N top-level strokes, N≈8; skip ink already parented to a SmartGroup) so cost is predictable. Give the creator feedback with **chrome only** (a small live tick on a free end that currently binds a node) — view state, not document state.
- **R2-I8** (architect): Merge semantics — the connector keeps the raw strokes as `body` children in draw order and derives one spine polyline. This satisfies the human's "3 inks merged into 1 polyline… register new ink as connector with raw inks" while staying reversible: one undo and they are three plain inks again.
- **R2-I9** (qa): Terminals (arrowheads) have a proven algorithm to borrow later — InkKit: the **longest stroke is the shaft**, shorter nearby strokes are the head, and direction is decided by which shaft end they sit closer to; Tahuti refines it geometrically (two furthest points = shaft ends; points furthest from the shaft line on either side = head ends). Park for v1.1 but **reserve a `terminal` slot per end now** so it is additive. [R2-I9]
- **R2-I10** (designer): Existing guards already protect against the obvious collisions: enclose captures **free top-level Ink only**, so a `Connector` node is not capturable content; and `CHL-0011` keeps SmartGroups out of enclose. No new exclusion rule needed for "enclose around a diagram".

### Round 3 — build & challenge · technique: assumption surfacing + steelman/devil's advocate (UX3 deformation)

- **R3-I1** (architect): **EH1 vs EH2 is a false binary.** Both are "a spine plus offsets". Decompose into two independent axes:
  - **Axis A — what the spine is:** (A1) analytic curve fitted to the ink (bezier 2/3); (A2) a routed path computed from the anchors, ignoring what was drawn; (A3) the ink's own smoothed centreline, kept as the **rest shape**.
  - **Axis B — how the spine updates when an end moves:** (B1) recompute analytically from anchors; (B2) similarity warp of the rest spine driven by the two endpoints; (B3) elastic relaxation (rope / position-based dynamics); (B4) piecewise warp with interior pins.
  EH1 = **A1+B1**. EH2 = **A3+B3**. The unexplored sweet spot is **A3+B2**.
- **R3-I2** (architect): **A3+B2 concretely.** At recognition time, compute a smoothed centreline (rest spine) from the merged body and store every raw sample as `(s, d)` — normalized arc-length along the rest spine, and signed perpendicular offset in world units. On any endpoint change: (1) re-resolve both anchors from live node geometry (already an Infini invariant); (2) map the rest spine through the similarity that takes the old endpoint pair to the new one; (3) Hermite-blend the first and last ~15% of arc length so the departure tangents match each end's required `facing`; (4) re-place each sample at `spine(s) + d · normal(s)`.
- **R3-I3** (architect): This is a known, published construction, not an invention: spine-driven deformation with **global arc-length parameterization** plus **normal offsets**, with **Hermite interpolation of position *and tangent* at the ends** — see `Bender` (Rossignac et al.) and `Fleshing`. `Bender` also warns to use a *global* arc-length parameterization rather than per-segment, or stretching becomes visibly non-uniform. [R3-I3]
- **R3-I4** (architect): **The wiggle objection dissolves.** EH1 was scoped to "mostly smooth" connectors because a bezier *fit* discards wiggle. With `(s, d)` the wiggle is not fitted — it is stored as offsets and reproduced exactly. So A3+B2 handles the wiggly case **and** the smooth case with one code path. The analytic fit (A1) is then only needed if we later want *routing* (elbows, obstacle avoidance) — i.e. the fit is optional, the offsets are mandatory.
- **R3-I5** (qa): **The decisive argument is sync determinism.** `REQ-07` / `ADR-0015` commit us to a one-way op stream with "0 divergent figures" on the mirror. A warp is a **pure function** of (rest shape, endpoints) — Infini recomputes it identically from the same `create_connector` + `set_smart_transform` ops. A physics rope is iterative and state/time-dependent: either the mirror diverges, or we must stream the resulting sample set on every move (bandwidth, and it breaks the p95 ≤300 ms mirror bar). Physics also makes "committed geometry = last previewed geometry" unverifiable, which is the exact regression bar (`BR-N03`) that CHL-0004…0007 died on.
- **R3-I6** (dev): **Cost profile.** A3+B2 is O(n) per endpoint change with no solver and no iteration; it can run **once at move-commit** rather than per drag frame. During the drag, translate the last committed connector rigidly (or show a straight rubber-band) and re-warp at pen-up — which satisfies `BR-N06` (no full-panel invalidation during a gesture; slow is acceptable, wrong is not).
- **R3-I7** (dev): **Op cost is zero.** Connector geometry is *derived*, so moving A publishes only the existing `set_smart_transform` on A. No connector op per move, no extra undo entry, no queue growth. That is a strictly better wire story than any approach that re-publishes connector geometry.
- **R3-I8** (qa): **Invariant — never re-bake.** Always warp from the stored rest shape; never recompute the rest shape from a warped result. Re-baking accumulates distortion over repeated moves until the line is mush — this is the drift class of bug, and it is prevented by a one-line rule.
- **R3-I9** (designer): Two product-visible choices inside B2: (i) keep `d` **absolute** so wiggle amplitude stays constant when the connector is stretched (recommended — a stretched drawn line should not grow fatter waves), while `s` is normalized so drawn detail spreads along the new length; (ii) never mirror — preserve the sign of `d` relative to travel direction, so a bulge stays on the same side of the line.
- **R3-I10** (pm, devil's advocate for EH2): Steelmanned, the rope wins in two places — very **large** moves (a similarity warp drags a deliberate detour around box B along with it, which a rope would relax out of the way) and the "no sharp corners" property, which a rope gets for free from bending constraints. Honest verdict: **reject for v1**, but the escape is not "never" — a **fixed-iteration, dt-free** PBD pass *is* a pure function of the endpoints and would inherit the determinism property. So a future `rope` character class is admissible if specified as N fixed iterations in a fixed order with no time step. Large-detour preservation gets its own answer: **B4 interior pins** at high-curvature vertices, piecewise-warped.
- **R3-I11** (architect): Degenerate cases to specify, not discover: endpoints collapsing to near-zero separation (clamp the similarity scale, keep the last good spine); A moving to the far side of C (chord angle flips — rotate by chord delta, do not mirror); a node deleted (existing rule: connector marked `invalid`, never dangling); resize with `fixedInk` (anchors resolve against **geometric bounds** only, per `ADR-0011` — the freehand boundary path is never the anchor target).

### Round 4 — build & challenge · technique: pre-mortem ("18 months on, why did this fail?")

- **R4-I1** (qa): *Every stray line became a connector*; creators turned the feature off. → Guard ladder R2-I3, one-undo revert, and a tuning pass on a real ink corpus before ship.
- **R4-I2** (designer): *The first big move looked wrong* and creators stopped trusting the document. → Prototype the warp on the host harness and get a human verdict **before** committing the campaign; define the acceptable-distortion bar up front.
- **R4-I3** (dev): *Drift* — the rest shape was re-baked on each move until the connector was mush. → R3-I8 invariant, fixture-verified over a 20-move sequence.
- **R4-I4** (architect): *The mirror diverged* because the device and Infini implemented the warp with slightly different sampling or float order. → One canonical algorithm section plus **shared fixtures** (the mechanism already used for ink-box geometry agreement).
- **R4-I5** (sm): *Scope creep into routing* (squared / rounded / obstacle avoidance) ate the campaign. → Routing is explicitly **Won't** for v1; the warp needs no router.
- **R4-I6** (pm): *The unprompted-creation rule was violated quietly* and Smart Group behaviour drifted with it. → Surface the conflict now (see D2 / ASR-4), decide it as product, do not let a recognizer decide it.
- **R4-I7** (dev): *e-ink died* because the connector re-warped on every drag frame. → Warp at commit; rigid translate during the drag.

### Round 5 — converge · technique: weighted rank + MoSCoW (see Decision Log, prioritisation, ASRs below)

- **Feedback gate**: paused → human answered all five (see Round 6).

### Round 6 — converge · technique: human decision + consequence tracing

Human answers to the five gate questions (2026-08-14):

1. **Unprompted creation** — *"Keep 'never unprompted'. Add a fifth ToolChip tool AS A TOGGLE. We still have the pen tool selected. This new toggle the 'Connector recognition' alg. Also convert the 'Ink-box' button to a toggle. It toggles the 'Ink-box recognition' alg."*
2. **Binding** — edge-first with perpendicular leave; centre available.
3. **Centre presentation** — clip at the box boundary (`Avoid`).
4. **v1 targets** — SmartGroup only.
5. **Validation** — run the warp EXP on the host harness **before** locking the campaign.

- **R6-I1** (architect): The toggle answer is a **model change, not a button change**. Today the ToolChip conflates two different things in one exclusive row: *what the pen does* (draw vs select) and *which recognizer runs at pen-up* (`ink_box` is really "enclose recognition on"). The new model separates them cleanly:
  - **Tools (exclusive, 3):** `sel_rect` · `sel_freeform` · `pen`
  - **Recognizer toggles (independent, 2):** ink-box recognition · connector recognition
  This is a better ontology and it directly serves the "as natural as possible" goal — the creator never leaves Pen to get recognition, and Pen with both toggles off is **pure paper**.
- **R6-I2** (pm): **BR-B02 is upheld** — creation still requires an armed recognizer, so nothing is created unprompted. But the *strength* of the prompt drops: an exclusive `ink_box` mode had to be entered and left, so intent was renewed per stroke; a toggle persists across strokes, so the creator can be writing text with a recognizer still armed. Conclusion: guards matter **more** under the toggle model, not less. The load-bearing guards against text remain "two bindings on different nodes" and "body mostly outside every box".
- **R6-I3** (architect): **New state that is impossible today** — both recognizers armed on the same stroke. Enclose wants closed/near-closed; connector wants open with two bindings, so they are nearly disjoint, but "nearly" is not a spec. Proposed dispatch: classify the stroke's **closure** first, then run exactly one recognizer, rather than a linear chain of three.
- **R6-I4** (qa): **A shipped behaviour is now in question.** `SRS-EP-10` says draw-into membership "**never** runs on an enclose stroke", and a failed enclose "stays ordinary ink". Under the toggle model the tool is always `pen`, so it is no longer obvious that a failed enclose should be excluded from membership. Either answer is defensible; silently changing it would regress EP-016/EP-017, which carry human PASS. **Needs an explicit decision** (Q6).
- **R6-I5** (dev): Latching extends unchanged — the **tool and both toggle states latch at pen-down**, so flipping a toggle mid-stroke cannot retroactively change what the stroke meant. Same rule, wider tuple.
- **R6-I6** (designer): The chip is shipped chrome (UI-EP-02 → EP-023 four-tool rebase → EP-025 ToolCanvasLayer, all human-PASSed). Turning one exclusive tile into a toggle and adding a second changes inventory, hit targets, and the active-state indicator under partial refresh. This needs a **design story**, and the campaign needs an explicit do-not-regress list for EP-016…019 / EP-025.
- **R6-I7** (designer): Toggles are **not** pen sub-modes. `REQ-03` currently says "There is no separate sub-mode strip"; the amendment must use recognizer vocabulary (`recog.ink_box`, `recog.connector`) so the sub-mode ban stays intact in meaning.
- **R6-I8** (dev): No wire impact. `REQ-03` already makes tool state device-local; toggle state is equally device-local and never published.
- **R6-I9** (pm): Defaults matter for migration. Both toggles **off** at launch reproduces today's default exactly (Pen draws ordinary ink), at the cost of discoverability — flagged as Q7.
- **Feedback gate**: paused → human answered Q6–Q9 (see Round 7).

### Round 7 — converge · technique: human decision + consequence tracing

Human answers (2026-08-14): **Q6** failed enclose **falls through** to draw-into membership ·
**Q7** both recognizers **on** at launch · **Q8** inert toggles are **dimmed** ·
**Q9** vocabulary is **"Ink-box recognition" / "Connector recognition"**.

- **R7-I1** (pm): Shipping both recognizers **armed by default** retires a shipped `BR-B02` clause outright — *"A rectangle drawn in `Pen` mode is ordinary ink, forever."* Under the new default, a first-run creator's first rectangle around ink **becomes a box** without them arming anything. "Never unprompted" now means *"never without an armed recognizer, and recognizers ship armed"* — which is a materially weaker promise than the rule the ink-box campaign was verified against. This is the human's call and it is the intended magic; it must be **recorded as a supersession**, not absorbed silently.
- **R7-I2** (qa): Direct consequence — the false-positive rate stops being a metric and becomes a **ship gate**. For a default-on recognizer, guards plus the one-undo revert plus a visible "recognized" indicator are the *only* protection a creator has. The number must be agreed before ship, and the corpus must include the **first 20 strokes of a fresh page**, because that is the highest-exposure moment.
- **R7-I3** (qa): Fall-through retires the `SRS-EP-10` clause *"[draw-into membership] **Never** runs on an enclose stroke"*. New scenario required: a rectangle that fails the enclose guards **inside** an existing box becomes that box's `role: content` ink — as **one** op with **one** undo entry. EP-016 / EP-017 must be **re-verified** against the new pipeline, not assumed to hold.
- **R7-I4** (architect): D20 and D7 reconcile into one pipeline. Classify **closure** first, then fall through in a fixed order: closed-ish **and** ink-box armed → enclose; enclose guards fail → **draw-into membership**; open **and** connector armed → connector; otherwise ordinary ink. One verdict per pen-up, one `[recog]` log line.
- **R7-I5** (designer): The chip must now carry **two visual languages at once** — exclusive tool selection *and* independent armed/dimmed toggle state — and both must stay legible during a trailing partial refresh. That is the real design risk in ASR-7, more than the extra tile.
- **R7-I6** (dev): Dimmed-but-armed is state that survives tool switches, so the toggle tuple is part of device-local tool state (`SRS-EP-04`) and must latch at pen-down per D15 regardless of which tool is active.

## Research & sources

- [R2-I1] MyScript Interactive Ink — *Shapes and connectors* (developer.myscript.com, Diagram features): "Once two items are linked, moving/resizing one means moving/resizing the other accordingly"; "Draw a shape connector between two shapes to link them"; "Draw two shape connectors close to each other to link them"; connector links to the shape, not the text inside it. (**analog** — commercial pen product, validates both UX1 and UX2 chaining as shippable)
- [R2-I1b] Forbes review of Nebo (2016): hand-drawn connecting lines "attach to their endpoints but can be edited separately. If a figure with attached lines is moved, the lines move with it." (**analog**)
- [R2-I2] Plimmer & Freeman, *Connector Semantics for Sketched Diagram Recognition* (AUIC 2007) / *A Toolkit Approach to Sketched Diagram Recognition* (HCI 2007) — InkKit separates edges from nodes and records overlaps: "if the end points of an edge fall within two different node shapes, those nodes are connected". Connector + connection-point identification was moved **into the core recognizer** after proving it is domain-independent. (**fact**)
- [R2-I9] InkKit arrowhead heuristic (longest stroke = shaft; shorter nearby strokes = head; direction from which shaft end they are nearer) and Hammond & Davis, *Tahuti* (AAAI Sketch Understanding 2002) arrow algorithm (two furthest points = shaft; furthest points either side of the shaft line = head ends). (**fact**)
- [R3-I3] Rossignac et al., *Bender: A Virtual Ribbon for Deforming 3D Shapes* — spine/wire-driven warp using a coordinate frame at each point, Hermite interpolation of **position and tangent** at the ends, and an explicit finding that a **global** arc-length parameterization avoids the non-uniform stretching produced by per-segment parameterization. Also *Fleshing: Spine-driven Bending with Local Volume Preservation* (CGF 2013) — spine + arc-length parameter + radial/normal/binormal offsets, with the closest-projection arc-length invariant. (**fact** — 3D literature applied to our 2D case, which is the easy sub-case)
- [R1-I2/R1-I3] `~/Project/ml-mindmap` — `src/models/Connector.ts` (`ConnectorStyle`: `lineColor`, `lineWidth`, `lineDash`, `routingStyle`, `roundRadius`, `terminalOverlay: Avoid|Overlay`; `isMovable` only when neither end is a `NodeAnchor`), `src/models/Anchor.ts` (`NodeAnchor` with `facing`, `Proportional` vs `Relative`, plus `PointAnchor`), `src/modules/rendering/ShapeRenderingVisitor.ts` (per-style path construction; bezier chosen by **route vertex count**, V-corner vs Z-corner), `src/modules/rendering/MatrixRouting.ts` (obstacle matrix, support splits when facings oppose), `src/modules/drawingtools/ConnectorTool.tsx` (24px anchor snap; hover preview; modifier-driven style). (**fact** — human's own prior art)
- Repo constraints read this round: `.docs/adr/ADR-0010-tree-of-vectors.md` §6 (boundary attachment, preferred ports, centre named as a possible later kind), `ADR-0011` (connector targets resolve against geometric `bounds`), `ADR-0015` (one-way op stream), `ADR-0017` (four-tool ToolChip), `SRS-EP-07/08` (op set, gesture-commit, publish queue), `SRS-EP-10` (enclose guards, draw-into ≥80%, min 48 world units), `SRS-IN-04` (connector invariants 4/6/7), `SRS-IN-09` (`create_connector` envelope), `epaper/prd.md` Non-Goals (connectors deferred to REQ-08), `ink-box/srs-product.md` **BR-B02** (never created unprompted). (**fact**)

## Decision Log

| id | decision | status | supersedes | source |
|---|---|---|---|---|
| D1 | Connector-ink is the existing `Connector` kind extended with an ink **body** (raw strokes as children, draw order) plus a derived **spine** — not a new node kind, not a bespoke op family | decided | — | R2-I8, R3-I2 |
| D2 | Recognition is **automatic** at `pen` pen-up, guarded by the R2-I3 ladder, reversible by **one** undo, and announced by chrome — **no fifth ToolChip tool**. Conditional on PM resolving the BR-B02 conflict (ASR-4) | **superseded** by D13/D14 | R1-I5 (manual connector tool) | R2-I2, R2-I3, human directive |
| D3 | UX3 deformation = **rest shape in spine coordinates `(s, d)` + endpoint-driven similarity warp + Hermite tangent blend at the ends**. Live physics rope rejected for v1 | decided | EH1 (fit-only), EH2 (rope) | R3-I2…R3-I6, R3-I10 |
| D4 | Connector geometry is **derived**, never re-published on move: a node move emits only its own `set_smart_transform`; 0 extra ops, 0 extra undo entries | decided | — | R3-I7 |
| D5 | Rest shape is **never re-baked** from a warped result | decided | — | R3-I8 |
| D6 | UX2 uses **retro-chaining at completion** (no pending half-connector node); free-end feedback is chrome only | decided | R2-I6(a) | R2-I6, R2-I7 |
| D7 | Recognizer precedence at `pen` pen-up: **draw-into membership → connector → ordinary ink**; `ink_box` still goes to enclose. Never flip a shipped outcome | decided | — | R2-I4 |
| D8 | `d` is stored **absolute** (world units); `s` normalized; no mirroring of `d` | decided | — | R3-I9 |
| D9 | Anchor model gains `facing` and a `centre` kind; edge binds leave **perpendicular** to the edge, centre binds leave along the centre ray | decided | — | R1-I2, R3-I2, ADR-0010 §6 |
| D10 | Routing styles (squared / rounded / bezier presentation) and obstacle-aware matrix routing are **out** of v1 — the warp needs no router | decided | R1-I3 | R4-I5 |
| D11 | Terminals / arrowheads deferred to v1.1, but a `terminal` slot per end is reserved in the model now | deferred | — | R2-I9 |
| D12 | Interior **pins** (piecewise warp preserving a deliberate detour) and a deterministic fixed-iteration `rope` class are the named v2 upgrade paths | deferred | — | R3-I10 |
| D13 | **BR-B02 upheld** — nothing is created unprompted; recognition requires an armed recognizer | decided | D2 (auto in Pen mode) | human R6, R6-I2 |
| D14 | ToolChip becomes **3 exclusive tools** (`sel_rect` · `sel_freeform` · `pen`) **+ 2 independent recognizer toggles** (`recog.ink_box`, `recog.connector`). `ink_box` stops being an exclusive tool | decided | `ADR-0017` four-tile exclusive inventory | human R6, R6-I1 |
| D15 | Tool **and** both toggle states latch at pen-down for the whole stroke | decided | — | R6-I5 |
| D16 | Endpoint binding: **edge-first with perpendicular leave**; centre binding available as the alternative | decided | — | human R6 |
| D17 | A centre-bound end **clips the ink at the box boundary** (`Avoid`), so the drawn line never crosses content inside the box | decided | — | human R6 |
| D18 | v1 connector targets: **SmartGroup only** | decided | — | human R6 |
| D19 | The warp **EXP runs before the campaign lock**; PRD/SRS do not commit the deformation clause until the EXP verdict | decided | — | human R6 |
| D20 | Dispatch classifies stroke **closure first**, then falls through in fixed order to exactly one verdict: enclose → draw-into → connector → ordinary ink | decided | D7 (linear chain of three) | R6-I3, R7-I4 |
| D21 | A stroke that **fails the enclose guards falls through to draw-into membership** | decided | `SRS-EP-10` clause "membership never runs on an enclose stroke" | human R7, R7-I3 |
| D22 | Both recognizers **ship armed** (default on at launch) | decided | `BR-B02` clause "a rectangle drawn in Pen mode is ordinary ink, forever" | human R7, R7-I1 |
| D23 | Toggles are **dimmed** (armed state retained) while a Selection tool is active | decided | — | human R7 |
| D24 | Creator-facing vocabulary: **"Ink-box recognition"** and **"Connector recognition"** (`recog.ink_box`, `recog.connector`) | decided | — | human R7 |
| D25 | Because recognizers ship armed (D22), the **false-positive rate is a ship gate**, not a metric — a number agreed before ship, measured on a corpus that includes a fresh page's first 20 strokes | decided | — | R7-I2 |

## Assumptions & riskiest bets

| assumption | why critical | validation (EXP) | owner | by-when | status |
|---|---|---|---|---|---|
| The similarity warp + tangent blend **looks natural** on real moves | The entire feature rests on it; if it looks wrong, connectors feel broken and the document feels fake | **EXP** on the host harness: recognized connector + scripted moves/resizes/rotations, human verdict on a 20-move set before any device work | architect + designer | before campaign lock | **riskiest — open** |
| Guards reject ordinary writing at an acceptable rate | **Ship gate** (D25), not a metric: recognizers ship armed (D22), so a false positive eats the creator's stroke with no prior opt-in | **EXP**: replay a real ink corpus (text, underlines, ticks, brackets, arrows, **and a fresh page's first 20 strokes**) through the recognizer; measure false-positive rate per stroke and recall on intended connectors | qa | **blocks ship** | open |
| A default-armed recognizer is acceptable to a first-run creator | D22 retires the "ordinary ink, forever" promise; the first rectangle a new creator draws becomes a box | Human judgement on the EXP build; Nebo precedent is supportive [R2-I1]; one-undo revert + visible indicator are the only mitigations | pm | with the EXP verdict | open |
| Retro-chaining feels magical, not creepy | UX2's whole value; a wrong merge destroys three strokes at once | Same EXP, chain cases in both draw directions | designer | before ship | open |
| e-ink can present the re-warped connector fast enough to feel attached | Panel budget is the device-specific constraint (`BR-N06`) | Measure re-warp + partial refresh p95 after move commit on RM2 | dev | during build | open |
| Both ends produce bit-comparable warped geometry | `REQ-07` promises 0 divergent figures | Shared fixtures, as used for ink-box geometry | architect | at SRS | open |

## Goal prioritisation & metric tree

**v1 core set (MoSCoW)**

- **Must** — UX1 single-stroke recognition with the guard ladder; connector-ink node + one `create_connector` op carrying body + anchors; edge anchors with `facing`; the warp on move/resize/rotate of either node; one undo entry per gesture; mirror parity (0 divergent figures); "recognized" chrome + one-undo revert; invalid-on-delete behaviour.
- **Should** — UX2 retro-chaining; centre anchors; refuse/no-op feedback consistent with enclose (no error banner); `[connector]` device-log token.
- **Could** — terminals/arrowheads (v1.1); dashed style + 2–3 width presets; interior pins.
- **Won't (this campaign)** — physics rope; obstacle-aware routing; routing-style picker; desktop-side connector authoring; connectors to non-SmartGroup kinds; connector labels.

**North Star** — share of drawn connectors the creator never has to repair or redraw after a node move (target ≥90% on a scripted diagram session).

**Input metrics** — recognition recall on intended connectors ≥90%; connector visible p95 ≤500 ms after pen-up (matching the enclose bar); re-warp visible p95 ≤300 ms after move commit; mirror divergence **0** nodes.

**Ship gate (D25)** — false-positive rate ≤2% of `pen` strokes on a real ink corpus, measured with both recognizers armed as shipped, including a fresh page's first 20 strokes. Below the bar, the campaign does not ship with default-on.

**Guardrails** — `REQ-01` ink latency p95 ≤30 ms unchanged; 0 full-panel invalidations during a drag; exactly 1 undo entry per gesture; 0 extra wire ops per move; EP-016 enclose and EP-017 membership **re-verified** under the new fall-through pipeline (D21) rather than assumed; EP-018 / EP-019 / EP-025 chrome outcomes unchanged.

## Goal dependencies

- UX3 (warp) depends on the connector-ink node model (D1) — nothing to warp without a stored rest shape.
- UX2 (chaining) depends on UX1 (single-stroke recognition + binding rules); it adds only the chain walk.
- Terminals (D11) depend on UX1 and on the `terminal` slot existing in the model.
- Centre anchors (D9) depend on the `ADR-0010` §6 amendment.
- Nothing here depends on REQ-08; REQ-08's connector **re-anchor** tool depends on **this**.

## ASRs & quality drivers

| ASR / driver | why significant | provisional measure | pending ADR? |
|---|---|---|---|
| ASR-1 — Warp must be a **pure function** (no dt, no solver state) shared by device and desktop | It is the only reason the mirror can stay consistent under `ADR-0015` one-way sync without streaming geometry | 0 divergent nodes on shared fixtures; identical output for identical inputs | **yes** — new ADR for connector-ink geometry |
| ASR-2 — `create_connector` envelope must carry ink **body** + anchor attributes; `Connector` becomes a node with children | `SRS-IN-09` currently treats `create_connector` as `{id, from, to}` and `ADR-0010` says a connector is not a spatial parent; both need amending, and the device is not currently an author of this op (`SRS-EP-07`) | Round-trip through SVG persistence + op replay with 0 loss | **yes** — amend `ADR-0010` (+ `SRS-IN-09`, `SRS-EP-07` author set) |
| ASR-3 — Anchor model gains `facing` + `centre` kind | Perpendicular attachment and centre binding are product requirements from the human; the current schema has `port` / `boundary` only | Anchors re-resolve on transform with 0 detachment | **yes** — `ADR-0010` §6 amendment |
| ASR-4 — **BR-B02 conflict:** "Never created unprompted" is a shipped product rule. ~~Automatic connector recognition would be the first unprompted creation~~ | **Resolved R7** — human upheld the rule via recognizer toggles (D13/D14), but chose **default-armed** (D22), which retires BR-B02's "ordinary ink, forever" clause and converts the false-positive rate into a ship gate (D25) | Wording change in `BR-B02` + the ship gate in `## Goal prioritisation` | superseded by ASR-7 |
| ASR-5 — Recognizer chain becomes a first-class ordered component with one verdict per pen-up | Today the pen-up dispatch is a two-branch table; a third recognizer with precedence and logging is a structural change to the hot path | Ordered, logged, fixture-tested; 0 change to `REQ-01` ink latency | with the geometry ADR |
| ASR-6 — Deformation runs at **commit**, not per frame | e-ink partial-refresh budget (`BR-N06`) | 0 full-panel invalidations during a drag; re-warp p95 ≤300 ms | no |
| ASR-7 — **Tools vs recognizers split** (D14): 3 exclusive tools + 2 independent recognizer toggles supersedes `ADR-0017`. Ripples into `REQ-03` ("exactly four tools", "no sub-mode strip"), `SRS-EP-04`/`SRS-EP-05` (arming, status affordance), `SRS-EP-10` trigger-dispatch table, `BR-B02` wording, and the `UI-EP-02` design package | Changes shipped, human-PASSed chrome (EP-023 rebase, EP-025 ToolCanvasLayer) and the arming contract every recognizer reads | 0 regressions in EP-016 enclose / EP-017 membership / EP-018 selection-create / EP-019 manipulation / EP-025 chrome outcomes; active-state legible under partial refresh | **yes** — supersede `ADR-0017`; needs a `/designer` story |
| ASR-8 — Both recognizers may be armed on one stroke (impossible today) | Requires a deterministic closure classifier and an explicit answer on whether a **failed enclose falls through to draw-into** (Q6) — the current spec says membership never runs on an enclose stroke | One verdict per pen-up, logged; fixtures for closed/open/ambiguous strokes | with the geometry ADR |

## Conclusion

**Answer / framing.** Hand-drawn connectors between ink-boxes are the next campaign, ranked above
REQ-08 / CHL-0011 / CHL-0012, and they are feasible without giving up either natural ink or the
one-way sync contract. Three findings carry it:

1. **Recognition is endpoint overlap, not shape classification** — the InkKit rule ("endpoints of an
   edge fall inside two different node shapes") plus a five-rung guard ladder, dispatched by a
   closure classifier with a fixed fall-through order (D20/D21).
2. **UX3's hard problem has a third answer neither escape hatch named.** Store the drawn body as a
   **rest shape in spine coordinates `(s, d)`** and warp it from the two endpoints with a Hermite
   tangent blend at the ends (D3). This preserves wiggle exactly, so the smooth/wiggly split
   disappears; it is a **pure function**, so the desktop mirror stays consistent with zero extra ops
   and zero extra undo entries (D4); and it runs at commit, so e-ink is safe.
3. **The tool/recognizer split is the right ontology** (D14) — three exclusive tools plus two
   independent recognizer toggles. It keeps `BR-B02`'s "never unprompted" promise structurally while
   giving the creator both pure paper and one-fewer mode switch.

**Why conclude now.** All seven definition-of-enough items are addressed, the mandatory pre-mortem
ran in Round 4, and every remaining unknown is either an **empirical bet with an EXP route** (warp
naturalness, guard false-positive rate) or an **explicitly parked v1.1/v2 item** — not a gap in the
framing. Further rounds would restate rather than resolve.

**Definition-of-enough checklist**

- [x] Recognition rule for UX1 — guard ladder (R2-I3) + closure classifier and fall-through order (D20, D21)
- [x] UX2 chaining rule — retro-chaining at completion, bounded search, both draw directions, no pending node (D6)
- [x] One deformation model chosen with the alternative's rejection reasoned — D3, rejected EH2 on sync determinism, op cost, e-ink budget and testability (R3-I5, R3-I10)
- [x] Document model change named — connector-ink node with ink body + derived spine (D1), anchors gain `facing` + `centre` (D9/D16), `terminal` slot reserved (D11)
- [x] Wire + mirror consequences — 0 extra ops per move, 0 divergent nodes, shared fixtures (D4, ASR-1, ASR-2)
- [x] Riskiest bet + cheap validation — warp naturalness, host-harness EXP before campaign lock (D19)
- [x] Product-rule conflicts surfaced, not overridden — `BR-B02` and `ADR-0017` (ASR-4→ASR-7), plus the two clauses retired below

**Clauses this campaign supersedes** (for `/pm` to propagate per element, no cascade):

| Element | Clause retired | Superseded by |
|---|---|---|
| `ink-box/srs-product.md` **BR-B02** | "A rectangle drawn in `Pen` mode is ordinary ink, forever." | D22 — recognizers ship armed; "never unprompted" = "never without an armed recognizer" |
| `ink-box/srs-logic.md` **SRS-EP-10** | Draw-into membership "**Never** runs on an enclose stroke" | D21 — failed enclose falls through to membership |
| `ADR-0017` | Four exclusive ToolChip tiles incl. `ink_box` as a tool | D14 — 3 tools + 2 recognizer toggles (new ADR supersedes) |
| `epaper/prd.md` **REQ-03** | "exactly **four** tools"; "no separate sub-mode strip" | D14/D24 — three tools + two named recognizer toggles (not pen sub-modes) |
| `epaper/prd.md` Non-Goals | "Rotation and **connector attachment** on a Smart Group this iter — they arrive with REQ-08" | REQ-09 (connectors precede REQ-08) |

**Rejected / parked.** EH2 live physics rope (parked as a deterministic fixed-iteration `rope`
character class, D12); interior pins for deliberate detours (D12); terminals/arrowheads with the
InkKit/Tahuti algorithm ready to borrow (D11); routing styles and obstacle-aware matrix routing
(D10); dangling half-connector nodes (D6); loose `PointAnchor` ends as persisted state; connectors to
non-SmartGroup kinds (D18); connector labels; desktop-side connector authoring.

## Outputs / next actions

All nine gate questions are **answered** (Rounds 6–7). The remaining gate is empirical, not a
question: the EXP below blocks the campaign lock (D19) and the false-positive bar blocks ship (D25).

Sequenced actions (human chose **EXP before campaign lock**, so the lock stays open):

- [ ] `/explore` **[EXP-0002](../explorations/EXP-0002-connector-ink-warp.md)** — (a) warp naturalness on the host harness over a scripted 20-case move set; (b) guard false-positive rate on a real ink corpus incl. a fresh page's first 20 strokes. Blocks the campaign lock (D19) and the ship gate (D25) → owner **architect + qa** · **drafted 2026-08-14, `status: proposed`**
- [ ] `/pm` mint **[REQ-09] On-device connectors** + amend **[REQ-03]** for the tools-vs-recognizers split (D14/D24); retire the five clauses listed in the Conclusion with `superseded-by`, propagating lifecycle per element; rank REQ-09 **above** REQ-08 / CHL-0011 / CHL-0012 → owner **pm**
- [ ] `/architect` supersede **ADR-0017** (ASR-7); new ADR for connector-ink geometry (ASR-1); amend **ADR-0010** §6 for `facing` + `centre` (ASR-2, ASR-3); closure classifier + one-verdict dispatch (ASR-5, ASR-8) → owner **architect**
- [ ] `/designer` ToolChip design story — 3 tools + 2 toggles, active/armed states legible under partial refresh, do-not-regress list for EP-023 / EP-025 → owner **designer**
- [ ] `/pm` flip the MASTER `execution:` lock to this campaign **after** the EXP verdict (currently `features: []`, `stop_line: srs-ready`, `autonomy: ask`) → owner **pm**
- [ ] `/sm` slice only after PRD → SRS (stop line is `srs-ready`) → owner **sm**
