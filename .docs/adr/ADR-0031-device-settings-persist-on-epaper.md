---
id: ADR-0031
title: Device Settings persist on Epaper
status: accepted
date: 2026-08-20
deciders: [architect, pm]
supersedes: [ADR-0030]
amends: [ADR-0015, ADR-0025]
source: TRACK-005 / Epaper [REQ-20] / [REQ-18] / Infini [REQ-05] retired
---

# ADR-0031 — Device Settings persist on Epaper

## Context

Human 2026-08-20: Device Settings (example: pen-button map) are **saved on the Epaper device**, not to Infini, and not into the document. Infini [REQ-05](../modules/infini/prd.md#pen-button-map) persist/restore is **retired** (`superseded-by` [Epaper REQ-20](../modules/epaper/prd.md#device-settings)). [CHL-0025](../../.plan/iter-005/challenges/CHL-0025-pen-map-settings-page.md) adopted: one Settings page, master-detail, catalogues inline.

[ADR-0030](./ADR-0030-tablet-authors-pen-button-map.md) correctly put **authoring on the tablet** and kept the map **out of the SVG**. What is **wrong** is the persist split: Infini must not be persistence home, must not restore-on-hello, and must not hold an app-settings copy. A second device opening the same Infini file must **not** inherit the first device’s barrel map.

[ADR-0028](./ADR-0028-pen-button-map-settings-channel.md) stays superseded. The settings family on `:9877` is **optional now**.

Quality goals at stake:

| Goal | Bar |
|---|---|
| 0 document messages | A settings write never emits `doc_load` / `doc_change` / `doc_snapshot` |
| Live while Infini down | Next barrel gesture uses the on-device map; persist does **not** wait on Infini; 0 lost local binds |
| Survive Epaper restart | Same device, next gesture uses persisted map, p95 ≤300 ms after first HID report |
| Per-device, not per-file | A different Epaper with factory defaults does not inherit the map from the Infini document |
| 0 Infini copies | 0 app-settings maps; 0 SVG map fields; 0 restore-on-hello `pen_button_map` |
| 0 fake slots | 0-button → neither side applies invented bindings |

## Decision

**The tablet authors the live map and persists it on this Epaper device.** Infini is **not** persist home. The map is **not** a document field. Authoring-on-tablet from ADR-0030 stands; the Infini persist/restore split does **not**.

Anatomy: [domain/pen-button-map](../domain/pen-button-map.md). Settings shell: [SRS-EP-52](../modules/epaper/features/tool-modes/srs-ui.md#srs-ep-52-pen-map-editor). Author + device persist: [SRS-EP-53](../modules/epaper/features/tool-modes/srs-logic.md#srs-ep-53-pen-map-author). Infini persist/restore ([SRS-IN-23](../modules/infini/features/tablet-sync/srs-logic.md#srs-in-23-pen-map-publish), [SRS-IN-25](../modules/infini/features/tablet-sync/srs-quality.md#srs-in-25-map-publish-quality)) is **retired**. Desktop editor UI ([SRS-IN-24](../modules/infini/features/tablet-sync/srs-ui.md#srs-in-24-pen-map-ui)) stays **retired**.

| Direction | `type` | When |
|---|---|---|
| Tablet → Desktop | `pen_capability` | **Optional HID telemetry** on session hello / when HID reports 0/1/2 barrel buttons. **Not** persist. **Not** restore. |
| Tablet → Desktop | `pen_button_map` | **0.** Do not persist-up. |
| Desktop → Tablet | `pen_button_map` | **0.** Do not restore-on-hello. |

If a peer still emits `pen_button_map` (old build), the other side **drops** it: do not store, do not apply, do not count as a document message.

| Rule | Value |
|---|---|
| Document messages | **0**. Auditors count `doc_load` / `doc_change` / `doc_snapshot` only |
| Persist | **Epaper device-local durable store**, **not** Infini app settings, **not** the SVG / VectorDocument |
| Author | Epaper on-device Settings · Pen buttons. Infini presents **0** map-editor screens |
| Apply | Tablet replaces its live map; **in-flight** barrel gesture keeps the map latched at button-down |
| Next gesture | Uses the new map; p95 ≤300 ms after apply on device |
| Restart | Load device store on Epaper start; next barrel gesture uses it p95 ≤300 ms after first HID report |
| Session | Live map applies with Infini down. Persist does **not** wait on the desktop |
| Unknown catalogue id | Device ignores that slot (treat as `off`); does not crash; does not invent a binding |
| 0-button capability | Store **0** slots; ignore maps that invent indexes |

Do **not** hang persist on [SRS-IN-05](../modules/infini/features/vector-document/srs-ui.md) Document chrome. Do **not** mint document-settings fields. Dispatch catalogues remain [SRS-EP-41](../modules/epaper/features/tool-modes/srs-logic.md#srs-ep-41-barrel-dispatch). Chip during hold-move remains [SRS-EP-42](../modules/epaper/features/tool-modes/srs-ui.md#srs-ep-42-chip-temp-tool) — **not** the Settings page.

## Consequences

- [ADR-0030](./ADR-0030-tablet-authors-pen-button-map.md) is **superseded**. Authoring-on-tablet and “not document / not SVG” stand. Infini persist/restore does **not**.
- [ADR-0028](./ADR-0028-pen-button-map-settings-channel.md) stays superseded. Amendment to [ADR-0015](./ADR-0015-one-way-sync-contract.md): `pen_capability` remains an allowed non-document type (HID telemetry). **`pen_button_map` is withdrawn** — 0 of those messages. They are not document, not viewport, not debug.
- [ADR-0025](./ADR-0025-barrel-vs-eraser-nib.md) routing stands (barrel ≠ nib). Rebind still applies to the **next** gesture. Restore source is the **device store**, not Infini.
- Sensitivity: **per-device preferences** vs **per-file convenience**. Two tablets on one Infini document keep independent maps.
- Trade-off point: **one checkable persist home (the tablet)** vs **desktop restore after wipe**. Accepted: a factory-reset Epaper returns to catalogue defaults; Infini cannot resurrect the old map.

## Alternatives Considered

| Approach | 0 doc msgs | Live while Infini down | Same-device restart | Per-device isolation | Ops cost | Why |
|---|---|---|---|---|---|---|
| Status quo ADR-0030 (Infini persist/restore) | + | + | + (via desktop) | − (file/session inheritance) | 0 | Rejected — Infini REQ-05 retired; not the product |
| Map fields on `doc_load` / SVG | − | 0 | − (file pollution) | − | + | Rejected — Device Settings are not document settings |
| Dual persist (device + Infini) | + | + | + | − | − | Rejected — two homes; restore races; second device inherits |
| Session-only (no persist) | + | + | − | + | + | Rejected — fails REQ-20 restart AC |
| Drop `pen_capability` too | + | + | + | + | + | Viable; deferred — optional HID telemetry is cheap and does not restore a map |
| **Device persist, 0 Infini copies (this ADR)** | + | + | + | + | 0 | Winner |

Trade-off point: **keep the one-way document contract checkable by message type** vs **conveniently stuffing settings into the file**. Unchanged. New trade-off: **this tablet’s preferences** vs **desktop as the restore home**. Device wins.
