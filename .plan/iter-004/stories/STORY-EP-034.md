---
id: STORY-EP-034
title: USB Ethernet stay-up and SSH/TCP keepalives
kind: implement
parent_srs: [SRS-EP-08, SRS-EP-13]
parent_req: [REQ-07]
status: ready
priority: P1
iter: iter-004
estimate: 3
owner: dev
depends_on: []
acceptance_criteria:
  - "Given ping 10.11.99.1 fails or en* is 169.254 while the cable is still plugged, When the operator diagnoses, Then the story notes distinguish USB-gadget/L3 death from SSH idle (ping-alive vs ping-dead)."
  - "Given deploy-rm2.sh and any long-lived Mac→RM2 ssh, When the session is idle, Then client keepalives are set (ServerAliveInterval=15, ServerAliveCountMax=4, TCPKeepAlive=yes) so a dead path is detected without requiring a USB unplug to notice."
  - "Given StrokeSync and debug-log TCP to RM_SYNC_HOST, When the sockets are open, Then TCP keepalive is on both ends (device QTcpSocket KeepAliveOption; Infini listen sockets) and the existing 2s retry still reconnects after a genuine drop (SRS-EP-08)."
  - "Given the USB gadget is the failure (ping dead), When documenting the device-side stay-up, Then deploy notes or a one-shot remote snippet cover usb0 autosuspend/sleep — SSH keepalives alone must not be treated as the fix."
  - "Given TRACK-004 NOW work, When this ships, Then no tabletcanvasitem.cpp / recognizer edits (REQ-01 latency unchanged)."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-034 — USB Ethernet stay-up and SSH/TCP keepalives

**Bug / ops.** Human 2026-08-15: RM2 frequently **unreachable**; only USB disconnect/reconnect restores `10.11.99.1`. Suspected SSH idle; diagnosis: **USB Ethernet gadget / Mac USB NIC dying** is the likely class — SSH idle would leave ping working.

Parent: [SRS-EP-08](../../../.docs/modules/epaper/features/device-document/srs-logic.md) · [SRS-EP-13](../../../.docs/modules/epaper/features/device-document/srs-quality.md) · [REQ-07](../../../.docs/modules/epaper/prd.md#one-way-sync) (link-down is a normal *TCP* state; L2 gadget death is not). Related (out of lock): [SRS-RW-10](../../../.docs/modules/reawa/features/connection-management/srs-logic.md), [SRS-RW-24](../../../.docs/modules/reawa/features/device-discovery/srs-logic.md).

No design story — transport, not UI.

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Priority | **P1** — queued on TRACK-004; **do not steal EP-030 cursor** |

## Diagnosis (do not skip)

When it happens, **before unplugging**:

1. `ifconfig` on the USB `en*` — still `10.11.99.12`, or `169.254` / down?
2. `route -n get 10.11.99.1` — USB `en*`, not Wi-Fi.
3. `ping -c 1 10.11.99.1` vs `ssh`.

| Result | Treat as |
|---|---|
| USB iface down / `169.254` / ping fail | USB gadget or Mac USB power — keepalives will not revive it |
| Ping OK, SSH hangs/refused | dropbear / leftover SSH children / probe leak |
| Ping OK, SSH OK, Infini “RM disconnected” | StrokeSync TCP only |

USBWatcher TCP-probes `:22` every 3s (Reawa). An aborted handshake can exhaust dropbear while ping still works — in-lock this story does **not** change Reawa; note it if ping-alive/ssh-dead.

## Fix intent (do not expand)

- `epaper/scripts/deploy-rm2.sh` `ssh_rm` / `scp`: `ServerAliveInterval=15`, `ServerAliveCountMax=4`, `TCPKeepAlive=yes`.
- `epaper/strokesync.cpp` + `epaper/debuglog/debug_log_ship.cpp`: `KeepAliveOption`.
- Infini `:9877` / `:9878`: socket keepalive (no protocol change).
- Device: document or apply usb0 autosuspend off / no sleep while tethered (snippet in deploy comments or a `--usb-stay` one-shot). Do not rewrite xochitl.
- `@fix [STORY-EP-034]` at keepalive sites.

## Out of scope

Reawa `SSHSession.swift` / USBWatcher (lock: `reawa/*` backlog). Coordinate map, EP-030 recognition, EP-033 origin guard, REQ-08.

## Conflicts

None with EP-030 (`tabletcanvasitem.cpp`). Safe to land after EP-030 without a freeze.
