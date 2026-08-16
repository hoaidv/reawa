---
slug: usb-gadget-no-software-unplug
iter: iter-004
date: 2026-08-16
scope: project
promoted_to: .docs/memory/rm2-usb-gadget-no-software-unplug.md
---

# USB gadget software unplug bricks RM2

UDC cycle / `g_ether` unload left the Mac unable to ping `10.11.99.1` until tablet reboot. Human cancelled STORY-EP-036. Promote: do not schedule gadget re-enum as a story without a dedicated Linux inspect.
