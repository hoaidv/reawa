---
iter: iter-004
date: 2026-08-16
status: complete
participants: analyst, pm, architect, sm, designer, dev, qa
---

# Iter 004 Retrospective

> SM close-iter 2026-08-16. Human verified TRACK-004 on device. PM retro-gate in the same session (human asked to close and open empty iter-005).

## What went well

- Design-first W1 (EP-026 / EP-027) then vertical implement: ToolChip, enclose dispatch, connector recognize + warp, Infini mirror (IN-030) and demo-mix (IN-032).
- Human RM2 checks caught origin/stale ingest (EP-033) and StrokeSync TCP-down vs gadget-down (EP-034 vs EP-036).
- WIP 2 dual-cursor (IN-030 ∥ EP-033, then EP-034 ∥ live review) without stealing enclose/ingest files.
- No-UDC rule after the brick: TCP retry only; physical unplug is the only safe re-enum.

## What to improve

- Do not slice “restore USB without unplug” until someone inspects Linux gadget/host enumeration. EP-036 over-claimed restore; keepalives belong on EP-034.
- Board/MASTER lagged PM gates (EP-034 still “HOLD” after human-stable). Rollup the cursor the same day as the gate.
- Enclose `"his"` FP needs measurement (EP-035) before another PIP/containment idea. Park it as a small enhancement, not a campaign closer.
- `adlc gate` still mixes `reawa/*` orphans with campaign signal (carry from iter-003).

## Iter memory reviewed

- [usb-gadget-no-software-unplug.md](./memory/usb-gadget-no-software-unplug.md)
- _no other files under `memory/`_

## Memory captured

- **Project** → [rm2-usb-gadget-no-software-unplug.md](../../.docs/memory/rm2-usb-gadget-no-software-unplug.md)
- **ADLC** → _none proposed this close_
- **Design system** → `.plan/iter-004/design/system/assets/**` → `.docs/design/system/assets/` (ToolChip + USB HUD icons). `_none` under `system/components/`

## Upstream signals

- Story `cancelled` is used in plan but not named in the story template status list — agents guess `cancelled` vs `done` with a strike note.
- `_none further this iter_`

## Persona reflections

- **analyst**: REQ-09 + REQ-03 ToolChip was the right Must; REQ-08 / CHL-0011 / CHL-0012 stayed parked. Empty-letter boxes as acceptable FP is a product call, not a bug.
- **pm**: Human verified connectors + Infini live drag. EP-036 cancelled rather than left `done`. Iter-005 stays empty until the next wave PRD; EP-035 is one enhancement, not the campaign.
- **architect**: ADR-0020/21/22 held. Do not invent UDC restore. EP-032 chrome owner still needed; not a reason to keep iter-004 open.
- **sm**: Cursor hygiene slipped after USB gates. Close when human verifies; carry EP-035; cancel EP-036; do not auto-slice iter-005.
- **designer**: UI-EP-04 / UI-EP-05 current. System SVGs promoted; screen packages stay in iter-004. USB HUD icons were late ops, still shared assets.
- **dev**: StrokeSync 5s TCP retry + keepalives recovered Infini without gadget bounce. `modprobe -r g_ether` / UDC pull is forbidden. Enclose A/L is log-only when it runs.
- **qa**: Host BDD covered IN-030/032 and EP-033 analog; USB classes needed live ping vs Infini-down split. EP-035 needs device `[recog]` corpus, not a threshold assert.

## Carry-over to iter-005

| Item | Disposition |
|---|---|
| [STORY-EP-035](./stories/STORY-EP-035.md) | **carry** — small enclose A/L enhancement; not committed until wave pick |
| [STORY-EP-032](./stories/STORY-EP-032.md) | **parked** in iter-004 (`draft`) — `/architect` later |
| [STORY-EP-036](./stories/STORY-EP-036.md) | **cancelled** — no gadget restore without Linux inspect |
| epaper `[REQ-08]`, CHL-0011, CHL-0012 | future — not auto-sliced |
| iter-005 draft REQs (BS-0002) | human picks wave; SM does not slice yet |
