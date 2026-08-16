---
id: STORY-EP-031
title: Ink/Curve warp and live re-warp on bound-node drag
kind: implement
parent_srs: [SRS-EP-18, SRS-EP-20]
parent_req: [REQ-09]
status: done
priority: P0
iter: iter-004
estimate: 8
owner: dev
depends_on: [STORY-EP-030]
acceptance_criteria:
  - "Given a connector, When a bound box is moved or resized, Then the connector re-warps live ≥5 Hz, 0 full-panel invalidations, committed geometry = last previewed (ADR-0020 / SRS-EP-18)."
  - "Given warpStyle morph at rest, When nothing has turned, Then output is bitwise the rest-shape reconstruction (I3)."
  - "Given 200 poses returning to start, When never-re-bake holds, Then drift is 0.000 u on host fixtures (I1)."
  - "Given the bound box is deleted, When the connector is drawn, Then it remains; undo of delete glues the same nodeId back (D39)."
design_package: ".plan/iter-004/design/connector-chrome/"
ui_spec: ".plan/iter-004/design/connector-chrome/ui-spec.md"
scenes:
  - ".plan/iter-004/design/connector-chrome/connector-chrome-blink.html"
  - ".plan/iter-004/design/connector-chrome/connector-chrome-selected.html"
  - ".plan/iter-004/design/connector-chrome/connector-chrome-rejected.html"
  - ".plan/iter-004/design/connector-chrome/connector-chrome-live-warp.html"
  - ".plan/iter-004/design/connector-chrome/connector-chrome-orphan.html"
hifi: ".plan/iter-004/design/connector-chrome/connector-chrome-live-warp.html"
wireframe: ""
---

# STORY-EP-031 — Ink/Curve warp and live re-warp on bound-node drag

Reimplement [ADR-0020](../../.docs/adr/ADR-0020-connector-ink-geometry.md) in production
(do not copy the EXP probe). Shared fixtures with [STORY-IN-030](./STORY-IN-030.md).

**Human verified 2026-08-15** on device (live re-warp; dirty-rect clip to the dragging node’s
box fixed — connector AABB no longer `QRectF(0×0).united`).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | EP-030 |
