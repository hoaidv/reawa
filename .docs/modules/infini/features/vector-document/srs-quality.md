---
feature: vector-document
parent_req: [REQ-02]
version: 0.2.0
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
| Boundary ink transform | After rotate/scale (any inkScaleMode), boundary ink samples transform with group | Always |
| Enclose happy path | Contained ink → SmartGroup; enclose stroke kept as boundary ink; bounds = fitted (x,y,w,h) | Fixture pass |
| Enclose miss / false | Undo or dismiss; ink samples intact | 100% channels |
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
| smart.group / enclose | STORY later (pilot design) | REQ-04 / SRS-IN-10 |

### A11y / resilience

- DocChrome controls keyboard-focusable.
- Error text not color-only (icon or prefix “Error:”).
