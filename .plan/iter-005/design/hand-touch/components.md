---
ui: [UI-EP-06]
iter: iter-005
---

# Components — [UI-EP-06]

Closed inventory. Compose by copy-paste from each self-contained `.html`.

| Name | Kind | Source | Pattern id | File | Variants | States | A11y |
|---|---|---|---|---|---|---|---|
| ToolChip | screen | reuse UI-EP-04 | `.c-tool-chip` | `./components/tool-chip.html` | 3 clusters · 64 du | default, armed, pressed, dimmed, queued | `role=toolbar`; tile `aria-label`; ≥64×64 |
| SelectionOverlay | screen | reuse UI-EP-02 | `.c-bounds` `.c-handle` | `./components/selection-overlay.html` | SmartGroup AABB | hidden, selected, moving, resizing, handle pressed | handle `aria-label`; hit 10 mm finger and pen |
| FingerContact | screen | build | `.c-finger` | `./components/finger-contact.html` | one / two / ignored / down / travel | preview-only filled circle + travel tick | not product chrome |

**Kind**

- `screen` — stays under this package’s `components/`
- No new system comps this story (unique icons only under `design/system/assets/`)

## Reuse rules

- ToolChip is UI-EP-04 (3 exclusive + 2 toggles + Undo/Redo). **Do not** revert to four exclusive tools.
- Overlay is UI-EP-02 dotted AABB + hollow knobs. Knobs are **finger-eligible** (10 mm hit). **Do not** add a hand-tool tile.
- `ind.two_finger_pan` has **no extra product chrome** — world transform + status text only. Must **not** imply Infini always matches (follow off = Infini unchanged).
- One-finger empty pan uses the same world-translate language; palm-rest keeps the world **unshifted**. Travel ticks in GestureAnnotate are preview-only (8 mm vs 36 mm; 10 mm tick on pan).

## Banned

- Hover / focus / cursor chrome
- Motion / CSS transition
- Ghost / marquee as move stand-in
- Phone chrome
- Infini slate/teal on the panel
- Follow-toggle buttons (STORY-EP-053 / SRS-EP-50)
- One-finger empty pan **without** the 10 mm / 89 du threshold (palm would pan)
- Two-finger pan copy that implies Infini match when follow is off
