---
from: analyst
to: pm
date: 2026-08-20
iter: iter-005
---

# Hand-off: Analyst → Product Manager

## Context

Amended [BRD-07](../../../.docs/brd.md#brd-07-infinity-canvas--tablet-sync-infini--epaper) (Infinity canvas + tablet sync) from the human decision of 2026-08-20. BRD `status: approved`; `last_review: 2026-08-20`. Analyst did not edit module PRDs, ADRs, stories, or design packages.

## What changed in BRD-07 (old → new)

1. **Old:** desktop Infini owns pan/zoom viewing; Epaper owns local ink. **New:** independent cameras by default; each product pans/zooms its own viewport. Infini still navigates its own canvas and remains persistence home; it does not exclusively own the tablet camera.
2. **Old:** Infini viewport drives the RM drawing region (always-on). **New:** optional one-way follow; follower camera matches leader; exactly one direction at a time; follow off on disconnect.
3. **Old:** on-device pan/zoom on Epaper deferred. **New:** in-scope — two-finger pan/zoom plus local one-finger pan on empty canvas (independent camera).
4. **Chrome:** viewport-follow **icon toggle** on both products (Epaper follows Infini; Infini follows Epaper). Not a ToolChip exclusive “hand tool” tile.
5. **Unchanged:** document channel stays one-way (tablet authors in-session; desktop is the file). Reversing document direction / CRDT / multi-writer is an explicit deferral. Infini-follow-Infini is forward, not a Must this campaign.

## Asks

1. Reconcile module Product Requirements Documents to the new BRD-07 (do not keep “Infini owns pan/zoom” or “0 pans” on one-finger empty canvas as the business rule):
   - Epaper [REQ-02](../../../.docs/modules/epaper/prd.md#region-sync) Drawing-region mapping from Infini — today Infini owns pan/zoom and Epaper only consumes viewport.
   - Epaper [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) Hand-touch on canvas — two-finger still blocked on the old BRD deferral; one-finger empty canvas currently accepts 0 pans; two-finger still assumes a shared viewport Infini follows.
   - Infini [REQ-01](../../../.docs/modules/infini/prd.md#infinity-canvas) Infinity canvas navigation — Infini still pans/zooms **its own** canvas; drop exclusive ownership of the tablet camera; add Infini follow-Epaper toggle.
   - Infini [REQ-03](../../../.docs/modules/infini/prd.md#tablet-sync) Tablet session — one-way sync contract — viewport down as always-on drive is obsolete; document channel stays one-way; viewport follow is optional and directional.
2. Specify the **new follow UI on both** products (viewport-follow icon toggle; not a ToolChip exclusive hand-tool tile). Palm-vs-pan threshold for one-finger empty-canvas pan is PM detail.
3. After PRDs match BRD-07, ask Architect to **supersede** [ADR-0023](../../../.docs/adr/ADR-0023-viewport-last-writer.md) Viewport last-writer token. That ADR is **not** the business model (shared last-writer camera). **Do not** ask Architect to implement last-writer.

## Constraints

- Execution lock: vertical; stop_line verified; modules epaper + infini; do not open REQ-15 tables, REQ-08, CHL-0011, CHL-0012, EP-032, or AI.
- Do not open CRDT / multi-writer document this campaign.

## Out of scope

- Document-channel reversal (explicit BRD-07 deferral).
- Infini following other Infini instances (forward, not Must).
- Implementation, stories, design packages, ADR text (Architect after PRD).
