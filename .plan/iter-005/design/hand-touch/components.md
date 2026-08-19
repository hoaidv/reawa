---
ui: [UI-EP-06]
iter: iter-005
---

# Components — [UI-EP-06]

Closed inventory. Compose by copy-paste from each self-contained `.html`.

| Name | Kind | Source | Pattern id | File | Variants | States | A11y |
|---|---|---|---|---|---|---|---|
| ToolChip | screen | reuse UI-EP-04 | `.c-tool-chip` | `./components/tool-chip.html` | 3 clusters · 64 du | default, armed, pressed, dimmed, queued | `role=toolbar`; tile `aria-label`; ≥64×64 |
| SelectionOverlay | screen | reuse UI-EP-02 | `.c-bounds` `.c-handle` | `./components/selection-overlay.html` | SmartGroup AABB | hidden, selected, moving, resizing, handle noop/pressed | handle `aria-label`; hit 56 du pen-only |
| FingerContact | screen | build | `.c-finger` | `./components/finger-contact.html` | one / two / ignored | preview-only | decorative `alt=""` + nearby caption |
| AnchorNoopIndicator | screen | build | `.c-indicator` | `./components/anchor-noop-indicator.html` | visible | hatch + text | not color-only |

**Kind**

- `screen` — stays under this package’s `components/`
- No new system comps this story (unique icons only under `design/system/assets/`)

## Reuse rules

- ToolChip is UI-EP-04 (3 exclusive + 2 toggles + Undo/Redo). **Do not** revert to four exclusive tools.
- Overlay is UI-EP-02 double-rail + 8 filled handles. **Do not** add finger-resize gizmos or a hand-tool tile.
- `ind.two_finger_pan` has **no extra product chrome** — world transform + status text only.

## Banned

- Hover / focus / cursor chrome
- Motion / CSS transition
- Ghost / marquee as move stand-in
- Phone chrome
- Infini slate/teal on the panel
- One-finger empty as pan
- Finger-eligible handles (&lt;64 du)
