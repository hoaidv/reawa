---
id: STORY-EP-022
title: Design selection rubber-band and Enclose CTA
kind: design
parent_srs: [SRS-EP-12, SRS-EP-10, SRS-EP-11]
parent_req: [REQ-05, REQ-06]
status: ready
priority: P1
iter: iter-003
estimate: 3
owner: designer
depends_on: [STORY-EP-012]
acceptance_criteria:
  - "Given SRS states sel.marquee / sel.nodes_selected / sel.create_refused, When the package ships, Then one scene HTML exists per state with thin dotted rubber-band, 6 square anchors, and cta.enclose on SelectionOverlay (not ToolChip)."
  - "Given UI-EP-01 three-tool chip, When Enclose is shown, Then ToolChip inventory stays Selection · Pen · Ink-box (ADR-0016)."
  - "Given platform epaper-device, When ui-spec-gate runs, Then it passes; ui_spec + scenes + hifi are set on this story and copied to EP-018."
  - "Given refuse path, When no surround qualifies, Then ind.create_refused_no_surround is visible and selection chrome is unchanged."
design_package: ".plan/iter-003/design/selection-enclose-chrome/"
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-022 — Design selection rubber-band and Enclose CTA

Designs Creation B chrome for [SRS-EP-12](../../../.docs/modules/epaper/features/ink-box/srs-ui.md)
after [CHL-0013](../challenges/CHL-0013-selection-create-feedback-enclose-cta.md) Adopt and
[ADR-0016](../../../.docs/adr/ADR-0016-selection-create-enclose-cta.md).

**Compose** [UI-EP-02](../design/device-selection-chrome/) (EP-012 done) — reuse refuse patterns /
tokens; do not fork ToolChip. New package owns marquee + nodes_selected + Enclose CTA scenes.

## Kind

| Field | Value |
|---|---|
| Kind | `design` |
| Owner | `designer` |
| Depends on | EP-012 (compose) |

## Required scenes (SRS state ids)

| State | Must show |
|---|---|
| `sel.marquee` | Thin dotted rubber-band following pen tip |
| `sel.nodes_selected` | Dotted union rect + **6** square anchors + `cta.enclose` |
| `sel.create_refused` | Refuse indicator; selection unchanged |
| `sel.none` (optional hop) | Baseline |

## Done when

- `ui-spec-gate` pass; `ui_spec` / `scenes` / `hifi` set
- [STORY-EP-018](./STORY-EP-018.md) updated with those paths and `depends_on` includes this story
- SM may then flip EP-018 → `ready` for `/qa` → `/dev`
