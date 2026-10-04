---
source: .plan/iter-005/retro.md (dev reflection)
captured: 2026-10-04
related:
  - STORY-EP-067
  - STORY-EP-081
  - ADR-0041
---

# Lessons from `epaper_old/` for the rebuilt Epaper

Keep these when building the document forest in `epaper/`:

- **One id allocator.** Before STORY-EP-067, node ids were minted separately in
  `stroke_capture`, `erase_commit`, `recognize_connector` and `surround_create`.
- **Every commit logs a reason**, such as `noop` or `duplicate_id:…`. The brush-erase defect
  was found from that reason, not from geometry.
- **Repaint only the damaged area.** An empty damage set falls back to a full-panel repaint.

Path drift: memory entries written before 2026-10-03 cite `epaper/{document,drawing,rendering,tests}/`.
Those files now live under `epaper_old/`. The heaviest are
[plan_toolaction_context_ui](./plan_toolaction_context_ui.md),
[plan_epaper-tool-system-refactor](./plan_epaper-tool-system-refactor.md) and
[plan_dissolve_host_bags](./plan_dissolve_host_bags.md).
