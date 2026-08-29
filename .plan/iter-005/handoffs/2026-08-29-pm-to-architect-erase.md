---
from: pm
to: architect
date: 2026-08-29
iter: iter-005
---

# Hand-off: Product Manager → Architect

## Context

Human promoted [eraser-product.md](../eraser-product.md) as the **single** REQ-11 product document. Do **not** scatter into srs-product / srs-experience / srs-ui. UI/UX in that file is normative.

Adopted [CHL-0028](../challenges/CHL-0028-eraser-three-tools.md). Canonical PRD: [prd-erase.md](../../../.docs/modules/epaper/prd-erase.md). Module PRD stubs: [prd.md REQ-11](../../../.docs/modules/epaper/prd.md#erase) (version `0.14.0-draft`); REQ-03 six exclusives + HT-on-chip; REQ-18 last-used eraser.

## Review verdict

**READY-WITH-CONCERNS**

| Class | Finding |
|---|---|
| Strength | One job, three tools, measurable AC in §14; Frame never; Path B retired; clip not sample-drop |
| Concern | Brush proximity circle is a **field-test kill-switch** (ship on) |
| Concern | World millimetres vs existing unnamed `strokeWidth: 2` ([ADR-0012](../../../.docs/adr/ADR-0012-world-stroke-viewport-parity.md) never defined 1 world unit). Erase constants are mm; do **not** introduce a `Dimension` type in the core this wave |
| Gap | none blocking bind |

`prd-check` run with this handoff.

## Asks

1. One feature SRS (do not fork UI/UX out of prd-erase).
2. ADR for clip + remnant split.
3. Retire Path A/B SRS in place (SRS-EP-27…30).
4. Domain: boundary polyline; barrel last-used.

## Constraints

- Designer this wave: **three icons only**.
- Human is QA; no BDD gate required before implement stories.

## Out of scope

- Clipboard, connector ends, Device Settings implement, Infini undo apply.
- Size slider; Dimension-in-core; REQ-13 endpoint-ink.
