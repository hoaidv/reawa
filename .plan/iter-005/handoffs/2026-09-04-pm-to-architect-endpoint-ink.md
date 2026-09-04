---
from: pm
to: architect
date: 2026-09-04
iter: iter-005
---

# Hand-off: Product Manager → Architect

## Context

Human started [REQ-13](../../../.docs/modules/epaper/prd.md#connector-ends) (Connector endpoint styles) for Epaper. Closed 2026-09-04:

- **Epaper = Path B** (drawn ink on the end). **Path A** (closed-style toolbar) = Infini / web-desktop, **not this lock**.
- Storage: decoration lives on **`ConnectorAnchor`** (face frame; Architect owns the field list — must remain a **list of strokes**).
- Second stroke on the same end: **append**. One undo peels the last append.
- Needs design: **no**. [STORY-EP-045](../stories/STORY-EP-045.md) / [STORY-EP-046](../stories/STORY-EP-046.md) **blocked** (frozen). [STORY-EP-047](../stories/STORY-EP-047.md) has no design `depends_on`.

**Product contract:** [SRS-EP-74](../../../.docs/modules/epaper/features/connector-ink/srs-product.md#srs-ep-74-endpoint-ink-product). PRD `0.16.0-draft`.

## Review verdict

**READY-WITH-CONCERNS**

| Class | Finding |
|---|---|
| Strength | Stylus-only Path B; no fifth tool; append stays flexible; Needs design no |
| Concern | Brush/object-erase of **ticks only** (connector stays) is **out** until human reopens the PRD open question. Object-erase of the connector still takes decoration |
| Gap | none blocking bind — steal 5 mm / 80% length and face-frame storage are in SRS-EP-74 |

`prd-check` run with this handoff.

## Asks

1. Amend [ADR-0026](../../../.docs/adr/ADR-0026-endpoint-ink-membership.md): steal = ≥80% **length** in a 5 mm world circle at one end; store polylines on `ConnectorAnchor` (face frame), **not** `(s, d)` on rest spine; append list; mixed/two-end refuse (nearest-unique optional). Accept the ADR.
2. Rewrite [SRS-EP-35](../../../.docs/modules/epaper/features/connector-ink/srs-logic.md#srs-ep-35-endpoint-ink) against SRS-EP-74. Keep the id.
3. Mark [SRS-EP-34](../../../.docs/modules/epaper/features/connector-ink/srs-logic.md#srs-ep-34-end-styles) / [SRS-EP-36](../../../.docs/modules/epaper/features/connector-ink/srs-ui.md#srs-ep-36-endpoint-toolbar) **not Epaper this campaign** (lifecycle / note). Do not delete ids.
4. Amend [SRS-EP-37](../../../.docs/modules/epaper/features/connector-ink/srs-quality.md#srs-ep-37-endpoint-quality): drop device Path A 300 ms toolbar row; keep steal / warp / 2% gate; add append+undo.
5. Domain `terminal[end].ink` vs `ConnectorAnchor` — pick one home; PM named ConnectorAnchor.
6. BDD for Path B before code. Then Scrum Master can ready [STORY-EP-047](../stories/STORY-EP-047.md).

## Constraints

- Vertical TRACK-005. Do not mix Device Settings, TRACK-006 reopen, [STORY-EP-073](../stories/STORY-EP-073.md).
- Do not implement Path A chips on the tablet.
- Do not start Infini Path A.
- Rest spine never rebaked ([ADR-0020](../../../.docs/adr/ADR-0020-connector-ink-geometry.md) I1).
- False-positive ship gate still includes unintended endpoint-ink binds.

## Out of scope

- Path A toolbar / closed glyphs on Epaper
- Mid-attachments [REQ-14](../../../.docs/modules/epaper/prd.md#connector-attachments)
- Brush-erase of decoration only (unless human reopens)
- Design package `connector-ends/`
