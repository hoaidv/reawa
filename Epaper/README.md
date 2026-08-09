# Epaper

Qt Quick + C++ app that runs **on the reMarkable 2**: local pen ink on the
e-paper panel with pen-matched coordinates (landscape use vs portrait panel).

This tree sits beside `Sources/` (Swift Reawa host app). It is **not** a SwiftPM
target — build with the reMarkable Qt SDK (x86_64 Linux / Docker on Apple Silicon).

Promoted from [EXP-0001](../.plan/iter-001/explorations/EXP-0001-remarkable-canvas-sync.md)
after local ink was verified on firmware `3.28.0.157`. Spec: [SRS-EP-01](../.docs/modules/epaper/features/local-pen-ink/srs-logic.md).

## Working recipe

- `QT_QPA_PLATFORM=epaper`, `QT_QUICK_BACKEND=epaper`, `evdevtablet`
- Ink as a pre-created QML `Rectangle` pool moved into place (no per-point full-window repaint)
- Input via a **visible but transparent** `QQuickPaintedItem` (zero-size if `visible: false`)
- Coordinate transform (landscape tablet, portrait panel):

  ```
  renderX = penY · (w/h)
  renderY = h − penX · (h/w)
  ```

## Build (SDK container)

```bash
source /opt/remarkable-sdk/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi
cmake -B build -G Ninja
cmake --build build
```

Output: `build/epaper` (ARM 32-bit ELF).

## Deploy

```bash
./scripts/deploy-rm2.sh
```

Stops `xochitl`, scp’s the binary to `root@10.11.99.1`, launches with epaper QPA.
Restore UI with: `ssh root@10.11.99.1 'killall epaper; systemctl start xochitl'`.
