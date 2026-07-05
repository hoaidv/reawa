---
feature: pen-input-absolute
parent_req: [REQ-04]
version: 1.0.0
lifecycle: active
---

# SRS — Absolute Pen Input (UI)

## [SRS-RW-44] Picker overlay UX

- Full-desktop overlay on all displays.
- Hover highlights window under cursor (global cursor poll; per-screen highlight rendering).
- Click selects window; Esc cancels → Relative.
- Pen input paused during picker; trackpad/mouse remain usable.

## [SRS-RW-45] Snapped region overlay

- Colored border (configurable `borderColor`; default `#3B82F6`); no desktop dim.
- Four corner resize handles; aspect-locked resize syncs snapped window.
- Hidden when target minimized (including Stage Manager); reappears on restore.

## [SRS-RW-46] Menu bar Absolute context

| Element | When visible |
|---|---|
| **Absolute** menu item | Always (when connected) |
| **Sending to …** | Absolute + window snapped |
| **Choose window** | Absolute mode |

## [SRS-RW-47] Settings Absolute context

When output mode Absolute selected and window bound: **Snapped window** field shows bound window name/reference.

---

## Superseded

_None yet._
