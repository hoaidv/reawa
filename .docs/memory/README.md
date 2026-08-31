# Project memory — Reawa

Engineering narratives and reference material preserved from legacy docs. Normative specs live in [BRD](../brd.md), [PRD](../modules/reawa/prd.md), and feature SRS.

| Document | Purpose |
|---|---|
| [plan_epaper-tool-system-refactor.md](./plan_epaper-tool-system-refactor.md) | Historical taxonomy notes. **Canonical catalog:** [tool-system](../modules/epaper/tool-system/index.md) |
| [plan_dissolve_host_bags.md](./plan_dissolve_host_bags.md) | Operations own logic; HostCaps is ports only — dissolve Finger/Manip/Stroke hosts and intent-appliers |
| [plan_toolaction_context_ui.md](./plan_toolaction_context_ui.md) | ToolAction, selection context UI, overlay HitTarget, Interventions, leftover sessions |
| [refactoring-skills/](./refactoring-skills/SKILL.md) | Skills + use case: do not stop at hollow Operations; body-level extract gates |
| [swift-porting.md](./swift-porting.md) | Python → Swift port history, module mapping, porting bugs |
| [macos-window-lifecycle-investigation.md](./macos-window-lifecycle-investigation.md) | Stage Manager lifecycle detection investigation |
| [native-stylus-packaging.md](./native-stylus-packaging.md) | Entitlements, signing, and packaging checklist |
| [learn.ipynb](./learn.ipynb) | evdev / Linux input event format reference notebook |
| [erase-brush-commit.md](./erase-brush-commit.md) | Brush erase: duplicate remnant ids, not clip miss; generateNodeId |
| [ink-path-density-hitch.md](./ink-path-density-hitch.md) | Dense-page ink hitch (verified 2026-08-31): FullClear → ToolCanvas Mono → camera rasterize on pen-up; `/tmp/epaper-ink-path.log` |
| [camera-pan-zoom-rasterize.md](./camera-pan-zoom-rasterize.md) | Camera pan/zoom: blit preview + LatestJob sharpen; probe `/tmp/epaper-raster.log` |
| [connector-live-manip-settle.md](./connector-live-manip-settle.md) | Move/resize: origin connector punch must include the spine AABB, not only the box (BR-B19) |
| [object-erase-and-connector-ux2.md](./object-erase-and-connector-ux2.md) | Object-erase overlay: append-only dashed raster, no per-event restroke; connector UX2 is last-3 free inks, not DFS |
