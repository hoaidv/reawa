---
iter: iter-003
date: 2026-08-14
status: complete
participants: analyst, pm, architect, sm, designer, dev, qa
---

# Iter 003 Retrospective

> SM close-iter 2026-08-14. Campaign **Epaper owns the document** exited (human). PM retro-gate before iter-004.

## What went well

- CHL-0008 invert (device owns the tree) ended the four-wave snap-back loop; local enclose / membership / manip no longer wait on Infini.
- Wave discipline (W8 latency gate → W12 handshake) kept WIP 2; parallel lanes (EP-020 ∥ IN-028) used shared SRS-IN-09 names without inventing wire types.
- Human-in-the-loop RM2 checks (enclose, ToolCanvas, sync) caught `RM_SYNC_HOST` and missed-hello gaps that host tests could not.
- Design stories (EP-012 / EP-022 / EP-023) landed before implement UI; CHL-0013 Enclose CTA and four-tool chip stayed consistent.

## What to improve

- Handshake `hello` / renderer remount was a silent seq-gap: stroke previews looked synced while `doc_change` did not. Buffer or retry hello in the first ship, not as a hotfix.
- `deploy-rm2.sh` does not persist `RM_SYNC_HOST`; every deploy without it looks like a product regression.
- Repo-wide `adlc gate` still mixes `reawa/*` and scanner orphans with campaign signal.
- Stretch 45 vs capacity 15 repeated the pilot; W8-only NOW helped but the board still over-committed the calendar.

## Iter memory reviewed

- [ep-004-rm2-touch-spike.md](./memory/ep-004-rm2-touch-spike.md)

## Memory captured

- **Project** → [rm-sync-host-deploy.md](../../.docs/memory/rm-sync-host-deploy.md)
- **ADLC** → _none proposed this close_
- **Design system** → `.plan/iter-003/design/system/assets/**` → `.docs/design/system/assets/` (icons). `_none` under `system/components/`

## Upstream signals

- `adlc gate` cannot scope FAILS to the execution lock (`reawa/*`, C++ `@implements` not scanned) — campaign green is unreadable.
- Deploy scripts that optionally forward env should fail closed or persist last-known `RM_SYNC_HOST` on-device.
- `_none further_`

## Persona reflections

- **analyst**: Ownership invert (CHL-0008) was the right problem restatement; REQ-08 correctly stayed thickened-not-built. Carry nested enclose / FREE_FORM as future, not Must.
- **pm**: W12 gate READY-WITH-CONCERNS; human confirmed REQ-07 path. Full-repo gate FAIL owned. Did not slice REQ-08. Campaign `validated_by` human 2026-08-14.
- **architect**: ADR-0014/0015 + SRS-IN-09 transmit names held. Handshake needed an explicit resend/adopt path; do not assume renderer subscribe order.
- **sm**: Board waves + conflict edges worked. Do not open the next Must until PRD arrives. Residue EP-007…011 / IN-020…026 stays blocked in this iter.
- **designer**: UI-EP-01/02/03 current; ink-box-ui deprecated. System assets promoted; screen packages stay in iter-003. `_none — no system component HTML WIP_`
- **dev**: Device `commitOp` + Infini viewer applier is the live SoT. Preview `stroke_*` ≠ document. Always set `RM_SYNC_HOST` on deploy.
- **qa**: Host suites mapped AC; hardware confirm still required for sync. BDD-before-dev on W12 prevented wire-name drift.

## Carry-over to iter-004

| Item | Disposition |
|---|---|
| epaper `[REQ-08]` any-node manip | **not committed** — await human PRD |
| CHL-0011 nested enclose | future — not auto-sliced |
| CHL-0012 FREE_FORM / align-content | future — not auto-sliced |
| ADR-0019 amend (CHL-0018 option 2) | deferred |
| Residue EP-007…011 / IN-020…026 | stay **blocked** on iter-003 |
| Next PRD | human will supply — iter-004 scaffold only |
