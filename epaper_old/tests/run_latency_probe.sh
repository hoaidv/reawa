#!/usr/bin/env bash
# Host harness for STORY-EP-013 / [SRS-EP-13] (no Qt, not device p95).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/latency_probe_test.cpp -o /tmp/latency_probe_test
/tmp/latency_probe_test
