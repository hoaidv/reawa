---
feature: node-manipulation
parent_req: [REQ-08]
version: 0.1.0
lifecycle: active
owner: pm
campaign: next
---

# SRS — Direct manipulation of any document node (Product)

PM feature depth for [REQ-08](../../prd.md#node-manipulation). **Thickened now, built later** — the
model exists so that [REQ-06](../../prd.md#device-manipulation) ships as a conforming subset instead
of a one-off box handler that has to be torn out.

Benchmark: the manipulation depth creators already expect from modern vector drawing tools. Not
their feature list — their **coherence**. In a good vector app you never wonder whether this object
resizes differently from that one.

## Intent / JTBD

A creator manipulating a document is doing one job — *arranging their thinking in space* — with many
kinds of material. If every kind of material answers to a different set of gestures, the tool stops
being an extension of the hand and becomes a set of modes to remember.

So the job is: **learn the vocabulary once, apply it everywhere.** Move, resize, rotate, align,
duplicate, delete work the same way whatever is selected. Where a kind genuinely differs — text
reflows, a connector re-anchors, ink can hold its sample size — that difference shows up as *extra*
tools on top of the shared base, never as a different base.

The engineering half of the same job: **adding a node kind must not mean editing the manipulator.**

## In scope / out of scope (feature grain)

| In scope | Out of scope |
|---|---|
| Shared verb set every node kind can declare | A brush engine / illustration suite |
| Per-kind distinct tools layered on the base | Auto-layout and constraint solvers |
| Capability descriptor as the extension contract | Component instances, variants, symbol libraries |
| Selection model (single, multi, marquee, enter/exit group) | Multi-user selection / presence |
| Gesture grammar shared across pen, finger, and mouse | Keyboard-only accessibility parity (tracked separately) |
| E-ink feasibility rules for manipulation feedback | Desktop-side manipulation UI (returns with multi-directional sync) |
| Undo granularity: one entry per gesture | Redo trees / non-linear history |

## Product model

### [Layer 1] Shared verbs — the base every kind declares against

~99% of node kinds support these. A kind that omits one must say why in its descriptor.

| Verb | Product meaning | Notes / bar |
|---|---|---|
| `select` | Make this node the subject of the next action | Topmost-first resolution; hit tolerance sized for a pen tip, not a mouse |
| `multi-select` | Add or remove nodes from the subject set | Additive gesture; mixed-kind sets are legal |
| `marquee` | Sweep an area to select what it covers | Containment vs intersection rule stated per platform |
| `enter` / `exit` group | Descend into a container to act on its children, then come back out | Depth is visible; exit is always reachable |
| `move` | Translate the node(s) | Drag from inside bounds; no prior selection needed for a single node |
| `resize` | Change extent from a corner or a side handle | Corner = both axes, side = one axis; **aspect lock** as a modifier |
| `rotate` | Turn about the transform origin | **Snap increments** by default; free rotation as a modifier |
| `nudge` | Small discrete displacement without a drag | Coarse/fine variants |
| `duplicate` | Produce an independent copy in place or offset | Copy is fully independent — no live link |
| `delete` | Remove the node(s) | Referential integrity handled per kind (see connectors) |
| `z-order` | Raise/lower within the sibling list | Tree sibling order **is** paint order — no separate z-index |
| `align` / `distribute` | Line up or space out a multi-selection | Relative to the selection bounds or to a chosen anchor node |
| `set-transform-origin` | Choose the pivot for resize and rotate | Defaults to bounds centre; movable |
| `snap` / `guides` | Constrain a drag to meaningful positions | Node edges/centres, and increments |
| `constrain` (modifier) | Axis lock, aspect lock, snap toggle during a gesture | Modifier vocabulary is shared, not per-kind |

**Shared-verb invariant (load-bearing).** Two kinds that both declare the same verb must respond to
the **same gesture** and show the **same gizmo**. If a kind needs different behaviour, it is a
different verb with a different name — not a variation of a shared one.

### [Layer 2] Per-kind distinct tools

Layered on top of the base; never replacing it.

| Node kind | Distinct tools | Why it differs |
|---|---|---|
| **Ink** | ink-scale mode (keep sample size vs scale with bounds), simplify, split at a point, join | A stroke is a sample path, so "resize" has two honest meanings |
| **Text** | edit content, resize-vs-reflow choice, baseline / alignment | A text box's width is layout input, not just extent |
| **Primitive** | endpoint drag (line), corner radius (rect), arc handles (ellipse) | Its geometry has named parameters the creator wants directly |
| **Connector** | re-anchor either endpoint, routing style, port vs free attachment | It is defined by what it connects, so it moves when they move |
| **Frame** | artboard resize (with or without children), clip toggle | Root-level container with page-like semantics |
| **Group** | ungroup, add/remove member | Its identity is its membership |
| **SmartGroup** | ink-scale mode, boundary-vs-content awareness, enter/exit to reach children | Boundary ink always transforms; content obeys the scale mode |

### [Layer 3] Capability descriptor — the extension contract

Every node kind declares, in one place, what it can do. The manipulation host reads the declaration;
**adding a kind must not require editing the host.**

A descriptor states, per kind:

- **Supported shared verbs**, and for each, any parameters (which resize handles exist, whether
  rotation snaps, which origins are legal).
- **Omitted shared verbs with a stated reason** — a Frame may refuse `rotate`; the reason is product
  documentation, not a silent gap.
- **Distinct tools** the kind adds, each with the gesture or control that invokes it.
- **Hit-test contribution** — what counts as "inside" for this kind (bounds, path proximity,
  endpoint zones).
- **Transform semantics** — how the kind's geometry responds to translate / scale / rotate, including
  what is preserved (ink sample size, text baseline, connector attachment).
- **Undo unit** — what one gesture on this kind commits as a single entry.

**Mixed-selection rule.** For a multi-selection, the available verb set is the **intersection** of
the members' declared verbs. The UI shows what is available for the whole set and states why
something is missing, rather than silently applying to some members.

### [Layer 4] Cross-cutting rules

| Rule id | Statement | Notes |
|---|---|---|
| BR-N01 | **One gesture, one undo entry.** Whatever the kind, a completed drag commits a single undoable op. | Inherited from ink-box BR-B13 |
| BR-N02 | **What you see under the pen is the document.** Live direct manipulation; no advisory ghost corrected afterwards. | Inherited from BR-B10; non-negotiable after CHL-0006/0007 |
| BR-N03 | **Committed = last previewed.** Release position is final; 0 px jump. | CHL-0007 regression bar |
| BR-N04 | **Hit tolerance is input-aware.** Pen, finger, and mouse get different tolerances for the same node; the *rules* are shared, the numbers are not. | Finger is not a precise pointer |
| BR-N05 | **Below the LOD cutoff, manipulation is unavailable and says so.** Uniform across kinds. | Inherited from BR-B14 |
| BR-N06 | **E-ink feasibility.** Manipulation feedback must fit the panel's partial-refresh budget: no full-panel invalidation during a gesture, and no assumption of a 60 Hz redraw. Slow is acceptable; wrong is not. | The constraint that makes this device-specific |
| BR-N07 | **Extension without modification.** A new node kind gains selection, move, and resize by declaring them — 0 changes to the manipulation host. | The engineering half of the JTBD |
| BR-N08 | **Verb names are product vocabulary.** The same word means the same thing in the descriptor, the UI, the ops, and the docs. | Prevents per-kind dialects |
| BR-N09 | **Manipulation is device-local and immediate**, then published as a document change like any other edit. | [REQ-07](../../prd.md#one-way-sync) |
| BR-N10 | **Coverage bar.** ≥90% of node kinds support the full shared verb set; every exception is declared with a reason. | Measurable in the descriptor itself |

### [Layer 5] Forward-compatibility contract for REQ-06

[REQ-06](../../prd.md#device-manipulation) ships **before** this feature is built, so it must be
written as a citizen of this model rather than a prototype of it. Binding requirements:

| Constraint | What REQ-06 must do |
|---|---|
| Declaration, not hard-coding | SmartGroup registers a descriptor declaring `{select, move, resize, set-ink-scale-mode}` — even though it is the only kind registered |
| Shared gesture grammar | Press-inside-to-move, handle-to-resize, press-empty-to-deselect are the shared definitions, not SmartGroup's private ones |
| Shared gizmo geometry | Bounds outline + handle placement/tolerance are the shared spec that a second kind will reuse unchanged |
| Reserved rotation | The transform op envelope carries a `rotation` field that stays unset; no code path may assume rotation cannot exist |
| Undo unit | One entry per gesture, per BR-N01 |
| Omission is declared | `rotate`, `multi-select`, `marquee`, `enter/exit` are recorded as **not yet declared**, not as impossible |
| No bespoke ops | SmartGroup's transform validates against the shared envelope; 0 kind-specific op shapes |

If [REQ-06](../../prd.md#device-manipulation) satisfies these seven rows, this feature's first
iteration is additive — new kinds and new verbs — with no rework of the shipped box.

## Edge cases

| Case | Expected product behavior |
|---|---|
| Multi-selection of kinds with disjoint verbs | Offer the intersection; state what is unavailable and why |
| Rotate a node whose kind omits `rotate` | The handle is absent for that selection; no silent no-op |
| Delete a node a connector references | Connector is invalidated per its own rule (flag or remove); never a crash or a dangling reference |
| Resize dragged past the opposite edge | Bounds normalize; no negative-extent document state |
| Snap target and free position conflict | Modifier decides; the active constraint is visible during the drag |
| Gesture starts below LOD then the viewport zooms in mid-gesture | The gesture keeps its initial availability decision; no mid-drag mode flip |
| A kind declares a verb but its transform semantics are undefined | Descriptor is invalid — this is a build-time error, not a runtime surprise |
| Enter a group, then its last child is deleted | Exit to the parent; empty containers follow their kind's own rule |
| Undo across an enter/exit boundary | Undo reverts the op regardless of current depth; depth is view state, not document state |
| Two kinds want the same gesture for different distinct tools | Resolved at descriptor level before ship; the shared base always wins the gesture |

## Acceptance (drives BDD / stories)

- Given a new node kind registered with a descriptor declaring the shared verbs, When a creator
  selects and manipulates it, Then it supports select, move, and resize with **0 changes** to the
  manipulation host.
- Given two kinds that both declare `move` / `resize` / `rotate`, When each is manipulated, Then the
  gesture grammar and gizmo geometry are identical (1 shared model; 0 per-kind special cases).
- Given all node kinds in the document model, When the descriptors are audited, Then ≥90% support the
  full shared verb set and 100% of omissions carry a stated reason.
- Given a mixed multi-selection, When the creator opens the manipulation affordances, Then exactly
  the intersection of declared verbs is offered and unavailable verbs are explained (0 silent
  partial applications).
- Given a node declaring `rotate` with snapping active, When the creator rotates, Then the angle
  lands on the snap increment within ±0.5° and one undo restores the previous transform exactly.
- Given any completed manipulation gesture on any kind, When the creator releases, Then committed
  geometry equals last previewed geometry (0 px jump) and exactly 1 undo entry is added.
- Given any gesture on the e-paper panel, When feedback renders, Then 0 full-panel invalidations
  occur during the gesture.
- Given SmartGroup as shipped under [REQ-06](../../prd.md#device-manipulation), When this feature
  lands, Then SmartGroup requires 0 changes to its declared capabilities and 0 changes to its
  transform op shape.

## Implemented via

| Concern | Pointer |
|---|---|
| Journeys | [srs-experience.md](./srs-experience.md) |
| Manipulation host, descriptor schema, gesture resolution | `srs-logic.md` — **deferred to the REQ-08 iteration** |
| Gizmos, handles, mixed-selection affordances | `srs-ui.md` — **deferred**, needs design |
| Budgets, coverage audit, regression bars | `srs-quality.md` — **deferred** |
| Decision record | **ADR-0016** — deferred |
| First conforming citizen | [ink-box srs-product](../ink-box/srs-product.md) — BR-B17, BR-B18 |
| Node kinds | [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md) · [ADR-0011](../../../../adr/ADR-0011-smart-group.md) |

---

## Open questions for the REQ-08 iteration

- Does `enter/exit group` become a view mode with visible depth, or an implicit consequence of
  clicking through? — **owner:** pm with designer
- Marquee semantics on a pen-first device: containment or intersection, and how it is invoked
  without a modifier key — **owner:** pm with designer
- Whether the desktop regains manipulation at the same time (it needs multi-directional sync first)
  — **owner:** pm
- Descriptor home: shared domain doc vs per-module SRS, given both peers will eventually read it —
  **owner:** architect
- Whether text editing on the device is in this feature or its own REQ — **owner:** pm

---

## Superseded

_None yet._
