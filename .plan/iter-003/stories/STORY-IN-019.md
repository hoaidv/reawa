---
id: STORY-IN-019
title: "RM connection status eager sync on load"
kind: implement
parent_srs: [SRS-IN-07]
parent_req: [REQ-04]
status: done
priority: P0
iter: iter-003
estimate: 2
owner: dev
depends_on: []
acceptance_criteria:
  - "Given Epaper already connected to stroke-ingest before Infini renderer mounts, When the canvas loads, Then sync hint shows RM connected within one paint and doc_snapshot + viewport publish immediately."
  - "Given Infini opens first and Epaper connects seconds later, When the TCP client joins, Then the connected event still updates UI and triggers snapshot (no regression)."
  - "Given the last RM client disconnects, When closed fires, Then hint shows disconnected/waiting (no stale connected state)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-IN-019 — RM connection status eager sync on load

**Human verify fail (2026-08-11):** desktop showed connection too late — likely race between
Epaper TCP connect and React `onRmClient` subscription / async `rmClientCount` in
`CanvasStage.tsx` + `electron/main.cjs`.

Fix: subscribe before async gap; push current client count when renderer attaches; idempotent
snapshot on first connected observation.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | — |

## Done when

- AC green `@SRS-IN-07`; human sees connected state on cold-start with Epaper already linked
