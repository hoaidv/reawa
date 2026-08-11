---
feature: vector-document
parent_req: [REQ-02, REQ-04]
version: 0.3.0
lifecycle: active
---

# SRS — Vector document (Quality)

## [SRS-IN-06] Fidelity, structure, and dual-ask

### Quality-attribute scenarios

| Scenario | Metric | Target |
|---|---|---|
| Save SVG → reopen (mixed tree) | Vertex / box / anchor error @ 100% zoom | ≤ 1 CSS px |
| Ink channel fidelity | pressure/tilt/extras present after reopen/transmit | 100% of channels that were on the fixture samples |
| Transmit encode → decode | Op equality on golden fixture set | 100% |
| Parenting preserved | Node `id` + parent/child relations | Exact match after round-trip |
| Connector endpoints | Resolved `from`/`to` node ids after reopen | 100% on valid fixtures |
| Preferred ports | Rect edge midpoints / ellipse cardinals round-trip as `port` | Exact enum match |
| Free boundary | Edge `t` / circumference angle round-trip | Param error ≤ 1e-6; world ≤ 1 px @ 100% |
| Glue on move | After translate/resize target, resolved anchor stays on boundary | Always |
| Smart Group round-trip | bounds, transform, inkScaleMode, ink samples | Exact + ≤1 px geom |
| `fixedInk` vs `withBounds` | After non-uniform scale: content ink fixed vs scaled; **boundary ink always scales** | Mode-correct |
| `fixedInk` per-ink UV | Two content inks with distinct UVs; after scale `s`, each UV preserved (±1 px @ 100%); sample sizes unchanged; no cross-ink move | Always |
| Boundary ink transform | After rotate/scale (any inkScaleMode), boundary ink samples transform with group | Always |
| Enclose happy path | Contained ink → SmartGroup; enclose stroke kept as boundary ink; bounds = fitted (x,y,w,h) | Fixture pass |
| Enclose miss / false | Undo or dismiss; ink samples intact | 100% channels |
| Enclose only when armed | Identical stroke drawn with `intent: ink` | **0** Smart Groups created |
| Selection create with surround | Open or closed surround among selection; ≥80% of others inside | SmartGroup with that stroke as `boundary` |
| Selection create refuse | Selection with no surround at ≥80% | **0** Smart Groups; selection unchanged |
| Draw-into membership | Pen stroke ≥80% inside one SmartGroup | Stroke is `content` of that box; siblings unmoved |
| Draw-into nested tie-break | Nested SmartGroups both ≥80% / 100% | Membership → highest paint/z order; 0 dual parents |
| Draw-into miss | Pen stroke <80% inside every SmartGroup | Stroke stays ordinary parent ink |
| Enclose guards | Fitted rect < 48 world units, or 0 ink inside | 0 creations on the negative fixture set |
| Enclose first-try rate | 20 scripted intentional enclose gestures | ≥80% create the intended Smart Group first try |
| Enclose latency | Armed `stroke_end` → Smart Group on canvas | p95 ≤300 ms |
| Undo exactness | Undo after any structural op | Tree snapshot string equals the pre-op snapshot **exactly** |
| Undo depth | 21 consecutive structural ops | Oldest drops; undo past the ring is a no-op, not an error |
| Undo memory | 20 snapshots of a 10k-point document | Advisory ≤50 MB retained on a reference Mac |
| Pick correctness | Pointer inside overlapping SmartGroup bounds | Topmost (last sibling) selected, 100% |
| Pick vs pan | Drag inside bounds at scale ≥0.35 | Node moves; viewport translate unchanged (0 px drift) |
| LOD guard | Drag inside bounds at scale <0.35 | Canvas pans; no node mutated; unavailability is visible |
| Gesture op economy | One drag of N frames | Exactly **1** `set_smart_transform` emitted (on release) |
| Rotation not exposed | Any pilot UI surface | 0 rotation affordances (anchors resolve translate+scale only) |
| Invalid connector apply | No crash; connector marked invalid | Always |
| Open failure | Prior tree bytes unchanged in memory | Always |
| Large ink reopen | 10k-point ink polyline open | ≤ 2 s cold open on reference Mac (advisory) |

### Correctness ties

- Flatten visitor emits every Ink/Text/Primitive/Connector drawable exactly once per paint
  (no duplicate leaves from group walks).
- Frames at non-root rejected on load (error or skip-with-error — **fail closed**).

### Dual-ask table (state → Designer + QA)

| State id | Designer scene / Spec | QA AC / BDD |
|---|---|---|
| `doc.none` | STORY-IN-006 package | open/save feature |
| `doc.open` | STORY-IN-006 | open/save + tree fixture visible |
| `doc.dirty` | STORY-IN-006 | dirty indicator |
| `doc.error` | STORY-IN-006 | error + canvas unchanged |
| tree round-trip | N/A (logic) | SRS-IN-06 scenarios / fixtures |
| `tool.selection` / `tool.ink_box` | ink-box design package | REQ-04 tool arming |
| smart_group.created (boundary ink) | ink-box design package | REQ-04 / SRS-IN-10 · SRS-IN-16 |
| smart_group.select_refuse | ink-box design package | REQ-04 BR-09j |
| smart_group.selected (handles) | ink-box design package | REQ-04 / SRS-IN-11 |
| smart_group.dragging | ink-box design package | pick vs pan |
| smart_group.ink_scale_mode | ink-box design package | `withBounds` vs `fixedInk` + per-ink UV |
| smart_group.draw_into | ink-box design package (optional beat) | REQ-04 / SRS-IN-15 |
| manipulation.unavailable (below LOD) | ink-box design package | LOD guard |

### A11y / resilience

- DocChrome controls keyboard-focusable.
- Error text not color-only (icon or prefix “Error:”).
