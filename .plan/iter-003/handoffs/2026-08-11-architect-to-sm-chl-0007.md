# Handoff — Architect → SM

**Date:** 2026-08-11  
**From:** architect  
**To:** sm  
**Re:** CHL-0007 — selection residue, move snap-back, consecutive enclose desync

## State

Human reported three verify bugs on TRACK-003 Smart Group. Diagnosed as SRS gaps + Epaper/Infini defects (not new product scope). **CHL-0007 adopted**; SRS amended (`SRS-EP-04` tool-modes, `SRS-EP-01` region-sync, `SRS-IN-13` tablet-sync). Hotfix already applied in tree (Epaper `tabletcanvasitem.*`, Infini `CanvasStage.tsx` / `native.d.ts`).

## Review verdict: READY-WITH-CONCERNS

| Class | Finding |
|---|---|
| Strength | Authority remains Infini; device optimistic only on move/resize commit; snapshots queued not dropped |
| Concern | Consecutive enclose still requires free root ink (≥80% inside); empty surrounds → ordinary_ink (by design). Human must verify with free strokes per box |
| Concern | Deploy/rebuild Epaper on device required before human re-verify; Infini HMR covers desktop side |
| Risk | None new if stuck-stroke + snapshot flush land on device |

## Ask SM

1. Schedule human re-verify checklist for bugs 1–3 after RM deploy.
2. Fold into EP-006 / IN-019 verify-fix wave (or split EP-012 / IN-027).
3. Do **not** reintroduce mid-drag full-vector / `swapPen` storms.

## Next

**Type `/sm`** to lock stories + verify wave.
