---
id: CHL-0010
title: Undo and selection-create cannot share the three-tool chip
author: designer
target: [SRS-EP-12]
severity: medium
status: deferred
resolution: deferred
resolved_by: pm
resolved: 2026-08-13
opened: 2026-08-13
iter: iter-003
expedite: false
interrupts_track: ""
raised_by: designer
source: STORY-EP-012 spike
---

# CHL-0010 — Undo and selection-create cannot share the three-tool chip

## Context

[STORY-EP-012](../stories/STORY-EP-012.md) spike (SRS-EP-12 Open) asked whether an **undo
affordance** fits the three-tool chip and **how selection-create is invoked**.

The chip is a closed inventory from [SRS-EP-05](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md) /
[UI-EP-01](../design/epaper-tool-strip/ui-spec.md): Selection · Pen · Ink-box. Adding a fourth
tool contradicts [srs-experience](../../../.docs/modules/epaper/features/ink-box/srs-experience.md)
anti-invent ("A fourth tool for undo — Open"). A properties panel is banned by
[SRS-EP-12](../../../.docs/modules/epaper/features/ink-box/srs-ui.md).
`cta.create_smart_group` is **out of v1 chrome** until this is answered.

Designer will **not** invent a fourth chip slot, a properties panel, or a gesture that is not
in the SRS. The refuse scene (`sel.create_refused`) still ships — it shows
`ind.create_refused_no_surround` after create has already been invoked; it does not introduce
the invocation control.

## Proposal

PM chooses one (or defers both out of this campaign):

1. **Undo** — device-side gesture with **no on-panel control** this iter (two-finger / button
   hardware if any; else document as "no affordance until REQ-04"), **or** replace a chip tool
   (rejected by EP-003 inventory), **or** a later fourth chip (new SRS).
2. **Selection-create invocation** — keep out of v1 chrome (enclose-with-Ink-box is the working
   path the refuse state already teaches), **or** specify a pen gesture on an existing selection
   that does not add a CTA, **or** add `cta.create_smart_group` via adopted SRS.

Until adopted: [STORY-EP-018](../stories/STORY-EP-018.md) must not invent a second product.
[UI-EP-02](../design/device-selection-chrome/ui-spec.md) records both as **open / CHL-0010**.

## Resolution

**Deferred** — 2026-08-13 (PM). Both on-panel questions stay out of this campaign.

1. **Undo chrome** — no on-panel control this iter. The three-tool chip stays closed
   ([SRS-EP-05](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md): Selection · Pen ·
   Ink-box). A fourth ToolChip slot is not added. A properties panel is not invented. Hardware
   / two-finger undo is unspecified (no verified RM2 button path in product docs). The **undo
   ring still ships** in [STORY-EP-015](../stories/STORY-EP-015.md) with no chrome — recoverability
   is the ring, not a visible control ([architecture](../../../.docs/modules/epaper/architecture.md)
   already: “undo logic ships regardless”).
2. **Selection-create invocation** — `cta.create_smart_group` stays **out of v1 chrome**.
   Enclose-with-Ink-box remains the create path this campaign
   (`journey.device_enclose`). The refuse scene (`sel.create_refused` /
   `ind.create_refused_no_surround`) still ships; it does not introduce an invocation control.
   No pen gesture is invented that is not already in the SRS.

Not reject: both capabilities remain in logic ([SRS-EP-07] ring, [SRS-EP-10] selection-create).
Not adopt: no new chrome, no fourth chip, no new SRS. Reopen in a later campaign if a hardware
gesture is verified or a new SRS authorizes a fourth chip / CTA.

**Stories:** [STORY-EP-018](../stories/STORY-EP-018.md) stays **frozen at `draft`** (no invocation
to implement). EP-015 is **not** frozen. Do not flip EP-018 / EP-019 to `ready` from this triage
(SM + wave). Not expedite. Does not ungate W9.

## Product doc updates

- [SRS-EP-12 Open](../../../.docs/modules/epaper/features/ink-box/srs-ui.md#open-needs-design) —
  **Undo affordance** and **Selection-create invocation** rows: deferred this campaign (this
  challenge). Handle size / LOD rows untouched (architect).
- [SRS-EP-12 closed inventory](../../../.docs/modules/epaper/features/ink-box/srs-ui.md) —
  `cta.create_smart_group` remains out of v1; enclose-with-Ink-box is the create path.
- [srs-experience anti-invent](../../../.docs/modules/epaper/features/ink-box/srs-experience.md) —
  fourth tool for undo → Defer (this campaign).
- [SRS-EP-05 Open](../../../.docs/modules/epaper/features/tool-modes/srs-ui.md) — undo affordance
  points at this deferral.
- [REQ-04 Open Questions](../../../.docs/modules/epaper/prd.md#open-questions) — undo affordance
  deferred this campaign; depth-20 ring still ships.

## Interrupt / expedite (when applicable)

Not expedite. Does not block EP-012 package ship; blocks EP-018 invocation and any undo chrome.
