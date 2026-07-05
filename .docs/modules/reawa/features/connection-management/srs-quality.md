---
feature: connection-management
parent_req: [REQ-02]
version: 1.0.0
lifecycle: active
---

# SRS — Connection Management (Quality)

## Prioritised quality goals

1. **Data compatibility** — Legacy Python store migrates without user action
2. **Status freshness** — Connection status reflects reachability within poll interval

## [SRS-RW-18] Legacy store migration

| Field | Value |
|---|---|
| Source | User upgrading from Python `remarkable-rm2` app |
| Stimulus | First launch of Swift app; Reawa App Support dir absent |
| Artifact | ConnectionStore + KeychainStore |
| Environment | Existing legacy connections.json and keys |
| Response | Connections and keys copied; passwords readable from either Keychain service |
| Response measure | 100% of legacy connections visible after first launch (integration test / manual matrix) |

Constrains: [SRS-RW-08](srs-logic.md#storage).

## [SRS-RW-19] Status poll interval

| Field | Value |
|---|---|
| Source | USBWatcher / reachability timer |
| Stimulus | Device plugged or unplugged |
| Artifact | Connection status in menu |
| Environment | Normal |
| Response | Status transitions within one poll cycle |
| Response measure | ≤ 3.5 s from state change to UI update (3 s poll + refresh) |

Constrains: [SRS-RW-13](srs-logic.md).

---

## Superseded

_None yet._
