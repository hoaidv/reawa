---
feature: node-manipulation
parent_req: [REQ-08]
version: 0.1.0
lifecycle: active
owner: pm
co_author: designer
purpose: PRD → technical bridge — journeys for manipulating any document node
campaign: next
---

# SRS — Direct manipulation of any document node (Experience)

Journeys for [REQ-08](../../prd.md#node-manipulation). Policy and the capability model live in
[srs-product.md](./srs-product.md).

**Status: next campaign.** These journeys are written now so the
[ink-box](../ink-box/srs-experience.md) journeys can be checked against them — anything that would
have to be *unlearned* when this lands is a design bug in [REQ-06](../../prd.md#device-manipulation).

**Single scene.** Same full-bleed panel as every other Epaper feature; these are in-scene state
sequences, not navigation.

---

## Capability narrative

The page fills up. The creator has boxed thoughts, sketched a diagram, dropped in a typed label, and
drawn arrows between things. Now they rearrange: pick up two boxes at once and slide them down,
turn a sketch slightly so it lines up with the margin, stretch a text label and watch it reflow
while the box beside it keeps its handwriting the same size, then drag an arrow's end onto a
different box.

Nothing about that sequence should require remembering which object plays by which rules. The
handles look the same, the drag feels the same, and one undo always means one thing they just did.
The differences that *do* exist — text reflowing, ink holding its size, an arrow re-anchoring — are
differences the creator can see and would expect from the material itself.

---

## Entry context

| Field | Value |
|---|---|
| Persona / role | Creator on the reMarkable 2, working a page that already has mixed content |
| Situation / when | Rearranging rather than authoring — the thinking exists, the layout does not |
| Trigger | Finger tap on `Selection` |
| Preconditions | [REQ-04](../../prd.md#device-document)…[REQ-07](../../prd.md#one-way-sync) shipped; document holds more kinds than SmartGroup |

---

## Primary journeys

### Journey: `journey.manipulate_any` — Same gestures, different material

- **Realizes:** [REQ-08](../../prd.md#node-manipulation); BR-N08, shared-verb invariant
- **Success end-state:** the creator moves and resizes three different node kinds without changing
  how they work

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Presses a Smart Group and slides it — handles appear where expected | `selection.selected` | Same as they already learned in REQ-06 |
| 2 | Presses a text label and slides it — identical grip, identical handles | `selection.selected` | Shared gizmo geometry |
| 3 | Resizes the label; it reflows because text is text | `selection.resizing` | The *difference* is in the material, not the gesture |
| 4 | Resizes a handwriting box under `fixedInk`; the writing keeps its size | `selection.resizing` | Per-kind tool, shared verb |

### Journey: `journey.multi_select` — Act on several things at once

- **Realizes:** BR-N01, mixed-selection rule

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Selects one node, then adds a second and a third | `selection.multi` | Additive gesture |
| 2 | The affordances show what all three can do | `selection.multi` | Intersection of declared verbs |
| 3 | Drags them together as one | `selection.moving` | Relative positions preserved |
| 4 | Aligns their left edges | `selection.aligned` | Align acts on the selection bounds |
| 5 | Undoes once — the whole align reverts | previous state | One gesture, one entry |

### Journey: `journey.rotate_snap` — Turn something straight

- **Realizes:** rotate verb; BR-N06

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Selects a sketch; a rotate affordance is available because the kind declares it | `selection.selected` | Absent for kinds that omit `rotate` |
| 2 | Turns it; the angle settles on snap increments | `selection.rotating` | Free rotation via modifier |
| 3 | Releases; the panel repaints in partial passes | `selection.selected` | No full-panel flash |
| 4 | Undo restores the previous angle exactly | previous state | ±0.5° bar |

### Journey: `journey.enter_group` — Reach inside a container

- **Realizes:** enter/exit verb

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Wants to adjust one stroke inside a box | `selection.selected` | |
| 2 | Enters the box; depth is visible | `selection.inside` | Exit always reachable |
| 3 | Manipulates the child with the same shared verbs | `selection.selected` | No new vocabulary |
| 4 | Exits back to the box level | `selection.idle` | Depth is view state, not document state |

### Journey: `journey.connector_reanchor` — Point the arrow somewhere else

- **Realizes:** Connector distinct tools

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Selects a connector | `selection.selected` | Endpoint zones are its hit contribution |
| 2 | Drags an endpoint onto a different node | `selection.reanchoring` | Snap to preferred ports, free on boundary |
| 3 | Releases; the connector now belongs to the new pair | `selection.selected` | Moving either node keeps it attached |

---

## Critical alternate journeys

### Journey: `journey.multi_select.alt_mixed_verbs` — The selection cannot all do the same thing

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Selects a Frame (no `rotate`) together with a sketch (has `rotate`) | `selection.multi` | |
| 2 | The rotate affordance is absent and the reason is available | `selection.multi.limited` | Intersection rule; no silent partial apply |
| 3 | Deselects the Frame; rotate returns | `selection.multi` | Predictable, not magical |

### Journey: `journey.manipulate_any.alt_below_lod` — Too far out

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Viewport is zoomed past the cutoff | `manipulation.unavailable` | Uniform across kinds |
| 2 | Press does not grab anything; the UI says why | `manipulation.unavailable` | BR-N05 |

### Journey: `journey.delete_referenced` — Delete something an arrow points at

| Step | Beat | In-scene state | Notes |
|---|---|---|---|
| 1 | Deletes a node that a connector references | `selection.idle` | |
| 2 | The connector is invalidated per its own rule, visibly | `connector.invalid` | Never a crash or a dangling reference |
| 3 | Undo restores both the node and the connector | previous state | One entry |

---

## Bridge matrix

| Journey step | In-scene state | srs-ui state / control | Product rule / AC | Logic pointer |
|---|---|---|---|---|
| `journey.manipulate_any` 2 | `selection.selected` | shared bounds + handles | shared-verb invariant | deferred — REQ-08 iteration |
| `journey.multi_select` 2 | `selection.multi` | intersection affordances | mixed-selection rule | deferred |
| `journey.multi_select.alt_mixed_verbs` 2 | `selection.multi.limited` | absent verb + reason | BR-N10 | deferred |
| `journey.rotate_snap` 2 | `selection.rotating` | rotate gizmo + snap feedback | rotate verb | deferred |
| `journey.enter_group` 2 | `selection.inside` | depth indicator | enter/exit verb | deferred |
| `journey.connector_reanchor` 2 | `selection.reanchoring` | endpoint affordances | Connector tools | deferred |
| `journey.manipulate_any.alt_below_lod` 2 | `manipulation.unavailable` | unavailable indicator | BR-N05 | deferred |

---

## Anti-invent / out of journey

| Path / wish | Decision | Tracking |
|---|---|---|
| Auto-layout / constraint solving | Reject | srs-product out-of-scope |
| Component instances and variants | Reject | srs-product out-of-scope |
| Desktop-side manipulation UI in this iteration | Defer | needs multi-directional sync first |
| Keyboard-driven manipulation | Defer | no keyboard on the device; revisit with desktop |
| Text authoring on the device | Open | srs-product open questions |

---

## Superseded

_None yet._
