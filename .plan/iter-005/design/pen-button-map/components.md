---
ui: [UI-EP-08]
iter: iter-005
---

# Components — [UI-EP-08]

Closed inventory. Compose by copy-paste from each self-contained `.html`.

| Name | Kind | Source | Pattern id | File | Variants | States | A11y |
|---|---|---|---|---|---|---|---|
| PenMapOpen | screen | build | `.c-pen-map-open` `.c-pen-map-open-btn` | `./components/pen-map-open.html` | 10 mm lone cluster, leading of panel | rest, pressed, open (inverted) | `aria-pressed`; `aria-label="Pen buttons"`; ≥10 mm |
| SettingsShell | screen | build | `.c-settings` `.c-settings-item` | `./components/settings-shell.html` | master-detail; first item Pen buttons | current invert, close press | `h1` Settings; master `aria-current`; close `aria-label` |
| PenMapOverlay | screen | build | `.c-settings-detail` | `./components/pen-map-overlay.html` | 0-button empty | persist-wait hatch | caption copy |
| PenMapSlotRow | screen | build | `.c-pen-map-row` `.c-pen-map-option` | `./components/pen-map-slot-row.html` | index 1 \| 2; Click 3 + Hold 3 inline | present / absent; selected invert | group `h3`; option `aria-selected`; min-height 10 mm |
| PenMapList | screen | build | `.c-pen-map-list` `.c-pen-map-option` | `./components/pen-map-list.html` | Click 3 ids · Hold-move 3 ids | default, selected invert, pressed | `aria-selected`; 0 Undo / 0 temp freeform |
| ToolChip | screen | reuse UI-EP-04 | `.c-tool-chip` | `./components/tool-chip.html` | 3 clusters · 10 mm; temp-erase mirror | default, armed, pressed, dimmed, queued | `role=toolbar`; tile `aria-label`; ≥10 mm |

**Kind**

- `screen` — stays under this package’s `components/`
- No Infini system `pen-map-button` / hover-select. Unique 1-bit icons: `icon-epaper-pen-map-*.svg`

## Reuse rules

- ToolChip is UI-EP-04 (3 exclusive + 2 toggles + Undo/Redo). **Do not** add pen-map as a fourth exclusive tile, recognizer, or 5-way radio.
- SettingsPage parent is DeviceScreen. Not nested in ToolChip. Shown instead of drawing chrome.
- 1-bit paper/ink; fill + hatch. Press = invert. Hover / focus / cursor / motion are N/A.
- 0-button: omit SlotRow markup entirely.
- Do **not** invent extra Settings master items this package.

## Banned

- Infini File menu entry
- Save / publish desktop footer
- Undo on Click list
- Temporary freeform / rect on Hold-move list
- Hover / focus / cursor chrome
- Infini slate/teal on the panel
- Reusing `icon-pen-map-*` Infini assets as 1-bit truth
- Separate Click / Hold-move sheet scenes
