#!/usr/bin/env bash
# Host tests for STORY-EP-014 / STORY-EP-015 / STORY-EP-016 (no Qt).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/device_document_test.cpp -o /tmp/device_document_test
/tmp/device_document_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/enclose_test.cpp -o /tmp/enclose_test
/tmp/enclose_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/debug_log_test.cpp -o /tmp/debug_log_test
/tmp/debug_log_test
