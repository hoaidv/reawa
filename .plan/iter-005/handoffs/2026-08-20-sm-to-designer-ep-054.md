---
from: sm
to: designer
date: 2026-08-20
iter: iter-005
cc: [pm, qa]
---

# Hand-off: Scrum Master → Product Designer — STORY-EP-054

Pick up [STORY-EP-054](../stories/STORY-EP-054.md) (Revise hand-touch: palm-rest vs empty local pan). Same package [hand-touch/](../design/hand-touch/) ([UI-EP-06](../design/hand-touch/ui-spec.md)).

Product [REQ-10](../../../.docs/modules/epaper/prd.md#hand-touch) (2026-08-20): Euclidean panel travel **≤ 10 mm** = palm-rest / tap (0 pan, 0 selection, 0 tool switch); **> 10 mm** = local one-finger pan. Box / knob / chip hit wins. Infini camera unchanged unless Infini follow is on.

Do **not** edit `pen-button-map/` or viewport-follow packages. Do **not** add follow-toggle buttons. Do not invent new hand-touch chrome.

Write-back: story `done` + `ui-spec-gate`; parent stitches `.docs/design/index.md` if needed.
