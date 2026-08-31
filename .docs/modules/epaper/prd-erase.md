---
title: PRD — Erase like paper
module: epaper
req: [REQ-11]
lifecycle: active
owner: pm
date: 2026-08-29
traces: [BRD-07]
amends: [REQ-03, REQ-10, REQ-11, REQ-18, REQ-20]
source: .plan/iter-005/eraser-product.md
challenge: [CHL-0028]
---

# PRD — Erase like paper

**This is the canonical product specification for [REQ-11](./prd.md#erase).** Do not fork into a second PRD, srs-product, or srs-experience. UI/UX in this file is normative. Architect SRS binds algorithms and millimetre→du constants; it does not invent journeys.

**Job:** when a mark is wrong, it is gone on the tablet the way pencil-on-paper works — stroke it away, wipe a region, or lift whole objects — without a desktop round trip. One completed gesture is one undo.

**Priority:** Must. **Traces:** [BRD-07](../../brd.md). **Needs design:** icons only this wave (three 1-bit ToolChip glyphs). Interaction chrome is specified here; no separate design package.

Adopted [CHL-0028](../../../.plan/iter-005/challenges/CHL-0028-eraser-three-tools.md) 2026-08-29. **Do not implement** the old REQ-11 Path A / Path B text.

---

## 1. What this replaces

| Old draft (REQ-11 Path A / B) | This specification |
|---|---|
| No exclusive eraser on the chip | Three exclusive eraser tools |
| Path A: hardware nib deletes **samples**; **0** new Ink nodes | Brush (and area) **clip** polylines at geometry; remnants become **new nodes** |
| Path B: Erase command deletes the **current selection** | **Retired.** No “erase selected nodes” CTA |
| Barrel `temp_erase` = Path A feel | Barrel = **last-used eraser** (see §12) |
| Nib never steals the exclusive tool | Nib **is** brush while inverted (see §12) |

---

## 2. Three exclusive tools

One **Eraser** interaction mode in the tool system; three chip ids; three operations. Product sees three tools. They toggle exclusively with `sel_rect`, `sel_freeform`, and `pen`.

| Chip id | Name | Gesture | Commit |
|---|---|---|---|
| `erase_brush` | Brush eraser | Primary down + move | Clip **Ink** against a **capsule** along the path |
| `erase_area` | Area eraser | Primary down + move (freeform) | Clip **Ink** inside the closed polygon; **remove** other kinds if **fully inside** |
| `erase_object` | Object eraser | Primary down + move (freeform) | Remove whole nodes that pass the **80%** test |

Order on the chip: **brush | area | object**.

**Frame** is never destroyed by any eraser (brush no-op; area does not remove even if fully inside; object never). Deliberate.

---

## 3. ToolChip

```text
[ HT | sel_rect | sel_freeform | pen ]
  ⟨gap⟩
[ ink-box | connector ]
  ⟨gap⟩
[ brush | area | object ]
  ⟨gap⟩
[ undo | redo ]
```

| Control | Kind | Notes |
|---|---|---|
| `btn.hand_touch` (HT) | **Toggle**, not exclusive | Same [REQ-10](./prd.md#hand-touch) kill-switch. **Moves onto** this chip as the first control. Trailing sibling HT tile is **removed** (one HT, not two). |
| `sel_rect`, `sel_freeform`, `pen` | Exclusive | Unchanged meaning |
| `tgl.recog.ink_box`, `tgl.recog.connector` | Toggles | Independent; ship armed |
| `erase_brush`, `erase_area`, `erase_object` | Exclusive | Own group |
| Undo / Redo | Actions | Unchanged |

**Recognizers while any eraser is armed:** **dimmed** (armed state kept). An erase stroke never runs enclose or connector recognition.

Exclusive tools are **six**: two selection + pen + three erasers. Viewport-follow, Device Settings entry, and Debug stay **siblings** of the chip, not tiles on it.

Default on launch remains `pen`. Last-used eraser starts as `erase_brush` until the creator arms another eraser.

---

## 4. Pointer routing

| Role | Eraser armed | Ink / Selection armed |
|---|---|---|
| **Primary** (default: pen) | The armed erase operation only | Unchanged (ink / lasso / marquee / move / resize) |
| **Secondary** (default: finger) | **Navigation only** — no Select, Move, Resize | Unchanged (REQ-10 pick / move / pan / pinch, subject to HT) |

Erase operations **accept Primary only**. Finger does not brush-, area-, or object-erase unless DeviceMap later makes Finger Primary.

Pen on the chip is still not ink and not erase (exclusion rect = chip bounds).

---

## 5. Size and zoom

| Quantity | Value | Space |
|---|---|---|
| Brush diameter (v1, fixed) | **8 mm** | **World** |
| Capsule radius | **4 mm** | World |
| Remnant floor | **1 mm** arc length | World |
| Hover-circle stroke | **0.5 mm**, black | World |
| Hover-circle fill | White | — |

**Zoom:** size is **world-constant**, same rule as ink. When the world is zoomed in and nodes look larger, the eraser preview **looks larger too**. (Not a constant millimetre on the glass.)

Architect converts mm → du in SRS (panel **226 dpi**; ≈ 8.90 du / mm). v1 has **no** on-chip size control.

The hover circle is specified **only for brush**. Easy to change later without touching area/object.

---

## 6. Shared: preview vs commit

- **During** the gesture: interaction chrome on **ToolCanvas** only. Do **not** restroke or rewrite the document tree on every move.
- **At pointer-up:** one document commit. One undo. Inverse-op ring ([ADR-0032](../../adr/ADR-0032-inverse-op-undo.md)); never `restore_snapshot`.
- Empty gesture (no down-move that changes anything, or nothing hit): **no-op**, 0 undo entries.
- No session: same local result; publish when linked.

---

## 7. Brush eraser

### 7.1 Proximity circle (pen only)

Ship **on**. Applies when Primary is **pen** and the pen is **near**, including both:

- pen **enters** proximity, and
- pen goes from **down → up into** proximity.

| Spec | Value |
|---|---|
| Shape | Circle at the pen tip |
| Diameter | Eraser size (8 mm world) |
| Stroke | 0.5 mm black |
| Fill | White |
| Erase? | **No** — representation only until down |

Field-test **circle on vs off** (e-ink refresh). Keep a kill-switch. Ghost-while-down is **Must**; the circle is **Should** with an off path.

### 7.2 While down

Draw a **white ghost polyline** on ToolCanvas, stroke width = eraser size, along the gesture. It covers document ink visually (erase feel). Do not mutate the tree yet.

### 7.3 Pointer up — ghost teardown

**Drop the ghost in the same refresh as the committed document damage.** Do not clear the overlay before the hole is painted (that flashes old ink back).

### 7.4 Commit

- Delete region = **capsule** of radius 4 mm along the gesture in world space.
- **Clip** each affected **Ink** polyline against that region (geometry, not “delete sample points and keep one array”).
- Do not round the clip area just to the ink samples; compute **intersection** between ink polylines and the eraser capsule. High-precision eraser.
- **Kinds:** Ink only, including `role: content` and `role: boundary` ink under a SmartGroup. Primitive, Text, Frame, Connector: **no-op**.
- Invisible SmartGroup **boundary polyline:** **never** clipped.
- Remnants: see §10.

---

## 8. Area eraser

### 8.1 While down

**Dotted polyline** on ToolCanvas. No white cover, no mask of document content.

### 8.2 Commit

- Treat the gesture as a **closed polygon**: auto-close last→first (same as freeform select). Stored overlay samples stay as drawn; close is for the test/clip only.
- **Open freeform → auto-close. No minimum area.**
- **Kinds**
  - `Ink`: clip polylines to the **interior**, **even-odd** fill. Geometric intersection with the eraser polygon — not sample-rounding.
  - **Other kinds** (Primitive, Text, SmartGroup, Connector, Group): **remove** the node if it is **fully inside** the closed polygon. SmartGroup fully inside → whole group gone. Connector fully inside → remove; attachments follow §9.3.
  - **Frame:** never removed (see §2).
- Brush still does **not** clip or convert connectors. Area does **not** clip connector ink; it only **removes** a connector that is fully inside.
- Invisible boundary polyline: never clipped.
- Ink remnants: see §10.

---

## 9. Object eraser

### 9.1 While down

- Dotted polyline on ToolCanvas (same close rule as area: auto-close last→first, no minimum area). Paint the **drawn** samples; a coarsened copy is for hit-test only.
- **Highlight (deletion-rect):** a second outline on ToolCanvas of each candidate that currently passes the 80% test. Outline = that node’s **AABB** (selection-style), not a second copy of the ink path. Stroke **2.5 panel pixels**, cosmetic (unchanged apparent width when the camera zooms). Live 80% is **not** on the UI thread: one in-flight compute, at most one queued latest lasso (newer events replace the waiting job). Overlay skips Ink and Connector. SmartGroup live/commit 80% uses a **downsampled boundary polyline** (the product table is still boundary area, not the fitted AABB).
- Never restroke the document during the gesture.
- No cover/mask of document content.

### 9.2 80% test (closed)

Object eraser does **not** reuse freeform-select’s sample-count / grid tests. Select stays as shipped. Object erase uses this table only.

| Kind | Remove the node when | Otherwise |
|---|---|---|
| **Ink** | ≥80% of **arc length** lies inside the lasso (even-odd) | Leave it |
| **SmartGroup** | ≥80% of **polygon area** of the **invisible boundary polyline** lies inside the lasso | Leave the group (children untouched by this test) |
| **Primitive**, **Text** | ≥80% of **world AABB area** lies inside the lasso | Leave it |
| **Connector** | ≥80% of **warped `V` length** lies inside the lasso | Leave it |
| **Frame** | **Never.** Object eraser cannot destroy a Frame. Deliberate. | — |

Passing nodes are **removed whole**. **No leftovers**, no clip, no remnant nodes.

SmartGroup that passes is removed **as one object** (the group and its ink children). Connectors that targeted it follow existing missing-end rules (stay; last live pose).

### 9.3 Connector extras

When a connector is **removed** (object erase 80%, or area fully-inside):

- The connector node is gone (warp, rest spine, terminals, endpoint decoration if any — gone with it).
- **Attachments keep existing** as ordinary world nodes: **unbind**, **reparent** to the connector’s parent, paint order adjacent to where the connector was, pose = **last derived world pose**. Undo rebinds.
- This wave does **not** convert a connector into Ink. (Endpoint-ink as a product is [REQ-13](./prd.md#connector-ends); not required for eraser. Prefer fixed terminal styles.)

Brush: **0** mutations to connectors.

---

## 10. Remnant split (brush and area Ink clip only)

Ink is one **continuous polyline** per node. After a clip, disconnected pieces are **separate nodes**. Do not keep a gapped sample list (that draws a chord). Do not store multiple contours on one Ink node.

For each Ink the gesture hits:

| Remnants after clip (each ≥ **1 mm**) | Ops |
|---|---|
| 0 | `remove_node` |
| 1 | `set_ink_samples` on the original id |
| ≥2 | Original id keeps the **longest** remnant; each extra remnant is `append_ink` (same parent, same style, adjacent paint order) |

Whole gesture (all nodes) is **one** `compound` if more than one tree op. Tiny pieces below 1 mm are dropped (dust).

**Content ink** remnants stay `role: content` and **reseed** `layoutOffset` UV. Boundary **ink** remnants stay `role: boundary` (the surround may be broken; see §11).

Original id on the longest remnant: connectors/attachments that pointed at that Ink stay on that remnant. No per-fragment retargeting in v1.

---

## 11. Ink-box (SmartGroup)

A box has three geometric facts:

| Fact | Visible? | Erase |
|---|---|---|
| Content ink | Yes | Brush/area **clip** (split remnants) |
| Boundary ink | Yes | Brush/area **clip** (split remnants; **broken surround allowed**) |
| **Boundary polyline** | **No** | **Never clipped.** Seeded at create as a closed copy of the enclose stroke. Transforms and resizes **with boundary ink**. **Persisted** (device + Infini). Used for object-erase **area**. |

**Keep the box as a coherent unit.** Do not split one ink-box into several this wave. No “invalid box” cleanup for a broken visible surround.

**Last visible ink gone → remove the ink-box.** The boundary polyline is not content and not boundary ink. If no visible ink remains, the group is removed (existing last-child rule, counting only ink children). The polyline does not keep an empty box alive.

Object erase: 80% **area of the boundary polyline**; if it hits, the **whole** SmartGroup goes.

Area erase: SmartGroup **fully inside** the polygon → whole group gone (not an 80% test).

---

## 12. Barrel, last-used, nib

**Last-used eraser** = the last of `{erase_brush, erase_area, erase_object}` the creator armed. Default **brush**. Persist on **this device** with [REQ-20](./prd.md#device-settings) (not the document, not Infini). No extra Settings master row this wave.

| Accelerator | Behaviour |
|---|---|
| Barrel **Click** `toggle_pen_eraser` | Exclusive tool toggles **ink (`pen`) ↔ last-used eraser**. Not Path B. Not the nib. |
| Barrel **Hold-move** `temp_erase` | From **ink-mode only:** temporarily switch to last-used eraser until release, then restore (unless the event was a Click). Chip **mirrors** that eraser. Hold while **already in an eraser:** **no-op**. |
| **Eraser nib** (inverted tool) | Treat as **brush**: preview + brush commit. Chip shows `erase_brush` while inverted; restore previous exclusive on un-invert. Hardware unverified this environment — spec stands; do not ship-gate the rest of eraser on a nib fixture. |

REQ-18 catalogue **defaults unchanged in shape:** 1-button Click = pen ↔ `sel_freeform`, Hold-move = `temp_erase`; 2-button B2 Click = pen ↔ last-used eraser, B2 Hold-move = `temp_erase`. Labels/help text must say **last-used eraser**, not “delete selection.”

Nib is still a **different HID channel** from barrel ([ADR-0025](../../adr/ADR-0025-barrel-vs-eraser-nib.md)).

---

## 13. Undo, sync, quality

| Bar | Measure |
|---|---|
| Brush / area / object settled | p95 ≤ **50 ms** after pointer-up until the tree matches the clip/remove |
| Ghost teardown | Same refresh as document damage (no restored-ink flash) |
| One gesture | **One** undo; skip/no-op per [REQ-04](./prd.md#device-document) / [ADR-0032](../../adr/ADR-0032-inverse-op-undo.md) |
| Geometry | ±1 px @ 100% zoom vs pre-erase when `lastOpId` matches |
| Pen-tip ink | [REQ-01](./prd.md#local-pen-ink) unchanged (eraser must not sit on the pen-ink hot path) |
| Tool switch | p95 ≤ **300 ms** chip indicator |
| Chord across a hole | **0** |
| New Ink from brush/area clip | Only remnant `append_ink` as in §10 — never a new stroke from the eraser gesture itself |
| Connector from **brush** | **0** mutations |
| Frame from any eraser | **0** removals |
| No session | Same local result |

Publish: `set_ink_samples`, `append_ink`, `remove_node`, `reparent` (attachments), `compound` as needed. Never `restore_snapshot` for erase.

---

## 14. Acceptance

**Brush**

- Given `erase_brush` and world ink, When Primary draws a capsule across a stroke, Then intersecting geometry is gone, remnants ≥1 mm are separate Ink nodes (longest keeps the original id), p95 ≤50 ms after up, and one undo restores the pre-erase tree (±1 px @ 100% zoom; skip/no-op per REQ-04).
- Given zoom-in so ink looks larger, When the hover circle and ghost are shown, Then they scale with the same world diameter (8 mm), not a constant panel millimetre.
- Given pen near and `erase_brush`, When the pen is in proximity (enter or up→near), Then a white-filled circle (0.5 mm black stroke, 8 mm diameter) follows the tip and **0** samples are deleted until down.
- Given a connector or Frame under the capsule, When the gesture commits, Then those nodes are unchanged.

**Area**

- Given `erase_area` and an open freeform across ink, When pointer-up, Then the polygon auto-closes, even-odd interior ink is clipped, remnants follow §10, and 0 minimum-area refusal.
- Given `erase_area` and a connector (or Primitive / Text / SmartGroup) **fully inside** the closed polygon, When commit, Then that node is removed (SmartGroup as a whole; connector attachments unbound per §9.3).
- Given `erase_area` and a connector only **partially** inside, When commit, Then the connector is unchanged (no clip).
- Given a Frame fully inside the area polygon, When commit, Then the Frame is still there.

**Object**

- Given `erase_object` and a lasso containing ≥80% **arc length** of an Ink, When commit, Then that Ink is gone (0 remnant nodes for that id) and one undo restores it.
- Given a SmartGroup whose boundary-polyline area is ≥80% inside the lasso, When commit, Then the **whole group** is gone.
- Given a SmartGroup whose area is &lt;80% inside, When commit, Then the group and children are unchanged (even if some child ink is inside the lasso).
- Given a Connector with ≥80% of warped `V` length inside, When commit, Then the connector is removed, each attachment is a world node at last pose (unbound, parent = former connector parent), and one undo restores connector + binds.
- Given a Frame ≥80% inside the lasso, When commit, Then the Frame is still there.

**Ink-box**

- Given brush/area across visible boundary ink, When commit, Then boundary ink may split and the surround may be broken; the **boundary polyline is unchanged**; the SmartGroup remains if any visible ink remains.
- Given brush/area that removes **all** visible ink of a SmartGroup, When commit, Then the SmartGroup is **removed**.

**Chip / barrel**

- Given the ToolChip, When shown, Then HT is a toggle in the first cluster, then the three exclusives, gap, two recognizers, gap, three erasers, gap, Undo/Redo; 0 second HT tile on the trailing row.
- Given any eraser armed, When the chip is shown, Then both recognizers are dimmed.
- Given last-used = area, When barrel Click `toggle_pen_eraser` from `pen`, Then exclusive tool becomes `erase_area`.
- Given `pen` and Hold-move `temp_erase`, When hold-move, Then last-used eraser runs until release and the chip mirrors it; 0 click toggle on that release.
- Given an eraser already armed, When Hold-move `temp_erase`, Then 0 tool change (no-op).

**Retired Path B**

- Given a non-empty selection, When looking for an Erase-selected-nodes control, Then it does not exist (Cut remains).

**Offline**

- Given no session, When any of the three erasers commits, Then the local tree matches the linked case.

**UI states** (1-bit, partial refresh; specified here — Designer this wave is **three glyphs only**): six exclusives + HT toggle on one chip; three eraser glyphs distinct after a trailing refresh; brush proximity circle; brush ghost in progress; ghost teardown with document; area dotted freeform; object dotted freeform + AABB highlights; recognizers dimmed; barrel mirror last-used; nib-as-brush; empty no-op; undo after each eraser.

Human is QA this wave (no BDD ceremony required before implement).

---

## 15. Non-goals

- On-chip eraser **size** slider (v1 fixed 8 mm).
- Splitting one SmartGroup into several boxes.
- “Invalid box” repair when boundary ink is broken.
- Brush/area **convert** of connectors to Ink; brush/area **holes** in a live connector (area may only **remove** if fully inside).
- Erase of **Frame**.
- Path B erase-selected-nodes.
- Exclusive **Hand** mode (HT stays a toggle).
- Changing freeform-**select** 80% tests (object erase has its own table).
- Retiring [REQ-13](./prd.md#connector-ends) endpoint-ink (separate adopt if desired).
- A `Dimension` type inside the document core (world numbers are millimetres; paint converts — Architect SRS).
- Bézier fitting, OCR, a second Infini undo stack.
- Full ToolChip hi-fi package / scene HTML this wave (icons only).

---

## 16. Tool-system note (binding)

- **One** `Eraser` Mode class; chip ids `erase_brush` | `erase_area` | `erase_object` choose the Operation (same pattern as SelectionMode + `sel_rect` / `sel_freeform`).
- Operations: `BrushErase`, `AreaErase`, `ObjectErase` — `acceptPrimary` only; Secondary allow-list = Navigation.
- Live ink stays TabletCanvas; all erase chrome is ToolCanvas / `ToolContext`.
- Commit builds `DocEdit`s (`set_ink_samples`, `append_ink`, `remove_node`, `reparent`, `compound`) through `DocContext`.
- Decision: [ADR-0034](../../adr/ADR-0034-erase-clip-remnants.md) (clip + remnant split).
