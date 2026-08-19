---
id: ADR-0028
title: Pen-button map publish is a settings channel
status: accepted
date: 2026-08-19
deciders: [architect]
supersedes: null
amends: [ADR-0015]
source: TRACK-005 / Infini [REQ-05] / Epaper [REQ-18]
---

# ADR-0028 — Pen-button map publish is a settings channel

## Context

[Infini REQ-05](../modules/infini/prd.md#pen-button-map) : the creator edits Click / Hold-move bindings on the **desktop**; Infini persists the map and **publishes** it to the tablet. Acceptance: when the map is published, Infini still sends **0 document messages**. [ADR-0015](./ADR-0015-one-way-sync-contract.md) already forbids anything but `doc_load` + `viewport` downward after the handshake. Putting the map in `doc_change` / `doc_load` would either (a) count as a document message or (b) bake UI settings into the SVG.

[REQ-18](../modules/epaper/prd.md#pen-buttons) : the tablet **consumes** the map; it does not host a 5-way radio on the chip. Rebind never changes a gesture in flight.

Status **accepted**: forced by shipped ADR-0015 (0 inbound document messages) plus REQ-05’s explicit “settings, not a document edit.”

## Decision

Introduce a third **non-document** family on the existing TCP `:9877` JSON-lines session (same socket, distinct `type`), sibling to viewport — **not** a second port (debug `:9878` stays logs-only).

| Direction | `type` | When |
|---|---|---|
| Tablet → Desktop | `pen_capability` | On session hello / when HID reports 0/1/2 barrel buttons |
| Desktop → Tablet | `pen_button_map` | On save in Infini settings, and once after hello if a persisted map exists |

Payload (normative anatomy: [domain/pen-button-map](../domain/pen-button-map.md)):

```text
{
  "type": "pen_button_map",
  "mapId": "<uuid>",
  "buttons": [
    { "index": 1, "click": "<clickCatalogueId>", "holdMove": "<holdMoveCatalogueId>" }
  ]
}
```

| Rule | Value |
|---|---|
| Document messages | **0**. Auditors count `doc_load` / `doc_change` / `doc_snapshot` only. `pen_button_map` is **settings** |
| Persist | Infini app settings (user profile), **not** the SVG / VectorDocument |
| Apply | Tablet replaces its live map; **in-flight** barrel gesture keeps the map latched at button-down |
| Next gesture | Uses the new map; p95 ≤300 ms after the settings message is applied on device |
| Offline edit | Infini may edit while disconnected; publish on reconnect **after** the ADR-0015 drain/load handshake, still as `pen_button_map`, never inside `doc_load` |
| Unknown catalogue id | Device ignores that slot (treat as `off`); does not crash; does not invent a binding |
| 0-button capability | Infini UI hides/disables slots; device ignores maps for absent indexes |

Do **not** hang this on [SRS-IN-05](../modules/infini/features/vector-document/srs-ui.md) Document chrome (open/save). Home: [SRS-IN-23](../modules/infini/features/tablet-sync/srs-logic.md) + [SRS-IN-24](../modules/infini/features/tablet-sync/srs-ui.md).

## Consequences

- ADR-0015 “anything else is a protocol defect” is amended: **`pen_capability` / `pen_button_map` are allowed**. They are not document, not viewport, not debug.
- Viewport last-writer ([ADR-0023](./ADR-0023-viewport-last-writer.md)) remains a separate family (`viewport`).
- Barrel vs nib ([ADR-0025](./ADR-0025-barrel-vs-eraser-nib.md)) is device routing; this ADR is **how the map arrives**.
- Sensitivity: **settings-on-session** vs **new TCP port**. One socket keeps USB/firewall boring; type-level audit still proves 0 document messages.

## Alternatives Considered

| Approach | 0 doc messages | Persist correctly | Ops cost | Why |
|---|---|---|---|---|
| Status quo (no map) | + | n/a | + | Rejected — REQ-05 / REQ-18 |
| Map fields on `doc_load` | − | − (file pollution) | + | Rejected — fails REQ-05 acceptance |
| `doc_change` of a hidden settings node | − | − | 0 | Rejected — document channel |
| Second TCP port | + | + | − | Rejected — extra session; `:9878` is already the sidecar for logs |
| **Settings messages on `:9877` (this ADR)** | + | + | 0 | Winner — forced by ADR-0015 + REQ-05 |

Trade-off point: **keep the one-way document contract checkable by message type** vs **conveniently stuffing settings into the file**. Type-level split wins.
