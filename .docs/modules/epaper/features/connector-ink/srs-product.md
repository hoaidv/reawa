---
feature: connector-ink
parent_req: [REQ-09, REQ-13, REQ-14, REQ-17]
version: 0.3.0
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
| Selection chrome: Ink/Curve; per-end Edge/Centre | Dash, double-line, width presets. **Mid-attachments** → [REQ-14](../../prd.md#connector-attachments) |
| [REQ-13](../../prd.md#connector-ends) Path B: drawn ink on a connector end, on `ConnectorAnchor`, append, no toolbar | Path A closed-style chips on Epaper; Infini Path A this lock |
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
| Failed enclose then membership | Membership already ran (ADR-0022 step 2). A closed stroke inside a box joins as content — not a new box or a connector |
| Chord-flip / U-turn | Accepted for edge binds; creator switches that end to Centre |
| Both boxes deleted | Connector remains between two frozen poses, still selectable |

## [SRS-EP-74] Endpoint-ink product (Path B) {#srs-ep-74-endpoint-ink-product}

<!-- lifecycle: active -->

**Parent:** [REQ-13](../../prd.md#connector-ends). **Needs design: no.** Epaper has no Path A chrome. Logic/quality: [SRS-EP-35](./srs-logic.md#srs-ep-35-endpoint-ink) / [SRS-EP-37](./srs-quality.md#srs-ep-37-endpoint-quality). [SRS-EP-34](./srs-logic.md#srs-ep-34-end-styles) / [SRS-EP-36](./srs-ui.md#srs-ep-36-endpoint-toolbar) stay for Infini Path A — **do not implement on device this campaign**.

Human 2026-09-04: Path B on Epaper; Path A web/desktop later; store on `ConnectorAnchor`; second stroke **appends**.

### Business rules / eligibility / policy

| Rule id | Statement (product language) | Notes |
|---|---|---|
| BR-E01 | **Armed with Connector recognition.** Endpoint-ink bind runs only when `recog.connector` was latched at pen-down. Off → ordinary ink, even on an end. | Same latch as BR-C01 |
| BR-E02 | **Steal:** ≥80% of the new stroke’s **length** lies in a **5 mm** (world) circle at **one** end of **one** existing connector. Mixed / two ends / spine / empty → do not steal. | Architect names the constant and the overlap tie-break |
| BR-E03 | **Belongs to the end.** Bound strokes are decoration of that connector end, not free Ink and not a second connector. They paint **as drawn**. | No closed-style glyphs on Epaper |
| BR-E04 | **Lives on `ConnectorAnchor`.** Decoration is stored in the live face frame. `drawnN` / `drawnE` / `drawnBoxX` / `drawnBoxY` stay unchanged through transform. When re-warp changes the spine’s leaving angle, world paint rotates the decoration by that delta so it stays consistent with the spine. Rest spine is never rebaked. Architect owns the field list (must stay a **list of strokes**, not one merged blob). | Human: storage = ConnectorAnchor |
| BR-E05 | **Append.** A later stolen stroke on the same end adds to the list. The earlier decoration stays. Keep this flexible: do not collapse strokes into a single polyline this campaign. | Human: append |
| BR-E06 | **One undo peels the last append.** Wrong bind / last stroke: one undo restores pre-stroke for that bind. Earlier decoration on that end is unchanged. | Same “one undo per gesture” as other recog |
| BR-E07 | **Not a separate node.** Decoration is not independently selectable. Selecting the connector selects the ends. No banner on accept or refuse. | Seamless stylus |
| BR-E08 | **Document state.** Decoration is part of the connector (survives the session the same way the connector does). Publish with other connector mutations — not a clipboard-style local-only exception. | Architect binds the op / wire |
| BR-E09 | **Erase ticks, keep connector.** Brush clips decoration like Ink. Object-erase removes a decoration stroke when ≥80% of its length is inside the lasso. Area clips decoration interior. The connector spine is unchanged. Object-erase / delete of the **connector** still removes decoration with it. | Human 2026-09-04 |

### Edge cases

| Case | Expected product behavior |
|---|---|
| Connector recognition off | Stroke stays free Ink |
| ≥80% length in one 5 mm end circle | Bind; 0 free Ink; 0 second connector |
| Stroke on spine or empty | Not stolen |
| Two ends / two connectors both qualify | Do not steal (Architect may tighten to nearest-unique; refuse is the product-safe default) |
| Second stroke on same end | Append; one undo removes only that stroke |
| Bound node move / rotate | Decoration stays on that end; stored leave and `{n, e}` unchanged; world paint follows the re-warped leaving tangent; 0 orphaned samples |
| Object-erase the connector | Connector and its endpoint decoration gone; one undo restores both |
| Brush / object / area on ticks only | Decoration clipped or gone; **connector remains** |
| Path A chip / toolbar on device | **0** — Non-Goal this campaign |

### Acceptance (drives BDD / stories)

- Given Connector recognition armed and an existing connector, When a stroke has ≥80% length in one end’s 5 mm circle, Then it is bound on that end’s `ConnectorAnchor` (0 free Ink; 0 second connector) and paints as drawn.
- Given decoration on an end, When the bound node rotates or the peer moves, Then stored `{n, e}` and drawn leave stay unchanged and world paint follows the re-warped leaving tangent (0 orphaned samples; warp bar of [REQ-09](../../prd.md#device-connectors)).
- Given decoration on an end, When the creator draws another stolen stroke on that end, Then it appends; one undo removes only the last stroke.
- Given Connector recognition off, or a stroke on the spine / empty, When the stroke ends, Then 0 endpoint bind.

### Implemented via

| Concern | Pointer |
|---|---|
| Logic | [SRS-EP-35](./srs-logic.md#srs-ep-35-endpoint-ink) — steal, `ConnectorAnchor.styleInk[]`, tick erase |
| UI | None on Epaper. [SRS-EP-36](./srs-ui.md#srs-ep-36-endpoint-toolbar) not this campaign |
| Quality | [SRS-EP-37](./srs-quality.md#srs-ep-37-endpoint-quality) |
| Decision | [ADR-0038](../../../../adr/ADR-0038-endpoint-ink-face-frame.md) (supersedes [ADR-0026](../../../../adr/ADR-0026-endpoint-ink-membership.md)) |

---

## Superseded

_None yet._ Path A on Epaper is **Won't this campaign**, not a retired requirement id.
