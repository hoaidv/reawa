#!/usr/bin/env bash
# EXP-0002 round 5 — Local vs Always-cubic vs Morph (SPIKE, host-only, no Qt).
# Throwaway sandbox harness; discarded with the exploration worktree.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
REPO="$(cd "$ROOT/.." && pwd)"
OUT="${1:-$REPO/out}"
cd "$ROOT"
c++ -std=c++17 -Wall -Wextra -Wpedantic -Wshadow -O2 -I. tests/warp_probe.cpp -o /tmp/warp_probe
rm -rf "$OUT"
mkdir -p "$OUT"
cp tests/CANONICAL-ALGORITHM.md "$OUT/CANONICAL-ALGORITHM.md"
/tmp/warp_probe "$OUT" | tee "$OUT/report.txt"
if command -v qlmanage >/dev/null 2>&1; then
    mkdir -p "$OUT/png"
    qlmanage -t -s 1600 -o "$OUT/png" "$OUT"/*.svg >/dev/null 2>&1 || true
fi
echo "contact sheet: $OUT/index.html"
