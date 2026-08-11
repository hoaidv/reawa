---
feature: vector-document
parent_req: [REQ-02]
version: 0.2.0
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

### Journey: `journey.smart_group` — Ink-box (pilot)

- **Realizes:** [REQ-04](../../prd.md#smart-group) · [ADR-0011](../../../../adr/ADR-0011-smart-group.md)
- **Steps:**
  1. Handwrite ideas (Epaper/Infini) · beat: ink samples on canvas
  2. Draw rectangle around ink **or** select ink → Smart Group · beat: preview
  3. Accept → Smart Group: geometric bounds from enclose; **enclosure stroke kept as boundary ink**; content ink reparented
  4. Move / scale / rotate — **boundary ink always transforms**; toggle `fixedInk` vs `withBounds` for **content** ink only · beat: text-box feel
  5. Attach connector to edge midpoint of **bounds** · beat: glue tracks geometric box, not wiggly path

## Critical alternate journeys

| Journey | Trigger | Outcome |
|---|---|---|
| `journey.open_error` | Bad file | `doc.error`; no silent wipe |
| `journey.abandon_dirty` | Close with dirty | v0: OS/Electron confirm or keep dirty (open option — see srs-ui) |
| `journey.enclose_miss` | Recognition fails | Explicit Smart Group; ink untouched |
| `journey.enclose_false` | Bad propose | Dismiss/undo; ink untouched |

## Bridge matrix

| Journey step | scene / state | UI | Product AC | Logic |
|---|---|---|---|---|
| doc_new_open_save.* | doc.none/open/dirty/error | SRS-IN-05 | BR-07 | SRS-IN-04 open/save |
| tree_compose.* | doc.open (+ canvas) | chrome + WorldLayer | BR-01…06 | SRS-IN-04 tree · ADR-0010 |
| smart_group.* | doc.open (+ canvas) | selection / bounds handles (design) | REQ-04 · BR-09 | SRS-IN-10 · ADR-0011 |
| round_trip.* | doc.open | — | REQ-02 px / ops | SRS-IN-06 · srs-data |

## Anti-invent / out-of-journey

- No layers panel suite, brush library, or component variants in v0 journeys.
- Do not invent cloud share or multiplayer presence.
- Connector auto-routing algorithms beyond a simple path are out of journey scope.
- Do not invent OCR / “convert ink to Text” in Smart Group journeys.

## Designer co-session

PM authored narrative from human brief (tree-of-vectors). Designer validates scene honesty
when painting STORY-IN-006; adopt chrome composition into `srs-ui` if Spec differs.
