---
feature: tool-modes
parent_req: [REQ-03]
version: 0.1.0
lifecycle: active
---

# SRS — Tool modes Epaper (Quality)

## [SRS-EP-06] Tool responsiveness and ink-latency protection

### Quality-attribute scenarios

| Scenario | Stimulus | Response measure |
|---|---|---|
| Tool switch latency | Finger taps a tool | Active indicator updated **p95 ≤300 ms**; partial refresh only |
| **Ink latency non-regression** | Draw with `pen` after the toolbar ships | p95 ≤30 ms pen-down → pixel — **equal to the pre-toolbar baseline within measurement error** ([SRS-EP-01](../local-pen-ink/srs-logic.md)) |
| Tool-independent ink path | Draw the same stroke in `pen` and in `ink_box` | Identical local raster and identical `stroke_point` payloads; only `stroke_begin.intent` differs |
| Chrome exclusion | Pen stroke started on the ToolStrip | **0** ink pixels drawn there; 0 strokes emitted |
| Refresh isolation | Switch tools repeatedly | 0 full-panel refreshes attributable to tool switching |
| Ghosted state legibility | Switch tools while a full refresh trails | Active tool identifiable from shape/fill alone on the un-settled frame |
| Enclose round trip | Draw an armed enclose over ink | Smart Group visible on panel **p95 ≤500 ms** after the Infini op ([REQ-03](../../prd.md#tool-modes)) |
| Selection pick feel | Pen down on a pickable | Local selection affordance **≤100 ms** — no network round trip |
| Ghost correction | Snapshot contradicts the local ghost | Authoritative geometry within one snapshot; no lingering ghost |
| Touch fallback | Touch layer unavailable | Device still inks; `pen` forced; the reason is visible, not silent |
| Session loss | Session drops mid-gesture | No crash, no lost local ink; unavailable tools shown as unavailable |

### Correctness ties

- Arming `ink_box` changes **only** the emitted `intent`; it must not alter sampling, the
  Round 19 map, stroke width, or the local paint path.
- The device never fits a rectangle, tests containment, or mutates a tree
  ([ADR-0013](../../../../adr/ADR-0013-ink-box-tool-modes.md) §3).
- `pickables` is treated as advisory cache: a stale entry causes a wasted `tool_intent`, never a
  wrong local mutation.

### A11y / resilience

- Tool targets ≥120 px on the shorter axis (finger, no hover, no cursor).
- Active state distinguishable without color **and** without motion — 1-bit panel.
- `pen` is always reachable in ≤1 tap from any state; the creator can never be trapped in a
  non-drawing tool.
- With no session, the device degrades to today's behaviour (local ink) rather than erroring.

### Dual-ask table (state → Designer + QA)

| State id | Designer scene / Spec | QA AC / BDD |
|---|---|---|
| `tool.pen` | tool-modes package | REQ-03 default + ink latency |
| `tool.ink_box` | tool-modes package | enclose intent emitted; ink path identical |
| `tool.selection.idle` | tool-modes package | pen inert on canvas |
| `tool.selection.selected` | tool-modes package | bounds + handles from `pickables` |
| `tool.selection.dragging` | tool-modes package | ghost, then snapshot authority |
| `tool.selection.empty` | tool-modes package | unavailable, stated |
| `session.down` | tool-modes package | pen still inks |
| `touch.unavailable` | tool-modes package | fallback to pen; reason visible |
