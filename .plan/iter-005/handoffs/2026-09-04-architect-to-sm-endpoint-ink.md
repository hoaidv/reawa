---
from: architect
to: sm
date: 2026-09-04
iter: iter-005
---

# Hand-off: Architect → Scrum Master

## Context

[REQ-13](../../../.docs/modules/epaper/prd.md#connector-ends) Path B on Epaper. Product [SRS-EP-74](../../../.docs/modules/epaper/features/connector-ink/srs-product.md#srs-ep-74-endpoint-ink-product). Human 2026-09-04: store on `ConnectorAnchor`; append; **erase ticks and keep the connector**. Path A chips frozen ([STORY-EP-045](../stories/STORY-EP-045.md) / [STORY-EP-046](../stories/STORY-EP-046.md)).

## Bind

| Id | Title |
|---|---|
| [SRS-EP-74](../../../.docs/modules/epaper/features/connector-ink/srs-product.md#srs-ep-74-endpoint-ink-product) | Path B product (BR-E01…E09, tick-erase **in**) |
| [SRS-EP-35](../../../.docs/modules/epaper/features/connector-ink/srs-logic.md#srs-ep-35-endpoint-ink) | Steal 80% length / 5 mm world; `set_endpoint_ink`; face-frame list |
| [SRS-EP-37](../../../.docs/modules/epaper/features/connector-ink/srs-quality.md#srs-ep-37-endpoint-quality) | Face-aim, append+undo, tick-erase keep connector, 2% gate |
| [ADR-0038](../../../.docs/adr/ADR-0038-endpoint-ink-face-frame.md) | Face frame; supersedes [ADR-0026](../../../.docs/adr/ADR-0026-endpoint-ink-membership.md); amends [ADR-0022](../../../.docs/adr/ADR-0022-recognizer-dispatch.md) / [ADR-0034](../../../.docs/adr/ADR-0034-erase-clip-remnants.md) |

BDD: [endpoint-ink.feature](../../../.docs/modules/epaper/features/connector-ink/bdd/endpoint-ink.feature). Domain: `ConnectorAnchor.styleInk[]` `{n,e}` — not `terminal[end].ink` on rest spine.

## Review verdict

**READY**

| Class | Finding |
|---|---|
| Strength | Face frame keeps arrows aimed under rotate; steal is unique-end + length; tick erase keeps the spine; append stays a list |
| Concern | Short connectors have overlapping 5 mm end circles — mixed/two-end **refuse** (product-safe; may feel picky) |
| Concern | Infini apply of `set_endpoint_ink` is **not this track** (same posture as `create_connector` vs closed transmit) |
| Risk | none blocking — 5 mm / 80% retune is an SRS constant, not a new ADR |

SRS orphans until code exist are expected.

## Asks

1. Ready [STORY-EP-047](../stories/STORY-EP-047.md) (`depends_on: []`, no design). Human will QA on device.
2. Keep EP-045 / EP-046 **blocked**.
3. Do not start Path A, Infini Path A, or [STORY-EP-073](../stories/STORY-EP-073.md).

## Constraints

- Dispatch: enclose → membership → **endpoint-ink** → new connector → ink. Armed with `recog.connector` at pen-down.
- Radius is `eraseMmToWorld(5)`, not raw `5.0`.
- Rest spine never rebaked ([ADR-0020](../../../.docs/adr/ADR-0020-connector-ink-geometry.md) I1).
- False-positive ship gate includes unintended endpoint-ink binds.

## Out of scope

- Path A toolbar / closed glyphs on Epaper
- Mid-attachments [REQ-14](../../../.docs/modules/epaper/prd.md#connector-attachments)
- Design package `connector-ends/`
