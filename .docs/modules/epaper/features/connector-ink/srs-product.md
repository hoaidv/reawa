---
feature: connector-ink
parent_req: [REQ-09]
version: 0.2.0
lifecycle: active
owner: pm
---

# SRS — Connector-ink (Product)

PM feature depth for [REQ-09](../../prd.md#device-connectors). Geometry is
[ADR-0020](../../../../adr/ADR-0020-connector-ink-geometry.md). Dispatch is
[ADR-0022](../../../../adr/ADR-0022-recognizer-dispatch.md).

## Intent / JTBD

A creator boxes two thoughts and draws a line between them. The line should remain *their*
line, stay glued when a box moves, and not ask them to pick a routing tool. The job is
**keeping a handwritten diagram alive**, not authoring a graph.

## In scope / out of scope (feature grain)

| In scope | Out of scope |
|---|---|
| Auto-recognize single-stroke and chained connectors between SmartGroups | Manual connector tool; non-SmartGroup targets |
| Ink / Curve warp styles, auto-picked from the rest spine | Squared / rounded / obstacle routing; quadratic bezier |
| Live re-warp during bound-node drag | Physics rope; interior pins |
| Keep connectors when a bound box is deleted | Mark invalid / auto-delete the connector |
| Selection chrome: Ink/Curve; per-end Edge/Centre | Dash, double-line, width presets. **Endpoint styles / arrowheads / endpoint ink** → epaper [REQ-13](../../prd.md#connector-ends). **Mid-attachments** → [REQ-14](../../prd.md#connector-attachments) |
| One-shot blink of connector + both nodes on create | Persistent "recognized" badge; style-named toast |

## Business rules

| Rule id | Statement | Source |
|---|---|---|
| BR-C01 | Recognition requires **Connector recognition** armed, latched at pen-down. | D13, D15 |
| BR-C02 | Guards: open, path-like, long enough, two different SmartGroup bindings, body mostly outside. Fail → ordinary ink, no banner. | R2-I3 |
| BR-C03 | UX2 retro-chains at completion; no pending half-connector. Auto-pick Ink/Curve from the **merged** rest spine. | D6, D40 |
| BR-C04 | Style names: **Ink** = morph, **Curve** = cubic. Auto: ≤1 inflection on `S` → Curve; else Ink. Override on the selected connector. | D33, D37 |
| BR-C05 | Edge facing is the drawn departure, carried with the edge. Centre facing is drawn departure in a 60° cone about the peer ray. Edge never re-selected. | D26, D29 |
| BR-C06 | A node move/resize/rotate emits **no** connector op. Warp is a pure function of rest shape + endpoints + style. Live preview is ToolCanvas; CanvasLayer origin punch **includes the spine AABB**, not only the box. | D4, ADR-0020, [BR-B19](../ink-box/srs-product.md) |
| BR-C07 | Rest shape is never re-baked from a warped result. | D5 |
| BR-C08 | Delete of a bound box **keeps** connectors; missing node resolves from last live pose; undo glues back. | D39 |
| BR-C09 | Recognition is best-effort plus **one undo**. Chrome: blink connector + both nodes **once**. Does not name the style. | D38, BR-B09 analogue |
| BR-C10 | False-positive rate with both recognizers armed is a **ship gate** (≤2% of `pen` strokes, incl. first 20 of a fresh page). | D25 |
| BR-C11 | A recognized connector is selected by (a) `sel_rect` / `sel_freeform` with ≥80% of **path samples** inside the gesture, or (b) **pen-down on the stroke**. AABB-only intersection or AABB-only press does not select. | REQ-09 2026-08-15 |

## Edge cases

| Case | Expected |
|---|---|
| Connector recognition off | Stroke stays ink |
| One end misses a box | Ordinary ink |
| Same box both ends | Ordinary ink |
| Failed enclose then membership | Stroke may join a box (ADR-0022 step 2) — not a connector |
| Chord-flip / U-turn | Accepted for edge binds; creator switches that end to Centre |
| Both boxes deleted | Connector remains between two frozen poses, still selectable |
