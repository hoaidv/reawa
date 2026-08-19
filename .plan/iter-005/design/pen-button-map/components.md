# Components — pen-button-map

Infini desktop settings. Unique `pen-map-*` names so this lane does not overwrite sibling `hand-touch` files.

| Component | Kind | File | Pattern | States |
|---|---|---|---|---|
| PenMapButton | system | `../system/components/pen-map-button.html` | `.c-pen-map-btn` | default, hover, focus-visible, active, disabled, loading (`aria-busy`) |
| PenMapSelect | system | `../system/components/pen-map-select.html` | `.c-pen-map-select` `.c-pen-map-option` | closed, open, hover, focus-visible, active, disabled, invalid, selected option |
| PenMapEditor | screen | `./components/pen-map-editor.html` | `.c-pen-map-editor` | default; 0-button empty |
| PenMapButtonRow | screen | `./components/pen-map-button-row.html` | `.c-pen-map-row` | present (index 1\|2) |
| PenMapStatus | screen | `./components/pen-map-status.html` | `.c-pen-map-status` | stale, offline, published |

## Anatomy

**PenMapButton** — label + optional leading icon. Primary fill on hover; outline at rest. Min 24×24.

**PenMapSelect** — labelled trigger showing current catalogue label + icon; expanded `listbox` of closed ids only. Click vs Hold-move are **separate instances** with different option sets.

**PenMapEditor** — `h1`, capability caption, optional status, stacked rows, Save, settings note.

**PenMapButtonRow** — “Button n” heading, SlotClick, SlotHoldMove.

**PenMapStatus** — icon + text (never color alone).
