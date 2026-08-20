---
id: STORY-EP-057
title: Persist Device Settings on the Epaper device
kind: implement
parent_srs: [SRS-EP-53]
parent_req: [REQ-20]
status: draft
priority: P0
iter: iter-005
estimate: 3
owner: dev
depends_on: [STORY-EP-056]
acceptance_criteria:
  - "Given a rebound barrel map on this device, When Epaper restarts on the same device, Then the next barrel gesture uses that map with p95 <=300 ms after the first HID report."
  - "Given Infini is connected, When the creator rebinds Device Settings on the tablet, Then Infini holds 0 copy of the map (0 app-settings, 0 SVG) and sends 0 document messages for that write."
  - "Given Infini has no session, When the creator rebinds on the tablet, Then the live device map still applies (persist does not wait on Infini)."
  - "Given a different Epaper device with factory defaults, When it opens the same Infini document, Then it does not inherit the first device's barrel map."
design_package: ".plan/iter-005/design/pen-button-map/"
ui_spec: ".plan/iter-005/design/pen-button-map/ui-spec.md"
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-057 — Persist Device Settings on the Epaper device

TRACK-005. Parent [REQ-20](../../../.docs/modules/epaper/prd.md#device-settings) (Device Settings). Persist **on this tablet**. Infini [REQ-05](../../../.docs/modules/infini/prd.md#pen-button-map) persist/restore is **retired**. Replaces the intent of [STORY-IN-035](./STORY-IN-035.md) (parked — do not implement Infini persist).

`parent_srs` is the nearest existing section until Solution Architect rebinds persist (SRS-EP-53 and/or a new id). Status stays **draft** until that bind and behavior-driven scenarios exist.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | STORY-EP-056 |
