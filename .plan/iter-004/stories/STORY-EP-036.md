---
id: STORY-EP-036
title: Detect USB gadget down and restore without unplug
kind: implement
parent_srs: [SRS-EP-08, SRS-EP-13]
parent_req: [REQ-07]
status: cancelled
priority: P1
iter: iter-004
estimate: 5
owner: dev
depends_on: [STORY-EP-034]
acceptance_criteria:
  - "Given USB is still plugged and ping 10.11.99.1 times out (xochitl Help would omit 10.11.99.1), When epaper diagnoses, Then it logs a gadget-down class distinct from StrokeSync TCP-down while ping works (STORY-EP-034 field note)."
  - "Given that gadget-down class, When epaper applies the in-process restore (usb0/g_ether bring-up, same class as xochitl advertising 10.11.99.1), Then the tablet address is reachable again without unplug-plug, or the log states restore failed (do not claim unplug is required until that log)."
  - "Given restore succeeds, When StrokeSync's existing 2s retry runs, Then [sync] connecting → [sync] connected to RM_SYNC_HOST:9877 without a USB replug (SRS-EP-08 reconnect)."
  - "Given ping already works and only Infini is disconnected, When this detector runs, Then it does not restart the USB gadget (that class stays EP-034 TCP keepalive)."
  - "Given TRACK-004 NOW work, When this ships, Then no tabletcanvasitem.cpp / recognizer edits."
design_package: ""
ui_spec: ""
scenes: []
hifi: ""
wireframe: ""
---

# STORY-EP-036 — Detect USB gadget down and restore without unplug

**Cancelled 2026-08-16 (human).** Restoring `10.11.99.1` without a physical unplug/plug needs a deep Linux USB gadget / host re-enumeration inspect. UDC unbind, `modprobe -r g_ether`, and software unplug **brick the port until tablet reboot** (see `.cursor/rules/rm2-no-usb-software-unplug.mdc`). Do not resurrect this story as gadget restore.

Keepalives and TCP retry that **did** ship stay under [STORY-EP-034](./STORY-EP-034.md). Physical cable unplug/plug remains the only safe re-enumeration.

**Original bug.** Human 2026-08-16: USB cable still plugged, **ping 10.11.99.1 times out**. Same class xochitl shows when Help omits `10.11.99.1` as the SSH address. Distinct from EP-034’s ping-alive Infini-down.

Wanted: **detect on epaper**, then **bring the gadget back without unplug**, then the existing StrokeSync session retry can connect again.

Parent: [SRS-EP-08](../../../.docs/modules/epaper/features/device-document/srs-logic.md) · [SRS-EP-13](../../../.docs/modules/epaper/features/device-document/srs-quality.md) · [REQ-07](../../../.docs/modules/epaper/prd.md#one-way-sync).

Depends on [STORY-EP-034](./STORY-EP-034.md) landing keepalives so the two classes stay separated.

No design story — transport / gadget, not ToolChip UI. Status line log is enough this slice (do not design a new Help page).

## Kind

| Field | Value |
|---|---|
| Kind | `implement` |
| Owner | `dev` |
| Priority | **P1** — **cancelled** 2026-08-16 — no restore without Linux USB inspect |
| Depends on | EP-034 |

## Diagnosis split

| Observation | Story |
|---|---|
| Ping OK, Infini disconnected | EP-034 — StrokeSync TCP |
| USB plugged, ping timeout / no `10.11.99.1` | **This story** — USB Ethernet gadget down |

Hypothesis (superseded): ifconfig usb0 + autosuspend is enough. **FAIL**.

UDC rebind on the **UI thread** + ICMP ping + `modprobe -r g_ether` **FAIL** (2026-08-16): Mac still cannot ping; ink froze; USB dead until tablet reboot. `g_ether` unload is forbidden. Restore is worker-thread UDC cycle only.

Must not block ink. Infini `:9877` refused must not bounce the gadget.

Must establish **USB gadget/hardware enumeration** first (host sees ethernet / `10.11.99.1`), **then** re-activate SSH over USB. StrokeSync retry is step 3.

## Fix intent (do not expand)

- Periodic UsbLink infra (worker): usb0 carrier/addr; restore UDC without touching ink or TCP apps.
- StrokeSync / debug-log only retry TCP (`kAppTcpRetryMs` = 5s).
- Best-effort restore without unplug; log success/fail.
- Do **not** bounce the gadget when the Mac USB peer still answers ping (Infini-only down stays EP-034).
- After restore, leave StrokeSync’s 2s `connectToMac` as the session restart.
- `@fix [STORY-EP-036]` at the detector.

## Out of scope

Reawa USBWatcher / `SSHSession.swift`. Infini demo figures (IN-032). Ingest/recognizer. Claiming gadget restore if the kernel cannot — then log fail, do not invent unplug as the product AC.
