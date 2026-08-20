---
ui: [UI-EP-07]
iter: iter-005
---

# Components — [UI-EP-07]

Closed inventory. Compose by copy-paste from each self-contained `.html`.

| Name | Kind | Source | Pattern id | File | Variants | States | A11y |
|---|---|---|---|---|---|---|---|
| FollowToggle | screen | build | `.c-follow-cluster` `.c-follow-btn` | `./components/follow-toggle.html` | 10 mm lone cluster, trailing of panel | off, following, pressed, peer-off (tappable), unavailable | `aria-pressed`; `aria-label`; `aria-disabled` when no session; ≥10 mm |
| ToolChip | screen | reuse UI-EP-04 | `.c-tool-chip` | `./components/tool-chip.html` | 3 clusters · 10 mm tiles | default, armed, pressed, dimmed, queued | `role=toolbar`; tile `aria-label`; ≥10 mm |

**Kind**

- `screen` — stays under this package’s `components/`
- No new system component HTML this story. Unique icon: `../system/assets/icon-epaper-viewport-follow.svg`

## Reuse rules

- ToolChip is UI-EP-04 (3 exclusive + 2 toggles + Undo/Redo). **Do not** add follow as a fourth exclusive tile, recognizer, or hand-tool.
- FollowToggle parent is DeviceScreen. Not nested in ToolChip. Chip hits still win when the finger is on the chip.
- 1-bit paper/ink; fill + hatch. Press = invert. Hover / focus / cursor / motion are N/A.

## Banned

- ToolChip exclusive-tool membership for follow
- Hand-tool tile
- Last-writer / token chrome ([ADR-0023](../../../../.docs/adr/ADR-0023-viewport-last-writer.md) superseded)
- Dual-on (this toggle on while Infini is following)
- Restoring follow on reconnect without a tap
- Hover / focus / cursor chrome
- Infini slate/teal on the panel
