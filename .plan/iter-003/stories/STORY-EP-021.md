---
id: STORY-EP-021
title: Ship device console logs on :9878
kind: implement
parent_srs: [SRS-EP-15, SRS-EP-16]
parent_req: [REQ-07]
status: in-review
priority: P0
iter: iter-003
estimate: 3
owner: dev
depends_on: []
acceptance_criteria:
  - "Given EPAPER_DEBUG_LOG is on and RM_SYNC_HOST is set, When Infini sends debug_start, Then Qt qInfo/qWarning/qCritical records ship as debug_log on TCP 9878."
  - "Given the GUI/render/ink thread, When a log is emitted, Then 0 write/flush/socket wait runs on that thread (worker or queued enqueue only)."
  - "Given ingestPoint or paint, When they run, Then 0 log I/O occurs on those paths."
  - "Given the ship queue overflows 512, When a new record arrives, Then the oldest is dropped and the next debug_log.dropped count increases."
  - "Given stdout/stderr can be captured, When capture succeeds, Then those lines ship with logger stdio; if capture fails, Qt handler still ships."
  - "Given enclose ingest returns, When shipping is on, Then one qInfo [enclose] line is emitted after ingest (created id=… or ordinary reason=…) with 0 change to recognize_enclose.hpp guards."
  - "Given EPAPER_DEBUG_LOG is off, When the app runs, Then 0 connects to 9878."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-021 — Ship device console logs on :9878

Implements [SRS-EP-15](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-15-debug-log-ship)
and [SRS-EP-16](../../../.docs/modules/epaper/features/device-document/srs-quality.md).
Peer: [STORY-IN-029](./STORY-IN-029.md).

Env: `EPAPER_DEBUG_LOG=1` (default off). Host: `RM_SYNC_HOST` (same USB Mac IP as stroke sync).
Port: `EPAPER_DEBUG_PORT` else 9878. **Not** `:9877`.

`[enclose]` log is a **source** after `ingestStrokeAtPenUp` returns — do not edit
`recognize_enclose.hpp` guards or `create_smart_group` payload.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — |

## Done when

- `@SRS-EP-15` / `@SRS-EP-16` scenarios green
- Deploy with `EPAPER_DEBUG_LOG=1` so the human can inspect EP-016 enclose on Infini
