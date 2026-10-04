---
id: CHL-0034
author: pm
target: [REQ-04, REQ-07, SRS-EP-08, SRS-EP-09, ADR-0041]
severity: medium
status: deferred
opened: 2026-10-04
iter: iter-006
expedite: false
interrupts_track: ""
resolution: deferred
resolved_by: pm
resolved: 2026-10-04
raised_by: human
source: human 2026-10-04 — "the connection/sync features to Infini is out of implementation scope"
---

# CHL-0034 — Forest ↔ wire mapping and Infini sync for the rebuilt Epaper

## Context

The forest stores children relative to their container and replaces `inkScaleMode` with
`manipMode` (`All` / `Boundary`). `Boundary` does not re-centre content the way `fixedInk` does.
A box resized on the device could therefore look different on Infini. Step 0.2 of the
[forest plan](../../document-forest-implementation.md) asked for one of three choices:

- Infini adopts `manipMode`;
- the wire carries `manipMode` as a new field;
- Epaper sends geometry already resolved, so Infini only displays it.

On 2026-10-04 the human put the connection and sync with Infini out of the implementation scope,
to keep the forest work focused.

## Proposal

Defer the choice and every sync step until the human brings sync back into scope.

## Resolution

**Deferred** — 2026-10-04 (PM, human decision).

- Out of the forest implementation: the wire mapping (plan step 0.2), forest ↔ wire conversion
  (old step 9.2) and `doc_load` from the desktop (old step 9.1).
- Still in: local undo on the forest, which is device-only (plan step 9).
- Records that wait: [REQ-07](../../../.docs/modules/epaper/prd.md#one-way-sync) and
  [SRS-EP-08](../../../.docs/modules/epaper/features/device-document/srs-logic.md#srs-ep-08-one-way-sync)
  (one-way sync), [SRS-EP-09](../../../.docs/modules/epaper/features/device-document/srs-data.md#srs-ep-09-device-data)
  (wire binding), and two outcomes of
  [REQ-04](../../../.docs/modules/epaper/prd.md#device-document) (initial load, shared semantics).
  They stay `active` as the description of the archived application and of the target contract.
- No story is frozen; none was written.

**Reopen when** the human brings Infini sync back. Then pick one of the three choices above,
record it in an ADR, revise SRS-EP-09 and add a plan step for the conversion.

## Product doc updates

Done 2026-10-04: notes on REQ-04, SRS-EP-07 and SRS-EP-09; the plan's scope and step 9; ADR-0041
consequences.

## Consequences and verification

- The rebuilt Epaper runs standalone. Its document lives in memory only (REQ-04).
- Bulk loading (STR) is still built in step 4 for the R-tree; only its use by `doc_load` waits.
- Verification: none needed until reopened.
