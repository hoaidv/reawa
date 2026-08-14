---
title: RM_SYNC_HOST required on every epaper deploy
date: 2026-08-14
source: .plan/iter-003/handoffs/2026-08-14-sm-to-pm-w12-done.md
---

# RM_SYNC_HOST on deploy

Epaper StrokeSync is inert unless `RM_SYNC_HOST` is set in the process environment. `deploy-rm2.sh` only forwards that env when it is set **locally** at deploy time. A deploy with only `RM_SSH_KEY` leaves Infini on “RM disconnected” even when the tablet binary is running.

Use the **Mac USB Ethernet IP** (usually `10.11.99.12`), never the tablet `10.11.99.1`.

```bash
RM_SYNC_HOST=10.11.99.12 RM_SSH_KEY="$HOME/Library/Application Support/Reawa/keys/<id>/id_rsa" ./scripts/deploy-rm2.sh
```
