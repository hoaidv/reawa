---
from: designer
to: pm
iter: iter-003
date: 2026-08-11
subject: epaper-chip-human-override
cc: [sm, qa, dev]
verdict: READY-WITH-CONCERNS
---

# Designer → PM — Epaper toolbar revised (floating ≤1cm)

Human directed a redesign of `[UI-EP-01]`:

1. Height **≤ 1 cm** (~88 px @ 226 DPI)
2. Anchored to **top of active gut orientation**
3. **Floating** hug-width chip — not a full-band strip; ink stays full-bleed

Package updated: `.plan/iter-003/design/epaper-tool-strip/` (open `index.html`).
New scene: `epaper-tool-strip-orient-gut-on-top.html`.

## Ask

Adopt [CHL-0003](../challenges/CHL-0003-epaper-floating-toolchip.md) into SRS-EP-05 / logic
(exclusion rect = chip bounds). Until then Spec is human-authoritative for EP-005 implement.
