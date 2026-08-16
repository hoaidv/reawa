---
iter: iter-004
author: pm
date: 2026-08-16
frameworks: [MoSCoW]
---

# Prioritisation — iter-005 **draft** (not committed)

In the room: pm + human. Source [BS-0002](./brainstorms/BS-0002-iter-005-feature-wave.md).
**Do not** treat as iter-004 capacity. SM: no stories until retro-gate.

## MoSCoW

| REQ | Title | Priority |
|---|---|---|
| epaper [REQ-11](../../.docs/modules/epaper/prd.md#erase) | Erase like paper | Must |
| epaper [REQ-12](../../.docs/modules/epaper/prd.md#clipboard) | Copy/cut/paste | Must |
| epaper [REQ-13](../../.docs/modules/epaper/prd.md#connector-ends) | Connector endpoint styles | Must |
| epaper [REQ-14](../../.docs/modules/epaper/prd.md#connector-attachments) | Mid-attachments | Must |
| epaper [REQ-18](../../.docs/modules/epaper/prd.md#pen-buttons) | Barrel-button accelerators | Must |
| infini [REQ-05](../../.docs/modules/infini/prd.md#pen-button-map) | Pen-button map settings | Must (peer of REQ-18) |
| epaper [REQ-16](../../.docs/modules/epaper/prd.md#device-pan-zoom) | Finger pan/zoom | Should — BRD-07 block |
| epaper [REQ-17](../../.docs/modules/epaper/prd.md#manual-create) | Manual frame/connector/primitive | Should |
| epaper [REQ-15](../../.docs/modules/epaper/prd.md#table-recognition) | Table recognition | Could |
| AI | — | Won't (unspecified) |
| CHL-0011 / CHL-0012 | Nested / wrap-content | Won't unless pulled in |

## Sequence (advisory)

1. REQ-11 erase + REQ-18/IN-05 buttons (erase hold needs an erase verb)
2. REQ-12 clipboard
3. REQ-13 ends → REQ-14 attachments (both extend REQ-09)
4. REQ-17 manual create (after or with REQ-08)
5. REQ-16 pan/zoom after BRD amendment + ADR
6. REQ-15 tables last (FP gate)
