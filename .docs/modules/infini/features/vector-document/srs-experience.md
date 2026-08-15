---
feature: vector-document
parent_req: [REQ-02, REQ-04]
version: 0.3.0
lifecycle: active
owner: pm
co_author: designer
purpose: PRD → technical bridge — open/save + tree-aware document journeys
---

# SRS — Vector document (Experience)

## Capability narrative

Infini’s document feels like a **living sketchbook on an infinite desk**: handwriting
arrives as ink, you type a caption, group related marks, drop a frame around a topic, and
draw a connector from an idea to a shape — then open/save that whole tree without losing
structure.

## Entry context

| Field | Value |
|---|---|
| Persona / role | Creator (primary) |
| Situation / when | After canvas navigation works; starting or continuing a sketch session |
| Trigger | App launch with no doc; Open…; New; Save; tablet strokes appending |
| Preconditions | Infini window up; optional Epaper session ([ADR-0009](../../../../adr/ADR-0009-shared-document-viewport.md)) |

## Primary journeys

### Journey: `journey.doc_new_open_save` — New / open / save

- **Realizes:** [REQ-02] UI states + product BR-07
- **Steps:**
  1. Launch → `doc.none` (empty world, open/new affordance) · beat: discover CTA
  2. New or Open success → `doc.open` · beat: title/path in chrome
  3. Edit tree (ink/text/group/…) → `doc.dirty`
  4. Save success → `doc.open` clean
  5. Open failure → `doc.error` · beat: inline error; canvas unchanged

### Journey: `journey.tree_compose` — Build tree content (desktop)

- **Realizes:** [REQ-02] tree model · [ADR-0010](../../../../adr/ADR-0010-tree-of-vectors.md)
- **Steps:**
  1. On `doc.open`, ink already present or drawn (Epaper/Infini)
  2. Add text box / primitive (desktop) · beat: leaf appears in world
  3. Multi-select → Group · beat: children nest under Group anywhere in tree
  4. Create Frame at root · beat: frame bounds; optional move children into frame
  5. Add Connector between two nodes · beat: link visible; ids bound

### Journey: `journey.round_trip` — Persistence confidence

- **Realizes:** REQ-02 acceptance round-trip
- **Steps:** Save → quit/reopen or Open same file → tree matches (±1 px)

### Journey: `journey.smart_group_enclose` — Ink-box by enclosure (primary)

- **Realizes:** [REQ-04](../../prd.md#smart-group) · [epaper REQ-03](../../../epaper/prd.md#tool-modes) · [ADR-0011](../../../../adr/ADR-0011-smart-group.md)
- **Steps:**
  1. Handwrite ideas in `Pen` mode (Epaper) · beat: ink samples on canvas, nothing groups
  2. Switch to `Ink-box` (tablet toolbar or Infini toolbar) · beat: tool reads as armed
  3. Draw a rectangle around the writing · beat: it is still just ink under the pen
  4. Stroke ends → Infini recognizes, guards pass → Smart Group: bounds from the fitted rect;
     **enclosure stroke kept as boundary ink**; contained ink reparented as content
  5. Tablet settles → the box reads the same on both devices · beat: one object now
- **Note:** the creator never confirms a proposal — arming the tool *was* the intent.

### Journey: `journey.smart_group_select` — Ink-box from a selection (fallback)

- **Realizes:** [REQ-04](../../prd.md#smart-group) · BR-09j · [SRS-IN-16](./srs-logic.md#srs-in-16-selection-create-surround)
- **Steps:**
  1. Switch to `Selection` · beat: pen/pointer now picks instead of draws
  2. Select the surround stroke **and** the ink it should contain · beat: selection reads clearly
  3. Invoke Smart Group · beat: surround → `boundary`; others → `content`; bounds from surround
  4. If no surround qualifies · beat: create refused; selection unchanged; reason visible
- **Note:** the surround stroke may be open; containment uses an artificial closed path for the test only.

### Journey: `journey.smart_group_manipulate` — Live like a text box

- **Realizes:** [REQ-04](../../prd.md#smart-group) manipulation scope
- **Steps:**
  1. In `Selection`, press inside a box and drag · beat: it moves; the canvas does not pan
  2. Press it once to select · beat: resize handles appear on the geometric bounds
  3. Drag a handle · beat: **boundary ink always transforms** with the bounds
  4. Toggle `fixedInk` vs `withBounds` · beat: content pads with centroid UV tracking vs scales
  5. Press empty canvas to deselect; press another node to select it instead

### Journey: `journey.smart_group_draw_into` — Keep writing inside a box

- **Realizes:** [REQ-04](../../prd.md#smart-group) · BR-09g · BR-09h · [SRS-IN-15](./srs-logic.md#srs-in-15-draw-into-membership)
- **Steps:**
  1. An ink-box already exists · beat: boundary ink or subtle hint
  2. Switch to `Pen` and write more inside the box · beat: ink appears under the pen as usual
  3. Stroke ends → Infini parents the stroke as content of that box · beat: it moves with the box later
  4. Existing handwriting inside is untouched — no reflow/alignment · beat: free layout
- **Nested variant:** when nested boxes both contain the stroke, membership goes to the highest
  paint/z-order box (later sibling / topmost).

## Critical alternate journeys

| Journey | Trigger | Outcome |
|---|---|---|
| `journey.open_error` | Bad file | `doc.error`; no silent wipe |
| `journey.abandon_dirty` | Close with dirty | v0: OS/Electron confirm or keep dirty (open option — see srs-ui) |
| `journey.enclose_miss` | Armed enclose not recognized as a rect | Nothing groups; stroke stays ink; fall back to `journey.smart_group_select` |
| `journey.enclose_empty` | Armed enclose contains no ink, or is below minimum size | Nothing groups; stroke stays ink; **no error banner** |
| `journey.enclose_false` | Grouped something the creator did not mean | One undo restores the prior tree; ink samples intact |
| `journey.select_create_refuse` | Selection has no surround stroke at ≥80% | Create refused; selection unchanged |
| `journey.manipulate_below_lod` | Drag a box at scale <0.35 | Manipulation unavailable and said so; drag pans instead |
| `journey.draw_into_miss` | Pen stroke <80% inside every Smart Group | Stroke stays ordinary root ink |
| `journey.tool_stale_refresh` | Tool switched while the panel refresh trails | Active tool still legible on the strip (partial refresh) |

## Bridge matrix

| Journey step | scene / state | UI | Product AC | Logic |
|---|---|---|---|---|
| doc_new_open_save.* | doc.none/open/dirty/error | SRS-IN-05 | BR-07 | SRS-IN-04 open/save |
| tree_compose.* | doc.open (+ canvas) | chrome + WorldLayer | BR-01…06 | SRS-IN-04 tree · ADR-0010 |
| smart_group_enclose.* | doc.open (+ canvas) | tool strip, armed state (design) | REQ-04 · BR-09, BR-09a…c | SRS-IN-10 · ADR-0011 |
| smart_group_select.* | doc.open (+ canvas) | selection + create/refuse affordance (design) | REQ-04 · BR-09j | SRS-IN-16 · ADR-0011 §4B |
| smart_group_manipulate.* | doc.open (+ canvas) | bounds handles + inkScaleMode toggle (design) | REQ-04 · BR-09f, BR-09i | SRS-IN-11 · ADR-0011 |
| smart_group_draw_into.* | doc.open (+ canvas) | (no extra chrome — freehand) | REQ-04 · BR-09g, BR-09h | SRS-IN-15 · ADR-0011 §7 |
| round_trip.* | doc.open | — | REQ-02 px / ops | SRS-IN-06 · srs-data |

## Anti-invent / out-of-journey

- No layers panel suite, brush library, or component variants in v0 journeys.
- Do not invent cloud share or multiplayer presence.
- Connector auto-routing algorithms beyond a simple path are out of journey scope.
- Do not invent OCR / “convert ink to Text” in Smart Group journeys.
- Do not invent an accept/dismiss proposal step — arming the tool is the confirmation.
- Do not invent rotation handles or connector glue on a Smart Group in pilot journeys.
- Do not invent in-box alignment, snap-to-margin, or reflow when appending ink.
- Do not invent a tool palette on Infini. Epaper only: `sel_rect` · `sel_freeform` · `pen` plus
  recognizer toggles. Infini ToolStrip is **removed** this campaign.

## Designer co-session

PM authored narrative from human brief (tree-of-vectors). Designer validates scene honesty
when painting STORY-IN-006; adopt chrome composition into `srs-ui` if Spec differs.
