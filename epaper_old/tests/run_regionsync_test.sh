#!/usr/bin/env bash
# Host fixture path for STORY-EP-001 / @SRS-EP-02 (no Qt required).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
c++ -std=c++17 -Wall -Wextra -I. tests/regionsync_test.cpp -o /tmp/regionsync_test
/tmp/regionsync_test
