---
from: designer
to: sm
iter: iter-003
date: 2026-08-13
subject: device-selection-chrome-done
cc: [pm, architect, qa, dev]
verdict: READY-WITH-CONCERNS
---

# Designer → SM — STORY-EP-012 done `[UI-EP-02]`

## Delivered

| Artifact | Path |
|---|---|
| Package | `.plan/iter-003/design/device-selection-chrome/` |
| Spec | `ui-spec.md` (`[UI-EP-02]`) |
| Navigator | `index.html` (`data-preview-scale="tablet"` @ 100%, 1872×1404) |
| Primary / hifi | `device-selection-chrome-sel-selected.html` |
| States | `device-selection-chrome-states.html` |

Nine package-contained scenes: `sel.none` · `selected` · `moving` · `resizing.with_bounds` ·
`resizing.fixed_ink` · `deselected` · `create_refused` · `unavailable` · `reloaded`.
ToolChip composed from [UI-EP-01](../design/epaper-tool-strip/). **0 ghost / marquee scenes.**

[STORY-EP-018](../stories/STORY-EP-018.md) and [STORY-EP-019](../stories/STORY-EP-019.md) have
`ui_spec` / `scenes` / `hifi` copied; **status left `draft`**.

`.docs/design/index.md` row for `[UI-EP-02]`; infini `ink-box-ui` marked deprecated (not deleted).

## Spike

| Question | Result |
|---|---|
| Handle size / hit | **Propose 28 du visual / 56 du hit** @ 226 dpi. OPEN until architect confirms. |
| LOD cutoff | **Propose min on-panel axis < 96 du**. OPEN until architect confirms. Not TILE_LOD_SCALE 0.35. |
| Undo on three-tool chip | Does not fit. **[CHL-0010](../challenges/CHL-0010-undo-vs-selection-create-chrome.md)** → PM |
| Selection-create invocation | Out of v1 chrome (`cta.create_smart_group`). Same **CHL-0010**. |
| Chrome vs dense 1-bit ink | Closed in Spec: double-rail bounds, filled handles, hatch toggle/indicators. |

## Concerns

- [CHL-0002](../challenges/CHL-0002-epaper-platform-gate.md) — `data-platform=epaper` vs mechanical gate allowlist (engine patch pending human ADLC). Spec/SRS authoritative.
- Handle / LOD numbers are **proposals**, not implement locks. EP-019 must not invent 8 CSS px or 0.35.

## Ask

1. `/sm` — flip EP-018 / EP-019 to `ready` only when W9+ lock allows (not now).
2. `/pm` — triage CHL-0010 (undo + create invocation).
3. `/architect` — confirm 28/56 du and 96 du LOD (or counter-propose in device units).
4. `/qa` — BDD against Spec states after architect confirms constants, or tag open constants.
