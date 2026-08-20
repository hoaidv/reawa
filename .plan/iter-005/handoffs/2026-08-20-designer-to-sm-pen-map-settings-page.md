---
from: designer
to: sm
date: 2026-08-20
iter: iter-005
cc: [pm, architect, qa]
---

# Hand-off: Designer → Scrum Master — EP-056 Settings page (CHL-0025)

Human 2026-08-20 override painted in [UI-EP-08](../design/pen-button-map/ui-spec.md). Product docs **not** edited.

## What changed

One **Settings** page, **master-detail**. First (and only this package) master item: **Pen buttons**. Click and Hold-move catalogues are **inline** on the detail pane. 0 / 1 / 2 / offline remain states of that same page.

Deleted sheet scenes: `pen-button-map-slot-click.html`, `pen-button-map-slot-hold.html`.

Navigator: [`.plan/iter-005/design/pen-button-map/index.html`](../design/pen-button-map/index.html). Primary: `pen-button-map-layout-1.html`.

## Challenge (PM / architect)

[CHL-0025](../challenges/CHL-0025-pen-map-settings-page.md) — adopt: drop `present-sheet` / `scene.pen_map_click` / `scene.pen_map_hold`; Settings shell + master-detail. Designer did not edit `srs-ui.md` or the scene graph.

## Next

`/sm` can spawn Product Manager to triage CHL-0025. Quality Assurance Engineer BDD against the one-page Settings surface, not sheets.
