---
feature: ink-box
parent_req: [REQ-05, REQ-06]
version: 0.2.0
lifecycle: active
owner: pm
---

# SRS — Ink-box on the device (Product)

PM feature depth for [REQ-05](../../prd.md#device-ink-box) and
[REQ-06](../../prd.md#device-manipulation).

**Inheritance.** Smart Group *semantics* are unchanged from
[ADR-0011](../../../../adr/ADR-0011-smart-group.md) and the infini rules BR-09…BR-09j in
[vector-document srs-product](../../../infini/features/vector-document/srs-product.md). What changes
is **where they run** and **what the creator sees while they run**. Rules below are renumbered
`BR-B*` for this feature; the mapping is noted per row.

## Intent / JTBD

A creator writes a thought by hand, draws a box around it, and expects the box to *be* a box — right
there, under the pen. Later they nudge it aside to make room, or stretch it to fit more writing. The
job is not "group some strokes"; it is **keeping a page malleable while thinking on it**, at the
speed of paper.

Two things break that job, and the pilot did both: waiting for the box to come back from somewhere
else, and watching the box you just dragged jump to a different position because something else had
the final say. This feature removes both.

## In scope / out of scope (feature grain)

| In scope | Out of scope |
|---|---|
| Enclose recognition on the device (rect-only, guarded) | OCR / handwriting-to-Text; "is this text?" gating |
| Selection-create from a surround stroke | AABB-only grouping with no boundary ink |
| Draw-into membership for existing boxes | In-box alignment, reflow, auto-padding; sizing `FREE_FORM`/`WRAP_CONTENT`; `align-content` ([CHL-0012](../../../../../.plan/iter-003/challenges/CHL-0012-inkbox-sizing-align.md)) |
| Select, move, resize, `inkScaleMode` toggle, deselect | **Rotation** — [REQ-08](../../prd.md#node-manipulation). Connector attachment — [REQ-09](../../prd.md#device-connectors) |
| Live direct manipulation of real ink | Advisory ghosts / outline stand-ins corrected later |
| One undoable entry per gesture; linear redo | Branching history, per-sample undo |
| Nested boxes (enclose + paste); empty-child flatten | Marquee/freeform pick of nested children — tap only ([CHL-0032](../../../../../.plan/iter-005/challenges/CHL-0032-nested-ink-box.md)) |
| Nested boxes resolved by paint order | Multi-select, enter/exit group — [REQ-08](../../prd.md#node-manipulation). Marquee is **top-level only** |
| Non-rectangular enclosure | Ellipse / lasso enclosure |

## Business rules / eligibility / policy

### Creation

| Rule id | Statement (product language) | Inherits |
|---|---|---|
| BR-B01 | **The device recognizes and creates.** Enclose evaluation, guards, capture, and the resulting Smart Group all happen on the device at pen-up. No peer message is required for the box to exist or to be visible. | replaces BR-09g placement |
| BR-B02 | **Never created unprompted.** Creation requires **Ink-box recognition** armed when the enclosing stroke is drawn, or an explicit Smart Group command on a selection. Recognizers ship armed; a rectangle drawn with the toggle **off** is ordinary ink, forever. | BR-09a; BS-0001 D13/D22 |
| BR-B03 | **Enclose guards (adaptive).** Closed-ish (start near end) counts. **With capturable content (ink or nestable/flattenable box):** shorter side ≥ **28** world. **Empty box (boundary first):** shorter side ≥ **36** and the stroke must look like a primitive (circle / ellipse / triangle / square / rectangle / parallelogram / diamond / pentagon / hexagon / octagon) so letters and scribbles stay ink **when they do not already sit inside a parent**. A closed stroke already inside an existing box still **joins it** as content (no nested *create* of a second empty box for that stroke). | BR-09b; CHL-0032 |
| BR-B04 | **Captures any ink.** No content test — handwriting, sketch, or shape all qualify. | BR-09c |
| BR-B05 | **A box always reads as a box.** A successful create always has `role: boundary` ink — the creator's own stroke, never a synthetic rectangle. | BR-09d |
| BR-B06 | **Selection create needs a surround stroke.** One selected stroke must contain ≥80% of each other selected **free** ink's samples (open stroke OK — tested via an artificial closed path). That stroke becomes `boundary`; remaining free ink → `content`; selected **non-empty** Smart Groups nest; selected **empty** Smart Groups flatten (BR-B21). If no surround qualifies, **creation is refused** with a visible reason. Nesting is **not** a refuse reason. | BR-09j; CHL-0032 |
| BR-B07 | **Draw-into membership.** A new `Pen` stroke with ≥80% of **polyline length** inside one or more boxes’ **boundary ink** joins the qualifying box with the **highest paint/z order** (later sibling wins; no z-index field). Never dual-parented. **Not** ≥80% of samples inside the AABB. | BR-09g |
| BR-B08 | **Free layout inside the box.** Appending content never reflows, realigns, or shifts existing children. Placement is as drawn. | BR-09h |
| BR-B09 | **Recognition is best-effort plus undo.** A wrong box costs one undo. The product does not chase perfect recognition. | REQ-05 |

### Manipulation

| Rule id | Statement (product language) | Inherits |
|---|---|---|
| BR-B10 | **What you see under the pen is the document.** Manipulation feedback mutates the real ink; there is no advisory outline that something else later corrects. On pen-up the committed geometry **equals** the last previewed geometry. | supersedes SRS-IN-13 ghost model; CHL-0006 |
| BR-B11 | **Move needs no prior selection**; dragging inside the bounds moves the box. Resize requires explicit selection and a handle. Pressing empty canvas deselects. | BR-09f / SRS-IN-11 |
| BR-B12 | **`inkScaleMode` applies to content only.** `withBounds`: content scales with the box. `fixedInk`: each content ink keeps its sample size and tracks the box by **its own** UV/offset, so a newly drawn stroke never moves older content. **Boundary ink always transforms with the frame.** | BR-09i, CHL-0004/0005 |
| BR-B13 | **One gesture, one undo entry.** A drag is a single op committed at pen-up, not a stream of micro-ops. | SRS-IN-11 |
| BR-B14 | **Below the LOD cutoff, manipulation is unavailable and says so.** The gesture must not silently do something else. | SRS-IN-11 |
| BR-B15 | **Slow is acceptable; wrong is not.** E-ink refresh may lag a drag, but the picture must never settle to a different geometry than the one the creator released. | CHL-0006, CHL-0007 |
| BR-B16 | **Deselect leaves nothing behind.** No residual selection chrome on the next settled frame. | CHL-0007 |
| BR-B17 | **Rotation is out of scope but not designed out.** The transform op carries a rotation field that stays unset; nothing in this feature may assume rotation cannot exist. | REQ-06 ↔ REQ-08 |
| BR-B18 | **SmartGroup declares its capabilities.** `{select, move, resize, set-ink-scale-mode}` through the shared capability descriptor of [node-manipulation](../node-manipulation/srs-product.md) — not as a bespoke, hard-coded tool. | REQ-06 conformance |
| BR-B19 | **Live node paint is overlay, settle is document.** During move/resize the origin box **and bound connector spines** are hidden on the document surface and the live ink + chrome + live connectors are painted on ToolCanvasLayer. On pen-up, one raster of the committed node **and re-warped connectors** on CanvasLayer (origin∪live∪connector AABB). Mid-gesture ghosting/dirty traces are allowed; a settled duplicate or snap-back is not. Painting the live node on CanvasLayer (option 2) is deferred. | CHL-0018 / ADR-0019 |

### Nesting ([CHL-0032](../../../../../.plan/iter-005/challenges/CHL-0032-nested-ink-box.md))

| Rule id | Statement (product language) | Inherits |
|---|---|---|
| BR-B20 | **Ink-boxes nest.** Creation A (recognizer) and Creation B (Enclose CTA) both capture **free ink and existing Smart Groups**. A non-empty captured box becomes a child ink-box. Draw-into and paste into a box remain legal. | CHL-0011 scheduled; CHL-0032 |
| BR-B21 | **Flatten empty children (Rule 1).** An ink-box is **empty** when it has only `role: boundary` ink (no content-role children, no nested Smart Groups). When such a box would become a child of another ink-box, **discard the wrapper**: apply the child’s own-transform (including any reserved rotation) to that boundary ink, then insert the ink as `role: content` of the parent. 0 redundant empty layers. | CHL-0032 Rule 1 |
| BR-B22 | **Composed paint (Rule 2).** A parent’s transform applies to **all** descendants, any kind. Each ink-box keeps its **own-transform**. World starts at identity; each box paints with `outcome = ancestorContext × own-transform` on boundary ink, then a content-outcome (scale applied or not per `inkScaleMode`) as the context passed to children. Nested boxes stay correct when the camera changes. **Content clip:** content-role ink, nested boxes, and their descendants are clipped to this group’s **natural world AABB** (axis-aligned hull of local `bounds` after outcome). Overflow is **not painted**. Clip is that AABB — not even-odd of boundary ink. Dirty / hierarchy cull stay on the natural AABB (no descendant-ink union). | CHL-0032 Rule 2; AABB clip 2026-09-05 |
| BR-B23 | **Tap-select any level (Rule 3).** Hit-test prefers **children over ancestors** (paint order among siblings still later-wins), **inside** the ancestor clip. A press outside a group’s natural world AABB does **not** enter that subtree — overflow is **not hittable**. Move / resize / (reserved) rotate of the selected child mutates **that child’s own-transform only**. The context toolbar is for the selected child. Paste-into-box children are selectable where they overlap the parent AABB. | CHL-0032 Rule 3; AABB clip 2026-09-05 |
| BR-B24 | **Marquee / freeform are top-level (Rule 4).** `sel_rect` / `sel_freeform` membership includes free ink and **top-level** ink-boxes only. Nested children are not independently lassoed. Nested pick is tap. | CHL-0032 Rule 4 |
| BR-B25 | **Reparent at end of move (Rule 5).** When a move **commits**, compute a new parent: the highest paint-order node that contains ≥**80%** of the moving node’s **natural area**; else the document root. Not during the drag. Resize does not reparent. | CHL-0032 Rule 5 |

## Edge cases

| Case | Expected product behavior |
|---|---|
| Enclose stroke encloses nothing | Boundary-only box **only if** ≥ 36 world **and** near-primitive shape; else ordinary ink |
| Enclose stroke below the minimum size | Same — treated as ordinary ink (protects small annotations) |
| Enclose stroke drawn in `Pen` mode | Ordinary ink, always; never grouped retroactively |
| Enclose captures ink already inside another box | Ink with an existing Smart Group parent is skipped as *free* capture; the **parent box** may itself be captured if ≥80% of its natural area is inside |
| Enclose around an existing Smart Group (nested box) | **In** — non-empty box becomes a nested child; empty box flattens (BR-B21). Same for Enclose CTA ([CHL-0032](../../../../../.plan/iter-005/challenges/CHL-0032-nested-ink-box.md)) |
| `FREE_FORM` / `WRAP_CONTENT` sizing or `align-content` | **Out of this campaign** — [CHL-0012](../../../../../.plan/iter-003/challenges/CHL-0012-inkbox-sizing-align.md); shipping stays `inkScaleMode` + non-expanding membership |
| Consecutive encloses in quick succession | Each is independent and correct; no shared state carries between them (CHL-0007 regression) |
| Selection create with no surround stroke | Refused; selection unchanged; the UI states the reason |
| Selection create with an open surround stroke | Artificial closed path used for the ≥80% test; the open stroke is preserved as `boundary` samples |
| New stroke ≥80% inside several nested boxes | Highest paint/z-order qualifier wins; never dual-parented |
| Letter recognized as empty ink-box, then surrounding enclose | Flatten: letter ink is content of the new box; 0 leftover empty wrapper; move/resize of the parent takes the letter |
| Paste ink-box into ink-box | Child remains a nested Smart Group (unless empty → flatten); tap-selectable **inside the parent AABB**; own-transform manipulable; paints correctly after camera change |
| Tap nested child vs parent | Child wins when the press is inside both AABBs; toolbar is the child’s; parent pose unchanged until Rule 5 reparent |
| Nested child outside parent AABB (`fixedInk` then `withBounds`, or a moved child) | Overflow is neither painted nor tap-selectable; parent AABB dirty/cull is sufficient |
| Marquee over nested cluster | Top-level boxes + free ink only; nested children not independently selected |
| Move child 80% into another box | On commit, reparent to that highest paint-order container |
| Move child <80% into any box | On commit, parent = document root |
| `fixedInk` resize with several content inks | Each ink's own UV preserved; no cross-ink translation; sample size unchanged (CHL-0004/0005 regression) |
| New stroke <80% inside every box | Stays ordinary ink at its current parent |
| Resize that would invert the box (handle dragged past the opposite edge) | Bounds normalize; content and boundary follow the normalized rect; no negative-size document state |
| Drag started, then the link drops | Gesture completes and commits locally; the change queues |
| Drag attempted below the LOD cutoff | No manipulation starts; UI shows it is unavailable |
| Box left with zero children after an edit | Disallowed; removing the last child removes the Smart Group and restores the ink to the parent |
| Panel refresh in flight when a gesture starts | Gesture takes priority; feedback uses partial refresh; no full-panel invalidation mid-drag |
| Move/resize in flight (live node on ToolCanvasLayer) | Origin box hidden on CanvasLayer; live ink+chrome on ToolCanvasLayer; e-ink trail/ghost during the drag allowed; pen-up one settled raster (CHL-0018) |
| Undo pressed mid-gesture | Ignored until the gesture commits; undo then reverts exactly that gesture |

## Acceptance (drives BDD / stories)

- Given `Ink-box` armed and free ink inside, When the creator draws a closed-ish enclosure with shorter side ≥ 28, Then a Smart Group is created (shape gate off).
- Given empty canvas, When the creator draws a closed primitive ≥ 36, Then a boundary-only Smart Group is created.
- Given empty canvas, When the creator draws a closed scribble or a letter-like loop, Then the stroke stays ordinary ink (`fail=not_primitive` or `fail=too_small_empty`).
- Given the negative fixture set (below minimum size, `Pen` mode / recognizer off), When each gesture
  ends, Then 0 Smart Groups are created and every stroke remains ordinary ink.
- Given a selection containing a surround stroke at the ≥80% bar, When the creator invokes Smart
  Group, Then the surround becomes `boundary`, the rest `content`, and `bounds` matches the surround's
  fitted rect (±1 px @ 100% zoom).
- Given a selection with no qualifying surround stroke, When the creator invokes Smart Group, Then
  0 boxes are created, the selection is unchanged, and the reason is visible.
- Given a selection that includes a **non-empty** Smart Group and a qualifying surround, When the
  creator taps Enclose, Then that Smart Group is a nested child of the new box (0 nesting refuse).
- Given a selection that includes an **empty** Smart Group (boundary only) and a qualifying surround,
  When the creator taps Enclose, Then the empty wrapper is gone and its boundary ink is `content` of
  the parent.
- Given an empty ink-box (letter-like) plus surrounding free ink, When Creation A enclose captures
  the cluster, Then the letter ink is inside the new box (flattened) and is not left behind on a
  subsequent parent move.
- Given a nested child (paste or enclose), When the creator taps the child, Then the child is
  selected, the toolbar is the child’s, and move/resize mutates only the child’s own-transform.
- Given `sel_rect` or `sel_freeform` over nested boxes, When the gesture commits, Then nested
  children are not independently selected.
- Given a nested child moved so ≥80% of its natural area is inside another box, When the move
  commits, Then that box is the new parent; otherwise the parent is the document root.
- Given a `Pen` stroke with ≥80% of **polyline length** inside a box’s **boundary ink**, When it ends, Then it becomes `content`
  within 300 ms and 0 existing content inks move.
- Given 10 consecutive enclose gestures, When each completes, Then each yields exactly one correct
  box (0 desync, 0 lost boxes).
- Given a box and a drag inside its bounds, When the creator releases, Then the committed geometry
  equals the last previewed geometry (0 px jump) across a scripted 20-gesture set with 0 snap-backs.
- Given `fixedInk` and a handle drag, When the creator releases, Then each content ink's sample size
  changed ≤1 px, each UV is preserved ±1 px @ 100% zoom, unrelated inks did not move, and the
  boundary transformed with the frame.
- Given `withBounds` and a handle drag, When the creator releases, Then content scales with the
  bounds within ±1 px @ 100% zoom of the expected transform.
- Given a drag in progress, When feedback renders, Then it updates at ≥5 Hz with partial refresh
  only (0 full-panel invalidations) and no stall exceeds 200 ms. The live box is on ToolCanvasLayer
  (0 second copy on CanvasLayer). Mid-gesture ghosting does not fail; pen-up settled frame shows
  exactly one box at the committed geometry.
- Given any completed gesture, When the creator undoes once, Then exactly that gesture reverts
  (1 entry per gesture; 0 partial reverts).
- Given a selected box, When the creator presses empty canvas, Then the next settled frame shows
  0 residual selection pixels.
- Given the link is down, When the creator runs the scripted 10-gesture create + manipulate set,
  Then results are identical to the linked run (100% parity).
- Given the capability descriptor, When SmartGroup is registered, Then its declared verbs are exactly
  `{select, move, resize, set-ink-scale-mode}` and its transform op validates against the shared
  envelope with `rotation` unset (0 bespoke op shapes).

## Implemented via

| Concern | Pointer |
|---|---|
| Journeys | [srs-experience.md](./srs-experience.md) |
| Recognition, guards, hit-test, transforms | `srs-logic.md` — architect |
| Selection affordances, handles, refuse states | `srs-ui.md` — architect + designer, **needs design** |
| Budgets + CHL regression bars | `srs-quality.md` — architect |
| Document it edits | [device-document](../device-document/srs-product.md) |
| Tool arming | [tool-modes srs-logic](../tool-modes/srs-logic.md) — [SRS-EP-04] |
| Forward model | [node-manipulation](../node-manipulation/srs-product.md) |
| Node semantics | [ADR-0011](../../../../adr/ADR-0011-smart-group.md) |

---

## Superseded

Inherits, on the device, the semantics of infini [SRS-IN-10], [SRS-IN-11], [SRS-IN-15], [SRS-IN-16]
and the UI intent of [SRS-IN-14]. See the
[lifecycle map](../../../../../.plan/iter-003/lifecycle-map-2026-08-13.md).
