---
feature: device-discovery
parent_req: [REQ-05]
version: 1.0.0
lifecycle: active
---

# SRS — Device Discovery (UI)

## [SRS-RW-25] Discovered devices list

In settings connection editor left pane:

- **Scan devices** button refreshes list.
- Shows IPs found on USB-tethered subnets.
- IPs already in saved connections are hidden from discovered list.
- Click discovered entry to pre-fill Name/IP in new-connection form.

## [SRS-RW-26] User notifications

Local notifications (when bundled + permission granted):

- Auto-connect success
- Device detected (manual connect prompt)
- Unknown device IP discovered
- Device disconnected during active session

Non-bundled runs: behavior logged to App Behavior Log instead.

---

## Superseded

_None yet._
