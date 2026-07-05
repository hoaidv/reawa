---
feature: device-discovery
parent_req: [REQ-05]
version: 1.0.0
lifecycle: active
---

# SRS — Device Discovery (Quality)

## Prioritised quality goals

1. **Discovery coverage** — Find device on USB without hardcoded IP
2. **Link-local robustness** — Wide subnets still find peer near interface address

## [SRS-RW-27] USB subnet discovery

| Field | Value |
|---|---|
| Source | reMarkable plugged via USB Ethernet |
| Stimulus | USBWatcher poll |
| Artifact | discoveredIPs |
| Environment | Typical `10.11.99.1` or link-local `169.254.x.x` |
| Response | Device IP appears in discovered list |
| Response measure | Device found within 2 poll cycles (≤ 7 s) when OS presents usable USB network |

Constrains: [SRS-RW-20](srs-logic.md), [SRS-RW-22](srs-logic.md).

## [SRS-RW-28] Link-local candidate ordering

| Field | Value |
|---|---|
| Source | Interface on `169.254.x.x/16` |
| Stimulus | Discovery scan |
| Artifact | Candidate IP list |
| Environment | `/16` link-local USB |
| Response | Probes include neighbors of interface address, not only `169.254.0.x` |
| Response measure | Unit test: candidates within ±1 of interface `/24` prioritized (see `Tests/ReawaTests/`) |

Constrains: [SRS-RW-20](srs-logic.md).

---

## Superseded

_None yet._
