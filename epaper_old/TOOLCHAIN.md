# Epaper toolchain & what not to commit

## Keep in git (build helpers)

| Path | Why |
|---|---|
| `docker/` | amd64 container + `install-sdk.sh` to cross-compile on Apple Silicon |
| `docker/sdk-installer/.gitkeep` | Drop zone for the SDK `.sh` installer (**installer itself is gitignored**) |
| `protocol/` | Draft RM↔macOS sync messages (future S2/S3) |
| `scripts/deploy-rm2.sh` | scp + stop xochitl + launch |

## Do **not** promote (failed / one-shot probes)

Documented in [EXP-0001](../.plan/iter-001/explorations/EXP-0001-remarkable-canvas-sync.md) Rounds 8–11. Safe to delete with the sandbox worktree.

| Artifact                      | Why skip                                                                                                                  | How to restore if needed                                                                                                       |
| ----------------------------- | ------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| `rM2-stuff` / `rm2fb`         | Built against older `libqsgepaper`; failed address lookup on OS `3.28` / Codex `5.8.197`. Local ink works **without** it. | `curl -L https://github.com/timower/rM2-stuff/archive/refs/heads/master.tar.gz` (or pin a tag); rebuild only if porting SWTCON |
| `rm-fb-probe`                 | Direct `/dev/fb0` + `MXCFB_SEND_UPDATE` returned `EINVAL` on this firmware                                                | Recreate a 50-line C probe writing `/dev/fb0` if re-testing framebuffer assumptions                                            |
| Spike `macos-canvas/`         | Separate Swift viewport mock for EXP; not part of Reawa yet                                                               | Re-implement from EXP notes + `protocol/viewport-sync.md` when promoting S2/S3                                                 |
| SDK installer `.sh` (~400 MB) | Redistributable from reMarkable; too large for git                                                                        | See below                                                                                                                      |

## Restore the SDK installer

1. Note device OS (`cat /etc/os-release` / version on RM2). EXP used `3.28.0.157` with toolchain `remarkable-production-image-5.7.119-rm2-public-x86_64-toolchain.sh` from Codex `3.27` bucket (closest public match).
2. Download from [reMarkable developer links](https://developer.remarkable.com/documentation/links) (rm2 public toolchain).
3. Place the `.sh` in `epaper/docker/sdk-installer/`.
4. Build and enter the container:

```bash
cd epaper/docker
docker compose build
docker compose run --rm rm-sdk bash -lc 'install-sdk.sh && source /opt/remarkable-sdk/environment-setup-cortexa7hf-neon-remarkable-linux-gnueabi && cmake -B build -G Ninja && cmake --build build'
```

5. Deploy: `../scripts/deploy-rm2.sh`

## Sandbox recovery

`.sandbox/` is gitignored. If the local worktree is gone, **canonical product code is this `epaper/` tree** plus EXP-0001 for narrative. Do not treat the sandbox as the source of truth.
