---
id: CHG-0002
title: Nest Swift package under macOS/; promote epaper toolchain helpers
date: 2026-08-09
iter: iter-001
---

# CHG-0002 — Dual roots layout

## Layout

```
macOS/     Package.swift, Sources, Tests, Config, scripts
epaper/    Qt app + docker/ + protocol/ + deploy script
```

## Helpers decision

- **In git:** `epaper/docker/` (no SDK installer blob), `epaper/protocol/`, deploy script, [TOOLCHAIN.md](../../../epaper/TOOLCHAIN.md) restore guide.
- **Out of git:** `rM2-stuff`, `rm-fb-probe`, spike `macos-canvas`, SDK `.sh` installer — see TOOLCHAIN.md.

## Path updates

- Swift commands run from `macOS/`
- Active docs / README point at `macOS/Sources/...`
