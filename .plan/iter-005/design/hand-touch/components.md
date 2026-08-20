---
ui: [UI-EP-06]
iter: iter-005
---

# Components — [UI-EP-06]

Closed inventory. Compose by copy-paste from each self-contained `.html`.

| Name | Kind | Source | Pattern id | File | Variants | States | A11y |
|---|---|---|---|---|---|---|---|
| ToolChip | screen | reuse UI-EP-04 | `.c-tool-chip` | `./components/tool-chip.html` | 3 clusters · 64 du | default, armed, pressed, dimmed, queued | `role=toolbar`; tile `aria-label`; ≥64×64 |
| HandTouchToggle | screen | build | `.c-hand-touch-cluster` `.c-hand-touch-btn` | `./components/hand-touch-toggle.html` | 64 du 1-bit tile, label **HT** | on (default, invert), off (paper), pressed | `aria-pressed`; `aria-label`; ≥64×64 |
| SelectionOverlay | screen | reuse UI-EP-02 | `.c-bounds` `.c-handle` | `./components/selection-overlay.html` | SmartGroup AABB | hidden, selected, moving, resizing, handle pressed | handle `aria-label`; hit 10 mm finger and pen |
| FingerContact | screen | build | `.c-finger` | `./components/finger-contact.html` | one / two / ignored / down / travel | preview-only filled circle + travel tick | not product chrome |

**Kind**

- `screen` — stays under this package’s `components/`
- No new system comps this revision (unique icons only under `design/system/assets/`)

## Reuse rules

- ToolChip is UI-EP-04 (3 exclusive + 2 toggles + Undo/Redo). **Do not** revert to four exclusive tools.
- Overlay is UI-EP-02 dotted AABB + hollow knobs. Knobs are **finger-eligible** (10 mm hit).
- `btn.hand_touch` is a **kill-switch** for canvas fingers — **not** a pan-mode / hand-tool tile. Same family as Debug / Follow / USB (64×64 du, 14 px label). Trailing orientation-top row, **left of Debug**. Follow toggle stays in UI-EP-07. Debug log panel is field debug — placement context only.
- `ind.two_finger_pan` has **no extra product chrome** — world transform + status text only. Must **not** imply Infini always matches (follow off = Infini unchanged).
- One-finger empty pan uses the same world-translate language; palm-rest keeps the world **unshifted**. Travel ticks in GestureAnnotate are preview-only (8 mm vs 36 mm; **20 mm** tick on pan).

## Banned

- Hover / focus / cursor chrome
- Motion / CSS transition
- Ghost / marquee as move stand-in
- Phone chrome
- Infini slate/teal on the panel
- Follow-toggle buttons (STORY-EP-053 / SRS-EP-50)
- Debug log panel as product inventory
- One-finger empty pan **without** the 20 mm / 178 du threshold (palm would pan)
- Two-finger pan copy that implies Infini match when follow is off
