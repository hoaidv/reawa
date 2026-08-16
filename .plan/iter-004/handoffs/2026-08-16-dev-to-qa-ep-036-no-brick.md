---
from: dev
to: qa
date: 2026-08-16
iter: iter-004
---

# Hand-off: Dev → QA — EP-036 stop freezing/bricking USB

## Context

Last restore **broke the device**: UI-thread ping + `modprobe -r g_ether` froze ink and left USB dead until reboot.

This build:

- **Never** unloads `g_ether`.
- **Never** pings or `usleep`s on the ink/UI thread. UDC cycle runs on a worker.
- Does **not** bounce when Infini refuses `:9877` (USB still up).
- Bounces UDC only on no-carrier / no-addr, or two TCP path-dead errors.

Ink must stay live even if Mac ping stays dead. If ping still fails after UDC, unplug is still the control — that log is allowed.

## Asks

1. **Restart the tablet**, then deploy **this** epaper (not the previous binary).
2. Draw: must not freeze while Infini is down.
3. Lid: if ping returns without unplug, EP-36 pass; if not, FAIL but tablet must remain SSH-able.

## Constraints

No ingest edits. Bug 2 still blocked on ping.
