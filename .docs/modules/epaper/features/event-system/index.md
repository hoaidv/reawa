---
feature: event-system
module: epaper
lifecycle: active
owner: architect
kind: implementation-view
---

# Feature — Event system (pen, finger, chrome arbitration)

How a contact on the glass becomes a document gesture. This folder is an **implementation view**: it
defines no requirement ids and owns no product behaviour. The behaviour lives in
[SRS-EP-04](../tool-modes/srs-logic.md) (tool state and input routing),
[SRS-EP-21](../ink-box/srs-logic.md#srs-ep-21-one-finger) (one-finger pick, move, pan) and
[SRS-EP-24](../region-sync/srs-logic.md#srs-ep-24-two-finger-viewport) (two-finger viewport); this
folder records *how the routing that satisfies them is wired, and why each knob is set*.

- [event-flow.md](./event-flow.md) — the stack, component roles, pen and finger sequences, every
  tuning knob with the bug it bought, the document gestures at the end of the flow, and the history
  of gotchas

Two standing rules for this area:

1. **Qt routes events; we hit-test document geometry only.** A rect containment check against a
   button is a defect.
2. **The raw input filter publishes facts; it never issues commands.** `QtInputFilter` exists for
   the three things Qt handlers cannot do — pen coordinate mapping, rescuing tilt/rotation channels,
   and counting live contacts — and it holds no reference to the canvas. What those facts *mean* is
   decided in `Main.qml`, beside the handlers.
