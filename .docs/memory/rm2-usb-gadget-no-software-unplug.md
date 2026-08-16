---
title: RM2 USB gadget — no software unplug / UDC rebind
date: 2026-08-16
source: .plan/iter-004/stories/STORY-EP-036.md
---

# RM2 USB gadget — do not software-unplug

Writing `""` then a UDC name to `/sys/kernel/config/usb_gadget/*/UDC`, unbinding the UDC driver, or `modprobe -r g_ether` can **brick the USB Ethernet port until a tablet reboot**. Physical unplug/plug does not recover it. Starting xochitl does not recover it.

Safe work: read sysfs / `/proc/net/tcp` for HUD; `usb0` IFF_UP / `10.11.99.1` ioctl **without** a UDC pull; Infini TCP retry when the tablet reports Plugged.

STORY-EP-036 (gadget restore without unplug) is **cancelled**. Host-side USB re-enumeration without cable motion is a Linux gadget/host inspect, not a product slice.

See `.cursor/rules/rm2-no-usb-software-unplug.mdc`.
