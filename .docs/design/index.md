---
version: 0.1.0
lifecycle: active
---

# Design index — final design → iter package

| UI / screen | Module / SRS | Current package | Iter | Supersedes | Status | Notes |
|---|---|---|---|---|---|---|
| Infini infinity canvas `[UI-IN-01]` | infini / [SRS-IN-02] | `.plan/iter-002/design/infinity-canvas/` | iter-002 | — | current | STORY-IN-001 |
| Infini ink-box tools `[UI-IN-02]` | infini / [SRS-IN-14] | `.plan/iter-003/design/ink-box-ui/` | iter-003 | — | **deprecated** | Desktop mouse+ghost. Superseded on-device by `[UI-EP-02]`. Keep row; do not delete. STORY-IN-013 |
| Epaper tool strip `[UI-EP-01]` | epaper / [SRS-EP-05] | `.plan/iter-003/design/epaper-tool-strip/` | iter-003 | — | **superseded** | Four-tool chip. Replaced by `[UI-EP-04]`. |
| Device selection chrome `[UI-EP-02]` | epaper / [SRS-EP-12] | `.plan/iter-003/design/device-selection-chrome/` | iter-003 | `[UI-IN-02]` (on-device) | current | STORY-EP-012 + EP-023 four-tool rebase. Live ink; 0 ghost. |
| Selection enclose chrome `[UI-EP-03]` | epaper / [SRS-EP-12] | `.plan/iter-003/design/selection-enclose-chrome/` | iter-003 | — | current | STORY-EP-022. Rect + freeform (CHL-0014) + 6 anchors + Enclose CTA. Composes UI-EP-01/02. |
| ToolChip recognizers `[UI-EP-04]` | epaper / [SRS-EP-05] | `.plan/iter-004/design/toolchip-recognizers/` | iter-004 | `[UI-EP-01]` | current | STORY-EP-026. 3 tools + 2 toggles + Undo/Redo. |
| Connector chrome `[UI-EP-05]` | epaper / [SRS-EP-19] | `.plan/iter-004/design/connector-chrome/` | iter-004 | — | current | STORY-EP-027. Blink + Ink/Curve + Edge/Centre. |
| Epaper hand-touch `[UI-EP-06]` | epaper / [SRS-EP-22] | `.plan/iter-005/design/hand-touch/` | iter-005 | — | current | STORY-EP-037. One-finger pick/move + two-finger pan. 64 du finger rule. |
| Infini pen-button map `[UI-IN-03]` | infini / [SRS-IN-24] | `.plan/iter-005/design/pen-button-map/` | iter-005 | — | current | STORY-IN-034. Desktop settings; not SRS-IN-05 open/save. |

## Changelog

| Date | Screen | From iter | To iter | Reason |
|---|---|---|---|---|
| 2026-08-10 | infinity-canvas | — | iter-002 | Initial design story |
| 2026-08-11 | ink-box-ui | — | iter-003 | Smart Group pilot chrome |
| 2026-08-11 | epaper-tool-strip | — | iter-003 | REQ-03 three-tool strip + touch fallback |
| 2026-08-13 | ink-box-ui | iter-003 | — | Marked deprecated — mouse/ghost; on-device chrome is UI-EP-02 |
| 2026-08-13 | device-selection-chrome | — | iter-003 | SRS-EP-12 selection overlay (STORY-EP-012) |
| 2026-08-14 | ToolChip recognizers | iter-003 UI-EP-01 | iter-004 UI-EP-04 | ADR-0021 3+2+Undo/Redo |
| 2026-08-16 | USB HUD icons | — | iter-004 assets | Promoted `icon-epaper-usb-*` (+ ToolChip SVGs) to `.docs/design/system/assets/` |
| 2026-08-19 | hand-touch | — | iter-005 UI-EP-06 | STORY-EP-037 W1-A |
| 2026-08-19 | pen-button-map | — | iter-005 UI-IN-03 | STORY-IN-034 W1-B |
