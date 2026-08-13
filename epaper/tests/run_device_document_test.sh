#!/usr/bin/env bash
# Host tests for STORY-EP-014 / [SRS-EP-07] [SRS-EP-09] (no Qt).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/device_document_test.cpp -o /tmp/device_document_test
/tmp/device_document_test
