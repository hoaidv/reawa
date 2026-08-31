---
id: STORY-EP-069
title: ToolContextImpl host ports and SelectionOverlay
kind: implement
parent_srs: [SRS-EP-04, SRS-EP-12]
parent_req: [REQ-03, REQ-06]
status: in-progress
priority: P0
iter: iter-005
estimate: 5
owner: dev
depends_on: [STORY-EP-068]
acceptance_criteria:
  - "Given ToolContextImpl, When any exclusive tool is armed, Then the adapter has 0 exclusive-id string compares and 0 selection-knob or live-manip methods."
  - "Given ToolCanvasItem, When exclusiveTool changes, Then chip-string to ModeId is mapped only in syncActiveMode."
  - "Given SelectionOverlay, When settled or live selection paints, Then Mode calls paintSettled or paintLiveManip; ToolContextImpl does not include SelectionOverlay."
  - "Given erase or lasso damage, When ToolContextImpl::damageChrome runs, Then dirty-union lives on the impl, not on SelectionOverlay."
  - "Given principles.md, When a later change would put Mode policy on ToolContextImpl or merge SelectionOverlay into ToolCanvasItem, Then the change files a challenge instead."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-069 — ToolContextImpl host ports and SelectionOverlay

Make the ToolContext adapter generic. Rename `ToolCanvasContext` → `ToolContextImpl`. Rename
`ToolChrome` → `SelectionOverlay` (host-owned, not merged into the Qt item). Lock the split in
[principles.md](../../../.docs/modules/epaper/tool-system/principles.md) and
[ADR-0035](../../../.docs/adr/ADR-0035-tool-context-is-host-ports.md).

Human is Quality Assurance Engineer; no design package; no behavior-driven ceremony.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Depends on | [STORY-EP-068](./STORY-EP-068.md) |

## Done when

- Grep: no `ToolCanvasContext`, `ToolChrome`, `isSelectionTool`, `isEraserTool` in `tools/`
- Panel: settled knobs + bar; live node on finger-move from ink; brush hover/ghost; lasso Pen-before-contact
