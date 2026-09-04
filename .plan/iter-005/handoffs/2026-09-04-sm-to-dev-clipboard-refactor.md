---
from: sm
to: dev
date: 2026-09-04
iter: iter-005
---

# Hand-off: Scrum Master → Developer (later)

## Context

Human verified [STORY-EP-044](../stories/STORY-EP-044.md) (In-document copy cut paste and
tap-origin paste) **complete** 2026-09-04. Do not start this until the human picks it.

## Ask

[STORY-EP-073](../stories/STORY-EP-073.md) (Split clipboard clipops into document helpers and
actions) — `draft` / P2 / later.

- Document query and mutation (no copy / cut / paste names) → `epaper/document`
- Copy / cut / paste orchestration → CopyAction / CutAction / PasteAction
- Slot stays in `clipboard.hpp`
- **Do not** add `copySelection` / `pasteClipboard` on `DocContext` ([ADR-0037](../../../.docs/adr/ADR-0037-device-clipboard-singleton.md))

## Out of scope

Field latency [STORY-EP-070](../stories/STORY-EP-070.md)…[STORY-EP-072](../stories/STORY-EP-072.md)
remain the NOW wave. Device Settings. Infini apply.
