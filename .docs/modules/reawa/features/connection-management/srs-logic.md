---
feature: connection-management
parent_req: [REQ-02, REQ-06]
version: 1.0.0
lifecycle: active
---

# SRS — Connection Management (Logic)

## [SRS-RW-08] Connection data model {#storage}

```swift
struct Connection: Codable, Identifiable, Equatable, Sendable {
    var id: String
    var name: String
    var ip: String
    var autoConnect: Bool
    var deviceConfig: DeviceConfig
}
```

Persisted at `~/Library/Application Support/Reawa/connections.json`. SSH keys at `keys/<connection-id>/id_rsa` (+ `.pub`).

**Legacy migration:** On first run, if Reawa store absent, copy from `~/Library/Application Support/remarkable-rm2/` (connections.json + key dirs). `KeychainStore` reads passwords from service `Reawa` with fallback `remarkable-rm2`.

## [SRS-RW-09] DeviceConfig model

| Field | Type | Notes |
|---|---|---|
| `outputMode` | `OutputMode` | `.relative`, `.absolute`, `.nativeStylus` — JSON: `RELATIVE`, `ABSOLUTE`, `NATIVE_STYLUS` |
| `scale` | `Double?` | Points per digitizer unit; `nil` = auto from display PPI |
| `swapXY`, `invertX`, `invertY` | `Bool` | Axis transforms (UI exposes as tablet orientation) |
| `absolute` | `AbsoluteConfig` | Region geometry + snap metadata |

### AbsoluteConfig (Quartz coordinates)

| Field | Purpose |
|---|---|
| `regionX`, `regionY`, `regionWidth`, `regionHeight` | Snapped mapping rect (RM2 aspect via `lockAspect()`) |
| `borderColor`, `borderStyle` | Overlay border (`solid` / `dashed`; style not yet in UI) |
| `snapWindowEnabled`, `snappedWindowRef` | Window binding metadata |

JSON uses snake_case keys: `region_x`, `snapped_window_ref`, etc. RM2 aspect: `20967/15725`.

## [SRS-RW-10] SSH authentication and key setup

- Connect with RSA key; on auth failure, `setupKey(...)` with Keychain password.
- `ensureKeyPair(...)`: 3072-bit RSA via `/usr/bin/ssh-keygen`.
- `setupKey(...)`: temporary `SSH_ASKPASS` helper **appends** this machine's public key to device `authorized_keys` if the exact line is absent. It must not truncate, rewrite, or replace the file — several Macs may share one RM2.
- After first successful setup, subsequent connects use key only.
- USB Ethernet always uses `10.11.99.1`. Do not share one `known_hosts` across developer machines or tablet reinstalls — each client keeps its own store. Key auth is still required.
- Epaper deploy (`epaper/scripts/deploy-rm2.sh`): `StrictHostKeyChecking=accept-new` and `UserKnownHostsFile` → `RM_SSH_KNOWN_HOSTS` (default `~/.ssh/reawa_rm_known_hosts`), isolated from the global known_hosts file. After the tablet host key is learned once, OpenSSH must not re-print “Permanently added” on every hop. On host-key rotation (reflash), delete that file’s `10.11.99.1` line (or the file) and reconnect. Resolves `RM_SSH_KEY` or any `~/Library/Application Support/Reawa/keys/*/id_rsa`, then appends that key's `.pub` the same way (append-only `authorized_keys`).
- Reawa app clients that cannot yet use a dedicated known_hosts file may still use `StrictHostKeyChecking=no` with `UserKnownHostsFile=/dev/null` for non-interactive first connect.

## [SRS-RW-11] Single active connection

`ConnectionManager` maintains one active `DriverSession`. Connecting to B disconnects A. Published state: `connections`, `discoveredIPs`, `activeConnectionID`, `nativeStylusStatuses`.

## [SRS-RW-12] Live config cache

`ConnectionManager.updateConnection(...)` persists JSON, refreshes in-memory state, pushes config snapshot into active session. Session reads config on next pen frame — no per-frame disk I/O.

## [SRS-RW-13] Connection status derivation

| Status | Condition |
|---|---|
| `offline` | Device IP not reachable |
| `online` | IP reachable, no active pen stream |
| `connected` | Active DriverSession with open SSH stream |
| `error` | Last connect attempt failed |

Updated ~every 3 s by reachability polling. Error persists until retry or device offline.

## [SRS-RW-14] Keychain password storage

Passwords keyed by connection ID. Used only for first-time public-key installation when local private key does not exist.

---

## Superseded

_None yet._
