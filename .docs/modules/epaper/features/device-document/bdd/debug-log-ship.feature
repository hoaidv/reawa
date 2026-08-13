@SRS-EP-15
Feature: Device console log ship on TCP 9878
  As the device in a live Infini session
  I ship Qt (and stdio when captured) logs on a sidecar when requested
  So that the desktop can inspect enclose and ingest without invading Smart Group logic

  # New 2026-08-13 — STORY-EP-021 / SRS-EP-15 · SRS-EP-16.
  # Not a document channel. Zero debug_* on :9877. Zero edits to recognize_enclose.hpp.

  @SRS-EP-15
  Scenario: Qt logs ship after debug_start
    Given EPAPER_DEBUG_LOG is on
    And RM_SYNC_HOST is set
    And the debug socket is connected
    When Infini sends debug_start
    And the device emits qInfo "hello-ship"
    Then a debug_log line is written on TCP 9878
    And level is "info" and logger is "qt"
    And msg contains "hello-ship"

  @SRS-EP-15 @SRS-EP-16
  Scenario: Handler never blocks the GUI thread on the socket
    Given EPAPER_DEBUG_LOG is on and shipping is on
    When qWarning is emitted on the GUI thread
    Then that thread performs 0 write, flush, or waiting socket syscalls
    And the record is enqueued or dropped for the worker

  @SRS-EP-16
  Scenario: ingestPoint and paint do no log I/O
    Given debug shipping is on
    When ingestPoint runs
    And paint runs
    Then those stacks perform 0 debug-port or handler I/O

  @SRS-EP-15 @SRS-EP-16
  Scenario: Queue overflow drops oldest
    Given the ship queue holds 512 records
    When one more record is enqueued
    Then the oldest record is gone
    And the next emitted debug_log.dropped is at least 1

  @SRS-EP-15
  Scenario: Stdio capture ships or degrades
    Given EPAPER_DEBUG_LOG is on
    When stdout and stderr fd capture is attempted
    Then if capture succeeds those lines ship with logger "stdio"
    And if capture fails Qt logs still ship
    And one debug_log msg is "[debug] stdio capture unavailable"
    And the process does not abort

  @SRS-EP-15
  Scenario: Enclose ingest emits one tagged qInfo
    Given shipping may be on or off
    When ingestStrokeAtPenUp returns
    Then exactly one qInfo whose msg starts with "[enclose]" is emitted
    And recognize_enclose.hpp guards are unchanged

  @SRS-EP-15 @SRS-EP-16
  Scenario: Env off never connects
    Given EPAPER_DEBUG_LOG is unset or not in {1, true, on, yes}
    When the app runs
    Then 0 TCP connects are made to port 9878
    And 0 debug sockets exist

  @SRS-EP-15 @SRS-EP-16
  Scenario: Document types on 9878 are not applied
    Given the debug worker is connected
    When a doc_change line arrives on :9878
    Then DeviceDocument is unchanged
    And 0 doc_change are queued from that line
