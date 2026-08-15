---
id: CHL-0021
author: pm
target: [SRS-EP-10]
severity: low
status: resolved
opened: 2026-08-15
iter: iter-004
expedite: false
interrupts_track: ""
---

# CHL-0021 — Adaptive enclose min + empty primitive gate

## Context

Human 2026-08-15: `kMinEncloseWorld = 28` is final. Empty boxes (boundary drawn first) are accepted as a feature, but arbitrary large loops must not become boxes (letters O/D/P/Q/G/C, scribbles).

## Resolution

**Adopted 2026-08-15 (pm).** Adaptive size: **28** with capturable content; **36** + near-primitive shape when empty. Recorded in BR-B03 / SRS-EP-10. Device implementation: `epaper/document/enclose_shape.hpp`.
