#!/usr/bin/env bash
# Host tests for STORY-EP-014 / STORY-EP-015 / STORY-EP-016 / STORY-EP-017 (no Qt).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/device_document_test.cpp -o /tmp/device_document_test
/tmp/device_document_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/enclose_test.cpp -o /tmp/enclose_test
/tmp/enclose_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/membership_test.cpp -o /tmp/membership_test
/tmp/membership_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/surround_create_test.cpp -o /tmp/surround_create_test
/tmp/surround_create_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/debug_log_test.cpp -o /tmp/debug_log_test
/tmp/debug_log_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/manipulate_test.cpp -o /tmp/manipulate_test
/tmp/manipulate_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/one_way_sync_test.cpp -o /tmp/one_way_sync_test
/tmp/one_way_sync_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/toolchip_layout_test.cpp -o /tmp/toolchip_layout_test
/tmp/toolchip_layout_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/dispatch_test.cpp -o /tmp/dispatch_test
/tmp/dispatch_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/enclose_shape_test.cpp -o /tmp/enclose_shape_test
/tmp/enclose_shape_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/connector_test.cpp -o /tmp/connector_test
/tmp/connector_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/connector_warp_test.cpp -o /tmp/connector_warp_test
/tmp/connector_warp_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/recog_warp_bench.cpp -o /tmp/recog_warp_bench
/tmp/recog_warp_bench
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/ingest_origin_guard_test.cpp -o /tmp/ingest_origin_guard_test
/tmp/ingest_origin_guard_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/usbgadget_test.cpp -o /tmp/usbgadget_test
/tmp/usbgadget_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/hand_touch_test.cpp -o /tmp/hand_touch_test
/tmp/hand_touch_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/viewport_follow_test.cpp -o /tmp/viewport_follow_test
/tmp/viewport_follow_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/canvas_frame_test.cpp -o /tmp/canvas_frame_test
/tmp/canvas_frame_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/stroke_capture_test.cpp -o /tmp/stroke_capture_test
/tmp/stroke_capture_test
c++ -std=c++17 -Wall -Wextra -O2 -I. tests/transform_session_test.cpp -o /tmp/transform_session_test
/tmp/transform_session_test
c++ -std=c++17 -Wall -Wextra -O2 -I. rendering/rendering.cpp tests/rendering_test.cpp -o /tmp/rendering_test
/tmp/rendering_test
