---
from: sm
to: human
date: 2026-08-20
iter: iter-005
cc: [qa, dev]
---

# Hand-off: Scrum Master → human — pause for Infini + Epaper field test

[STORY-IN-033](../stories/STORY-IN-033.md) (Infini applies tablet viewport only while following) is **done** (host tests). [TRACK-005](../../tracks/TRACK-005-hand-on-paper.md) (Hand-on-paper) is **paused**. Do not start W3 erase or Device Settings until you confirm the real-device run.

## Deploy

- Epaper: `epaper/scripts/deploy-rm2.sh --build` (see `epaper/README.md`).
- Infini: `cd infini && npm run electron:dev` (see `infini/README.md`). Session TCP `:9877`.

## Score on hardware (minimum)

1. Infini follow **on** → two-finger pan/pinch on tablet → Infini canvas matches after settle.
2. Infini follow **off** → tablet pans → Infini camera unchanged.
3. Infini following → local pan/pinch on Infini → follow off; Epaper camera unchanged.
4. Follow toggles still mutually exclusive; drop/reconnect leave follow **off**.

Host residual already known: no RM2 panel / no live TCP in the agent environment. This run is the real verification of p95 bars and two-app follow.

## Resume

Say go in `/sm` after the test (pass, fail, or notes). Scrum Master will not spawn erase design or Device Settings until then.
