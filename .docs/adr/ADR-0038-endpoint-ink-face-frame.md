---
id: ADR-0038
title: Endpoint-ink on ConnectorAnchor (face frame, length steal, tick erase)
status: accepted
date: 2026-09-04
deciders: [architect, pm]
supersedes: [ADR-0026]
amends: [ADR-0022, ADR-0034]
source: TRACK-005 / [REQ-13] Path B / [SRS-EP-74]
---

# ADR-0038 — Endpoint-ink on ConnectorAnchor (face frame, length steal, tick erase)

## Context

[REQ-13](../modules/epaper/prd.md#connector-ends) on Epaper is **Path B only** ([SRS-EP-74](../modules/epaper/features/connector-ink/srs-product.md#srs-ep-74-endpoint-ink-product)): drawn ink on an existing connector end, append, no toolbar. Human 2026-09-04: store on `ConnectorAnchor`; second stroke appends; creator can **erase the ticks and keep the connector**.

[ADR-0026](./ADR-0026-endpoint-ink-membership.md) stored decoration as `(s, d)` on rest spine `S` and stole by sample-count in an `s`-window + `R_END`. That frame follows warp, not the node face — arrows would not stay aimed at the box under rotate. It was never implemented (`status: proposed`).

## Decision

1. **Home.** Each end’s decoration is `ConnectorAnchor.styleInk`: an ordered **list of strokes**, each a polyline of `{n, e}` in the **live face frame** (origin = attach point; `+n` = outward face normal; `+e` = along the face CCW). Centre ends: origin = attach; `+n` = facing; `+e` = left-normal of facing. Not `(s, d)` on `S`. Rest spine is never rebaked ([ADR-0020](./ADR-0020-connector-ink-geometry.md) I1).

2. **Steal.** Pen-up order (amends [ADR-0022](./ADR-0022-recognizer-dispatch.md)): **endpoint-ink** → draw-into membership → enclose → new connector → ink. Armed only when `recog.connector` was latched at pen-down. Short ticks the closure classifier treats as closed-ish still run this test **first** (before membership / enclose); new-connector still requires open. Bind when ≥80% of the new stroke’s **arc length** lies in a **5 mm world** circle (`eraseMmToWorld(5)`) at **one** end of **one** existing connector. Mixed / two ends / two connectors / spine / empty → do not steal. Log `[recog] outcome=endpoint_ink end=from|to id=<connectorId>`.

3. **Commit.** Ingest still `append_ink`. Bind is a second gesture: `set_endpoint_ink` (append stroke on that end) + `remove_node` of the free Ink. One undo peels that append and restores the Ink.

4. **Paint.** World = `origin + n·N' + e·E'` with live face axes, then rotate by **α** = signed angle from the stored leave (`drawnN`/`drawnE` / `drawnBoxX`/`drawnBoxY`, reconstructed as `WarpEnd.f`) to the re-warped sample tangent at that end. Stored `{n, e}` and drawn leave are not rewritten. Same stroke width as the connector.

5. **Erase ticks, keep connector** (amends [ADR-0034](./ADR-0034-erase-clip-remnants.md) kinds). Brush / area clip `styleInk` polylines in world (same remnant floor). Object-erase drops a decoration stroke when ≥80% of its length is inside the lasso. Connector **spine** still follows [SRS-EP-56](../modules/epaper/features/erase/srs-logic.md) (brush no-op on `V`) / [SRS-EP-58](../modules/epaper/features/erase/srs-logic.md) (object 80% of `V` removes the connector and its decoration).

## Consequences

- Path A closed styles stay specified on [SRS-EP-34](../modules/epaper/features/connector-ink/srs-logic.md) / [SRS-EP-36](../modules/epaper/features/connector-ink/srs-ui.md) for Infini later — **not implemented on Epaper this campaign**.
- False-positive ship gate still includes unintended endpoint-ink binds ([SRS-EP-20](../modules/epaper/features/connector-ink/srs-quality.md)).
- Sensitivity: 5 mm radius and 80% length. Retune via SRS constants, not a new ADR.

## Alternatives Considered

| Approach | Face-aim | Spine safety | Erase ticks | Why |
|---|---|---|---|---|
| ADR-0026 `(s, d)` on `S` | − | + | 0 | Rejected — rotate shears the arrow off the face |
| Exclusive endpoint tool | + | + | + | Rejected — PRD is recognition |
| **Face-frame list on ConnectorAnchor (this ADR)** | + | + (circle + refuse mixed) | + | Winner |
