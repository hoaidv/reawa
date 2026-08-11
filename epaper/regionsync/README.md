# Epaper region sync (ADR-0009)

Header-only C++17 core for [SRS-EP-02](../../.docs/modules/epaper/features/region-sync/srs-logic.md)
/ [SRS-EP-03](../../.docs/modules/epaper/features/region-sync/srs-quality.md).

| File | Role |
|---|---|
| `viewport_map.hpp` | Infini viewport → input/world map |
| `doc_store.hpp` | Idempotent `append_ink` / remote ops |
| `region_session.hpp` | Pen hot path, net queue, coherent refresh |

**Host tests (no Qt):**

```bash
./tests/run_regionsync_test.sh
```

Device wiring: construct `RegionSession` beside `StrokeSync`; when ADR-0009 session is active,
`StrokeSync` / `RM_SYNC_HOST` must not own the document channel (see SRS-EP-02).
