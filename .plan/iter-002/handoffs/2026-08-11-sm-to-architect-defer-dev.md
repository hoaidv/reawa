---
from: sm
to: architect
iter: iter-002
date: 2026-08-11
subject: no-design-defer-dev-sync
---

# Handoff: SM → Architect — vector-document (no design / no dev yet)

## Human decisions

1. **STORY-IN-006 needs no design** → story **blocked** (cancelled). Do not run `/designer`.
2. **No `/dev` yet** — implement document chrome / tree / Smart Group when building
   **epaper ↔ desktop sync** (W4 with tablet-sync / region-sync).
3. **Cursor → `/architect`** for sync readiness of the document model.

## Already in `.docs/` (consume, do not re-litigate unless CHL)

| Topic | Pointer |
|---|---|
| Tree-of-vectors | [ADR-0010](../../../.docs/adr/ADR-0010-tree-of-vectors.md) · SRS-IN-04/09 |
| Smart Group | [ADR-0011](../../../.docs/adr/ADR-0011-smart-group.md) · SRS-IN-10 · REQ-04 |
| Boundary ink always transforms; `inkScaleMode` = content only | ADR-0011 + product BR-09 |
| Session channels | [ADR-0009](../../../.docs/adr/ADR-0009-shared-document-viewport.md) |

## Ask of Architect

1. Review vector-document ops/tree/Smart Group against **tablet-sync** / **region-sync** SRS —
   bind how `append_ink`, `create_smart_group`, enclose recognition, and transmit ops ride the
   document channel; note gaps as CHL or thicken `infini/tablet-sync` + `epaper/region-sync`.
2. Confirm implement stories should wait for W4 (do not ask SM to `ready` Dev stories now).
3. Handoff back → `/sm` when sync-facing SRS is READY (or READY-WITH-CONCERNS).

## Board

Wave **W3-arch** — [execution-board](../execution-board.md). TRACK-002 cursor updated.

## Next

**`/architect`** — then `/sm` to open W4 implement slices when you say sync-ready.
