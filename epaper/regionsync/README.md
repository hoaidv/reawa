# Epaper region sync (ADR-0009)

C++17 core for [SRS-EP-02](../../.docs/modules/epaper/features/region-sync/srs-logic.md)
/ [SRS-EP-03](../../.docs/modules/epaper/features/region-sync/srs-quality.md).
Map/session headers are host-tested without Qt; `strokesync` is the device TCP session.

| File | Role |
|---|---|
| `viewport_map.hpp` | Infini viewport → input/world map |
| `doc_store.hpp` | Idempotent `append_ink` / remote ops |
| `region_session.hpp` | Pen hot path, net queue, coherent refresh |
| `strokesync.{h,cpp}` | Device TCP session to Infini (`RM_SYNC_HOST`) |

**Host tests (no Qt):**

```bash
./tests/run_regionsync_test.sh
```

Device wiring: construct `RegionSession` beside `StrokeSync`; when ADR-0009 session is active
(`ownsDrawingRegionMap()`), `StrokeSync` / `RM_SYNC_HOST` must not own the drawing-region map
(see SRS-EP-02). Panel size via `setPanelSize`; refresh via `runRegionRefresh(nowMs, forceSettle)`.
