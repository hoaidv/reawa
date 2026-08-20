# Components — device-selection-chrome `[UI-EP-02]`

| Component | Kind | Source | File | Pattern | States |
|---|---|---|---|---|---|
| ToolChip | screen | **compose** UI-EP-03 | `components/tool-chip.html` | `.c-tool-chip` | default, sel_rect active, sel_freeform active |
| ToolButton | screen | **compose** UI-EP-03 | (in tool-chip.html) | `.c-tool-btn` | default, active, pressed, unavailable |
| SelectionOverlay | screen | **build** | `components/selection-overlay.html` | `.c-bounds` `.c-handle` | hidden, selected, moving, resizing, handle pressed |
| InkScaleModeToggle | screen | **build** | `components/ink-scale-mode-toggle.html` | `.c-mode-toggle` | withBounds, fixedInk, pressed, hidden |
| CreateRefusedIndicator | screen | **build** | `components/create-refused-indicator.html` | `.c-indicator` | hidden, visible |
| ManipulationUnavailableIndicator | screen | **build** | `components/manipulation-unavailable-indicator.html` | `.c-indicator` | hidden, visible |

## Anatomy

### SelectionOverlay

- `ovl.selection_bounds` — double-rail rect in content-space
- `ovl.resize_handles` — 8 filled squares (nw n ne e se s sw w). **No rotation**
- Hit target 56 du wrapping 28 du visual (proposed)
- Child: InkScaleModeToggle at bottom-center of bounds

### InkScaleModeToggle

- Icon-only square, size = primary ToolChip tile (10 mm), attached to the box (not ToolChip)
- Label is visually hidden; icon swaps `withBounds` ↔ `fixedInk`

### Indicators

- Hatch chip + paper text + 1-bit SVG
- Pointer-events none (do not swallow pen)

### ToolChip (composed)

- Four 64×64 tiles: `sel_rect` · `sel_freeform` · Pen · Ink-box ([ADR-0017](../../../../.docs/adr/ADR-0017-four-tool-chip.md))
- Orientation-top float; radius 0
- **Do not add undo or Enclose** (CHL-0010 / ADR-0016)
