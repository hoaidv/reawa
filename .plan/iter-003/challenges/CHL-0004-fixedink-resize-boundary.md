---
id: CHL-0004
title: fixedInk resize mutated bounds instead of scale — boundary ink stuck
status: resolved
resolution: adopted
severity: normal
raised_by: architect
resolved_by: pm
iter: iter-003
date: 2026-08-11
related: [SRS-IN-11, SRS-IN-09, SRS-EP-04, REQ-04]
---

# CHL-0004 — fixedInk resize vs boundary transform

## Conflict

Human verify: desktop resize **moves/scales content** but the **surrounding boundary ink stays put**. Pilot also reports ink-boxes do not feel like `fixedInk`, and tablet cannot resize handles / move ghost lacks content.

## Prior design

BR-09i / SRS-IN-11: **boundary ink always transforms** with the group; `fixedInk` only exempts **content** sample size (UV track). Resize must update `transform.scale*` (+ translate) so boundary × scale; must **not** only mutate local `bounds` under `fixedInk`.

## Evidence (code)

`smartTransformFromWorldAabb(..., "fixedInk")` returns unchanged transform and rewritten local bounds — boundary draw path ignores bounds width/height, so boundary ink does not stretch.

## Proposed resolution (Architect)

1. **Adopt** — amend SRS-IN-11 resize mapping (done 2026-08-11): both modes update scale+translate; fixedInk keeps local bounds fixed.
2. Dev: change `smartTransformFromWorldAabb` so `fixedInk` uses the same transform mapping as `withBounds`.
3. Default create already `fixedInk` in enclose/surround — verify after (2); add Infini `tgl.ink_scale_mode` if missing (srs-ui).
4. Epaper: implement handle hit-test → `tool_intent resize`; move ghost must translate ink preview with bounds (SRS-EP-04 amended).

## Resolution

**Adopted** 2026-08-11 by PM — restores BR-09i; not a scope expansion. Architect SRS amendments accepted. Dev implements IN-023 / IN-024 / EP-008 / EP-009 immediately (human: implement right then). No interrupt-track (severity normal; stays on TRACK-003).
