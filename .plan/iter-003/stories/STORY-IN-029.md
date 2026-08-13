---
id: STORY-IN-029
title: Device Log panel and :9878 listener
kind: implement
parent_srs: [SRS-IN-17, SRS-IN-18, SRS-IN-19]
parent_req: [REQ-03]
status: done
priority: P0
iter: iter-003
estimate: 3
owner: dev
depends_on: []
acceptance_criteria:
  - "Given Infini is running, When the window loads, Then a Device Log button is visible in window chrome (not WorldLayer) and TCP 9878 is listening."
  - "Given the overlay is closed, When the user clicks Device Log, Then a full-size in-app panel covers the canvas (not a second window), Infini sends debug_request then debug_start, and the state is streaming or disconnected."
  - "Given inbound debug_log lines, When they arrive, Then they append to an in-memory ring (cap 10000) and appear in the stream; they are never applied to VectorDocument."
  - "Given a filter string, When the user types it, Then only matching buffered lines show; the buffer is unchanged."
  - "Given the overlay is open, When the user clicks Close or Escape, Then the overlay closes, debug_stop is sent, and the buffer is kept."
  - "Given a viewport, doc_change, or stroke_* line on 9878, When it is parsed, Then it is dropped and never forwarded to the 9877 decoder."
design_package: ""
ui_spec: ".docs/modules/infini/features/tablet-sync/srs-ui.md"
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-029 — Device Log panel and :9878 listener

Implements [SRS-IN-17](../../../.docs/modules/infini/features/tablet-sync/srs-logic.md#srs-in-17-debug-log-channel)
(listen `:9878`), [SRS-IN-18](../../../.docs/modules/infini/features/tablet-sync/srs-ui.md#srs-in-18-device-log-panel)
(button + overlay), and [SRS-IN-19](../../../.docs/modules/infini/features/tablet-sync/srs-quality.md)
(0 log I/O on paint/apply).

**No design story** — architect `needs_design: false`; human specified the overlay. Compose from
SRS-IN-18 regions and `.docs/DESIGN.md` tokens. Do not invent a second `BrowserWindow`.

Peer: [STORY-EP-021](./STORY-EP-021.md). Do not mix traffic onto `:9877`. Do not amend ADR-0015.
Do not flip EP-017. EP-016 stays `in-review`.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — |

## Done when

- `@SRS-IN-17` / `@SRS-IN-18` / `@SRS-IN-19` scenarios green
- Device Log overlay inspects EP-012…016 logs without touching the document tree
