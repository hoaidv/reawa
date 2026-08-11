---
id: STORY-IN-013
title: "Design Infini ink-box tools and selection overlay"
kind: design
parent_srs: [SRS-IN-14]
parent_req: [REQ-04]
status: done
priority: P0
iter: iter-003
estimate: 5
owner: designer
depends_on: []
acceptance_criteria:
  - "Given SRS-IN-14, When the package ships, Then ToolStrip (Selection · Ink-box), SelectionOverlay (bounds, resize handles, inkScaleMode), and BoundsHint are specified with platform + responsive notes."
  - "Given multi-ink selection, When no surround stroke qualifies, Then scenes cover create-refused (`cta.create_smart_group` / `ind.create_refused_no_surround`)."
  - "Given a selected Smart Group, When below TILE_LOD_SCALE, Then manipulation-unavailable state is designed."
  - "Given design system rules, When ui-spec-gate runs, Then package-contained hi-fi scenes + Spec pass; no rotation handle in pilot."
design_package: ".plan/iter-003/design/ink-box-ui/"
ui_spec: ".plan/iter-003/design/ink-box-ui/ui-spec.md"
scenes:
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-selection-idle.html"
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-selection-selected.html"
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-selection-dragging.html"
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-ink-box-armed.html"
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-manipulation-unavailable.html"
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-create-refused.html"
  - ".plan/iter-003/design/ink-box-ui/ink-box-ui-states.html"
hifi: ".plan/iter-003/design/ink-box-ui/ink-box-ui-selection-idle.html"
wireframe: ""
---

# STORY-IN-013 — Design Infini ink-box tools and selection overlay

**Done.** Package [UI-IN-02](../design/ink-box-ui/ui-spec.md). Navigator: [index.html](../design/ink-box-ui/index.html).

Open: [CHL-0001](../challenges/CHL-0001-create-refused-state.md) — PM to add create_refused to SRS-IN-14 states matrix.

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | — |
