---
feature: device-discovery
parent_req: [REQ-05]
version: 1.0.0
lifecycle: active
---

# SRS — Device Discovery (Logic)

## [SRS-RW-20] Network interface scan

`NetworkDiscovery` parses `/sbin/ifconfig` for `en*` interfaces. Skips `lo0`, `utun*`, `bridge0`, etc. Derives subnets from interface addresses.

**Link-local optimization:** For subnets wider than `/24` (e.g. `169.254.0.0/16`), candidate IP generation prioritizes the `/24` neighborhood around the interface's current address — probes near active address, not subnet start.

## [SRS-RW-21] SSH host probing

- Concurrent worker pool: max 64 hosts, 0.35 s timeout per host, port 22.
- Gateway (`.1`) probed first.
- `discoverUSBSSHHosts()` prefers non-primary `en*` interfaces for USB Ethernet.

## [SRS-RW-22] USBWatcher poll loop

Background poll ~every 3 s:

1. Scan USB Ethernet subnets for SSH hosts.
2. Update `discoveredIPs` and reachability state.
3. Auto-connect or notify per connection profile.
4. Disconnect active session when device goes offline.

Implementation uses main-runloop `Timer` + detached tasks for expensive work (avoids dispatch queue assertion crashes from custom queue timers).

## [SRS-RW-23] Auto-connect and notification matrix

| Situation | Behavior |
|---|---|
| Saved connection, auto-connect **on**, device found | Connect automatically + notification |
| Saved connection, auto-connect **off**, device found | Notification prompting manual connect |
| No saved connection, device found | Notification with discovered IP(s) |
| Device disconnects while active | Session stops + notification |

Notifications only in bundled `.app` runs ([SRS-RW-04](../menu-bar-shell/srs-logic.md)).

## [SRS-RW-24] Environment dependency (documented)

Reaching `10.11.99.1` requires macOS to route USB interface correctly. If OS routes tablet IP via Wi-Fi instead of USB (`en*`), connection fails despite physical USB link — not compensable in app code. See [swift-porting.md](../../../../memory/swift-porting.md).

---

## Superseded

_None yet._
