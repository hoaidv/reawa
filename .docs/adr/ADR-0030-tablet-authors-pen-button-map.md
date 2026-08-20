---
id: ADR-0030
title: Tablet authors pen-button map; Infini persist/restore
status: accepted
date: 2026-08-20
deciders: [architect, pm]
supersedes: [ADR-0028]
amends: [ADR-0015, ADR-0025]
source: TRACK-005 / Epaper [REQ-18] / Infini [REQ-05]
---

# ADR-0030 — Tablet authors pen-button map; Infini persist/restore

## Context

Human 2026-08-20: the creator binds Click / Hold-move **on the tablet** ([Epaper REQ-18](../modules/epaper/prd.md#pen-buttons)). Infini [REQ-05](../modules/infini/prd.md#pen-button-map) is **persist/restore only** — the desktop map-editor UI outcome is retired in place. Live map must still apply when the desktop is down.

[ADR-0028](./ADR-0028-pen-button-map-settings-channel.md) correctly introduced a third **non-document** family on TCP `:9877` (`pen_capability` / `pen_button_map`) so the map never rides `doc_*` or the SVG. That family stays. What is **wrong** is the authoring direction: Infini does not author, does not present a settings screen, and does not publish the map down on desktop save.

Quality goals at stake:

| Goal | Bar |
|---|---|
| 0 document messages | Persist/restore never emit `doc_load` / `doc_change` / `doc_snapshot` |
| Live while Infini down | Next barrel gesture uses the on-device map; 0 lost local binds |
| Restore after later session | Matching 1- or 2-button capability → next tablet gesture uses persisted map, p95 ≤300 ms; in-flight unchanged |
| 0 fake slots | 0-button → neither side applies invented bindings |

## Decision

**The tablet authors the live map. Infini is persistence home.** The settings channel on `:9877` remains (same socket, distinct `type`) — **not** a second port, **not** a document field.

Anatomy: [domain/pen-button-map](../domain/pen-button-map.md). Editor: [SRS-EP-52](../modules/epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor) / [SRS-EP-53](../modules/epaper/features/tool-modes/srs-logic.md#srs-ep-53-pen-map-author). Persist/restore: [SRS-IN-23](../modules/infini/features/tablet-sync/srs-logic.md#srs-in-23-pen-map-publish). Desktop editor UI ([SRS-IN-24](../modules/infini/features/tablet-sync/srs-ui.md#srs-in-24-pen-map-ui)) is **retired**.

| Direction | `type` | When |
|---|---|---|
| Tablet → Desktop | `pen_capability` | On session hello / when HID reports 0/1/2 barrel buttons |
| Tablet → Desktop | `pen_button_map` | On live rebind while linked; on reconnect if the tablet authored this session (`map.pending_persist` or `map.applied` from the editor) |
| Desktop → Tablet | `pen_button_map` | **Restore only:** after hello if Infini has a persisted map matching `buttonCount` **and** the tablet has not authored a live map this session (still factory default / `map.absent`) |

Payload shape is unchanged from ADR-0028 (normative anatomy in the domain doc):

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
| Document messages | **0**. Auditors count `doc_load` / `doc_change` / `doc_snapshot` only |
| Persist | Infini **app settings** (user profile), **not** the SVG / VectorDocument |
| Author | Epaper on-device editor. Infini presents **0** map-editor screens |
| Apply | Tablet replaces its live map; **in-flight** barrel gesture keeps the map latched at button-down |
| Next gesture | Uses the new map; p95 ≤300 ms after apply on device |
| Hello race | Pending/authored live map **wins** (persist up). Restore down **only** when this tablet session has not authored |
| Unknown catalogue id | Device ignores that slot (treat as `off`); does not crash; does not invent a binding |
| 0-button capability | Neither side stores or applies slots for absent indexes |

Do **not** hang persist on [SRS-IN-05](../modules/infini/features/vector-document/srs-ui.md) Document chrome. Dispatch catalogues remain [SRS-EP-41](../modules/epaper/features/tool-modes/srs-logic.md#srs-ep-41-barrel-dispatch). Chip during hold-move remains [SRS-EP-42](../modules/epaper/features/tool-modes/srs-ui.md#srs-ep-42-chip-temp-tool) — **not** the editor.

## Consequences

- [ADR-0028](./ADR-0028-pen-button-map-settings-channel.md) is **superseded**. Keep the settings-family amendment to [ADR-0015](./ADR-0015-one-way-sync-contract.md): `pen_capability` / `pen_button_map` are allowed. They are not document, not viewport, not debug. **Payload direction of `pen_button_map` is reversed** except restore-on-hello.
- [ADR-0025](./ADR-0025-barrel-vs-eraser-nib.md) routing stands (barrel ≠ nib). Catalogue **UI home** is [SRS-EP-52](../modules/epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor), not Infini. Rebind still applies to the **next** gesture.
- Sensitivity: **live map while desktop is down** vs **restore after a later session**. Hello order (capability, then persist-up **or** restore-down) is the crux.
- Trade-off point: **one checkable settings type** vs **stuffing the map into the file or a second port**. Type-level split stays; only who writes the live map changed.

## Alternatives Considered

| Approach | 0 doc msgs | Live while Infini down | Cross-session persist | Ops cost | Why |
|---|---|---|---|---|---|
| Status quo ADR-0028 (Infini authors, D→T on save) | + | − | + | 0 | Rejected — not the product; desktop editor retired |
| Map fields on `doc_load` | − | 0 | − (file pollution) | + | Rejected — fails Infini REQ-05 |
| Device file persist, no Infini | + | + | − (lost on wipe / no desktop home) | 0 | Rejected — Infini is persistence home |
| Second TCP port | + | + | + | − | Rejected — extra session; `:9878` is logs-only |
| **Tablet authors + Infini persist/restore on `:9877` (this ADR)** | + | + | + | 0 | Winner |

Trade-off point: **keep the one-way document contract checkable by message type** vs **conveniently stuffing settings into the file**. Unchanged from ADR-0028. New trade-off: **in-session author (tablet)** vs **across-restart store (Infini)**.
