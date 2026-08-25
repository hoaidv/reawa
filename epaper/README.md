# epaper

Qt Quick + C++ app that runs **on the reMarkable 2**: local pen ink on the
e-paper panel with pen-matched coordinates (landscape use vs portrait panel).

This tree sits beside `macOS/` (Swift Reawa host app). It is **not** a SwiftPM
target — build with the reMarkable Qt SDK (x86_64 Linux / Docker on Apple Silicon).

Promoted from [EXP-0001](../.plan/iter-001/explorations/EXP-0001-remarkable-canvas-sync.md).
Spec: [SRS-EP-01](../.docs/modules/epaper/features/local-pen-ink/srs-logic.md).
Latency / SWTCON deep dive: [RENDERING.md](RENDERING.md).

## Working recipe

- `QT_QPA_PLATFORM=epaper`, `QT_QUICK_BACKEND=epaper`, `evdevtablet`
- Single `QQuickPaintedItem` + persistent `QImage`; `update(bbox)` only
- `EPScreenModeItem(Mode::Pen)` + `EPFramebuffer::swapBuffers` via runtime `dlsym`
- Time-based flush (~8 ms), not a distance gate
- Coordinate transform (landscape tablet, portrait panel):

  ```
  renderX = penY · (w/h)
  renderY = h − penX · (h/w)
  ```

## Build & deploy

See [TOOLCHAIN.md](TOOLCHAIN.md) for SDK installer setup.

```bash
./scripts/build.sh
./scripts/deploy-rm2.sh
./scripts/deploy-rm2.sh --build     # build then deploy
./scripts/deploy-rm2.sh --restore   # kill epaper, start xochitl
```

Deploy picks a working key under `~/Library/Application Support/Reawa/keys/*/id_rsa`
(or `RM_SSH_KEY`). It **appends** this Mac's `.pub` to the tablet `authorized_keys`
and never replaces that file, so other machines keep access ([SRS-RW-10]).
Host keys go in `~/.ssh/reawa_rm_known_hosts` (or `RM_SSH_KNOWN_HOSTS`) so SSH does not
re-learn `10.11.99.1` on every hop. After a tablet reflash, remove that entry and redeploy.

Optional env, forwarded to the device by `deploy-rm2.sh` when set locally:

| Env | Effect |
|---|---|
| `RM_INK_TRACE=1` | Dump arrival→flush / flush→swap percentiles on exit |
| `RM_INK_MODE=pool` | Fall back to the old QML `Rectangle` ink pool |
| `RM_INK_BEACON=1` | Stamp render-path probe squares into the ink layer |
| `RM_DOC_PROBE=1` | STORY-EP-013: resident 500-node / 50k-sample stub + hit-test on ingest (not in `paint()`) |
| `RM_DOC_PROBE_SYNTH=1` | Inject synthetic strokes then exit (device-runnable; still not panel p95) |
| `RM_DOC_PROBE_EVERY_SAMPLE=1` | Hit-test on every ingest sample (stress); default is pen-down only |
| `RM_SYNC_HOST=<mac-ip>` | Enable deferred stroke TCP to macOS `:9877` |
| `RM_EP_SWAP=1` | Also call `swapBuffers(Pen)` after each flush (experimental) |
| `RM_EP_SCREEN_MODE=<int>` | Override Pen-mode enum value |
| `RM_EP_CONTENT_TYPE=<int>` | Override content type (4-arg swapBuffers only) |

Output binary: `build/bin/epaper`.
