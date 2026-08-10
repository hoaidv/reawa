---
version: 0.1.0
lifecycle: active
---

# DESIGN.md — Project design contract

Binding contract for UI agents. Infini (Electron desktop) + future Reawa surfaces.

## Intent

**Infini:** quiet desktop drawing companion — the canvas *is* the product surface; chrome stays
minimal. Cool slate paper + ink strokes; never purple-gradient SaaS or cream/terracotta cliché.

## Tokens

Source: `.docs/design/tokens.json` · CSS: `.docs/design/tokens.css`. Screen packages may
subset into package `tokens.css` (same roles).

## Type

| Role | Token | Face |
|---|---|---|
| display | fontSize.display | Source Serif 4 |
| title | fontSize.title | DM Sans |
| body | fontSize.body | DM Sans |
| caption | fontSize.caption | DM Sans |

## Spacing & shape

- Base grid: 8px (`spacing.sm`)
- Radii: `radius.control` only for chrome chips; canvas has **no** card radius
- Shadows: banned on canvas; optional soft on StatusZoom only
- Desktop window: min ~960×640; canvas full-bleed in client area

## Platform and responsive profiles

| Profile | Frames | Resize | Input |
|---|---|---|---|
| **desktop** (Infini) | Electron window | Center world anchor | pointer + keyboard; hover required |

## Accessibility baseline

- WCAG AA; focus-visible on CanvasStage; status not by color alone (zoom uses text `%`)

## Component patterns

| Component | Pattern | States | Treatment |
|---|---|---|---|
| CanvasStage | `.c-canvas-stage` | hover, focus-visible, active | Full-bleed; grab/grabbing |
| ZoomReadout | `.c-zoom-readout` | (display) | Top-trailing overlay |
| WorldLayer | `.c-world-layer` | — | Transform host for grid/figures |

## Banned

- Cards / pill clusters / stat strips on Infini canvas
- Purple-indigo themes; cream+#terracotta+serif marketing look
- Hover-only discovery without focus path
- Invented toolbars not in SRS inventory
