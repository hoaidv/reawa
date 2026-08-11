---
from: architect
to: sm
iter: iter-002
date: 2026-08-11
subject: vector-document-tree-ready
verdict: READY
---

# Handoff: Architect → SM — vector-document architecture READY

## Verdict: **READY**

Tree-of-vectors specified; no blocking open questions for first design paint.
SVG attribute grammar may tighten later without changing ADR-0010.

## Artifacts

| Artifact | ID / path |
|---|---|
| ADR | [ADR-0010](../../../.docs/adr/ADR-0010-tree-of-vectors.md) **accepted** |
| Logic | [SRS-IN-04](../../../.docs/modules/infini/features/vector-document/srs-logic.md) tree + 3 representations |
| Data | [SRS-IN-09](../../../.docs/modules/infini/features/vector-document/srs-data.md) JSON + SVG map + ops |
| Quality | [SRS-IN-06](../../../.docs/modules/infini/features/vector-document/srs-quality.md) fidelity + dual-ask |
| Module arch | [architecture.md](../../../.docs/modules/infini/architecture.md) entities updated |

## Model (summary)

```
Document.rootChildren → Frame* | Group | Ink | Text | Primitive | Connector
Group.children → (same except Frame)
Ink = dense polyline (± pressure)
Connector → anchors on two node ids
```

Live SoT remains op-log ([ADR-0009](../../../.docs/adr/ADR-0009-shared-document-viewport.md));
SVG is persistence of the materialised tree.

## Findings / concerns

- Infini canvas still flattens a list today — implement stories must add tree + `flattenDrawables`.
- Nested frame local transforms deferred (world-space children in v0).
- Orphan SRS until implement stories exist — expected.

## Ask of SM

1. Keep STORY-IN-006 **ready** → `/designer` (SRS now thick).
2. After design `done`, slice implement stories: tree model, SVG I/O, chrome wire-up, fixtures
   (depends_on IN-006 for UI chrome).
3. Do not start tablet-sync feature until F2 wave clears (wip 1).

## Next

**`/designer`** on STORY-IN-006 (or `/sm` only if replan slices now).
