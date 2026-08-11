---
id: STORY-EP-003
title: "Design Epaper three-tool strip"
kind: design
parent_srs: [SRS-EP-05]
parent_req: [REQ-03]
status: done
priority: P0
iter: iter-003
estimate: 3
owner: designer
depends_on: [STORY-EP-004]
acceptance_criteria:
  - "Given SRS-EP-05 and spike note in iter memory, When the package ships, Then Selection · Pen · Ink-box strip is designed for the tablet form factor."
  - "Given touch not yet confirmed on device, When design proceeds, Then the Spec documents the pen-on-strip (or hardware) fallback without inventing a second product."
  - "Given ui-spec-gate, When run, Then hi-fi scenes + Spec pass for the armed/idle tool states."
design_package: ".plan/iter-003/design/epaper-tool-strip/"
ui_spec: ".plan/iter-003/design/epaper-tool-strip/ui-spec.md"
scenes:
  - ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-pen.html"
  - ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-ink-box.html"
  - ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-selection-idle.html"
  - ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-selection-selected.html"
  - ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-selection-dragging.html"
  - ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-selection-empty.html"
  - ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-session-down.html"
  - ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-touch-unavailable.html"
  - ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-orient-gut-on-top.html"
  - ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-states.html"
hifi: ".plan/iter-003/design/epaper-tool-strip/epaper-tool-strip-pen.html"
wireframe: ""
---

# STORY-EP-003 — Design Epaper three-tool strip

**Done (v0.3).** Floating **32px** icon chip at orientation-top; tablet landscape RM2 preview — [UI-EP-01](../design/epaper-tool-strip/ui-spec.md).
Human override vs full-band SRS: [CHL-0003](../challenges/CHL-0003-epaper-floating-toolchip.md).
