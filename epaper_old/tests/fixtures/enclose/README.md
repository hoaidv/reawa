# Shared enclose fixtures (SRS-EP-10 / SRS-EP-14 / SRS-IN-09)

Identical JSON for **device** and **desktop** guard evaluation.

Canonical copy: `.docs/modules/infini/features/vector-document/fixtures/enclose/`

Copies: `epaper/tests/fixtures/enclose/` and `infini/tests/fixtures/enclose/`

Each file:

| Field | Meaning |
|---|---|
| `seedOps` | Ops applied before the stroke (`applyOp`, not recognition) |
| `stroke.armed` | `ink_box` or `pen` (device latch; Infini maps to `intent`) |
| `stroke.points` | World samples of the finished stroke |
| `expected.verdict` | `created` \| `ordinary_ink` \| `skipped` |
| `expected.fittedBounds` | AABB of the enclose stroke samples (world) |

Device and Infini must agree 100% on verdict + fitted bounds. Divergence is a `CHL-*`.
