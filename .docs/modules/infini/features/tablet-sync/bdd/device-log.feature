@SRS-IN-17
Feature: Device Log sidecar on TCP 9878
  As a desktop debugger of a live tablet session
  I need a separate :9878 channel and an in-app Device Log panel
  So that I can inspect EP-012…016 device console output without touching the document

  # New 2026-08-13 — STORY-IN-029 / SRS-IN-17 · SRS-IN-18 · SRS-IN-19.
  # Debug family is not ADR-0015. Zero debug_* on :9877.

  @SRS-IN-18
  Scenario: Device Log button is window chrome
    Given Infini is running
    When the window loads
    Then a control labeled "Device Log" is visible in WindowFrame
    And it is not a child of WorldLayer
    And TCP 9878 is listening (INFINI_DEBUG_PORT default)

  @SRS-IN-18 @SRS-IN-17
  Scenario: Opening the overlay starts shipping
    Given the Device Log overlay is closed
    When the user clicks Device Log
    Then a full-size in-app panel covers the canvas (not a second BrowserWindow)
    And Infini sends debug_request then debug_start on :9878
    And the overlay state is streaming or disconnected

  @SRS-IN-18
  Scenario: Open overlay with empty buffer
    Given the overlay is open
    And the in-memory buffer has 0 lines
    And a debug client is connected
    Then the stream body shows "No device log yet"
    And the state is dlog.open_empty

  @SRS-IN-18
  Scenario: Open overlay while disconnected
    Given the overlay is open
    And no client is connected on :9878
    Then the stream body shows "Device log not connected"
    And the hint mentions EPAPER_DEBUG_LOG and TCP 9878
    And the state is dlog.disconnected

  @SRS-IN-17 @SRS-IN-19
  Scenario: Inbound debug_log appends to the ring and never mutates the document
    Given the Device Log buffer is empty
    When a debug_log line arrives on :9878 with msg "[enclose] armed=ink_box outcome=created"
    Then that line is in the in-memory ring
    And it appears in list.device_log_stream
    And VectorDocument is unchanged (0 apply)

  @SRS-IN-17 @SRS-IN-19
  Scenario: Ring overflow drops oldest
    Given the Device Log ring holds 10000 records
    When one more debug_log arrives
    Then the oldest record is gone
    And the ring size is 10000
    And VectorDocument is unchanged

  @SRS-IN-18
  Scenario: Filter is view-only
    Given the buffer holds lines "alpha" and "bravo"
    When the user types "BRA" into field.device_log_filter
    Then only the bravo line is shown
    And the buffer still holds both lines
    And Infini sends 0 filter messages to the device

  @SRS-IN-18
  Scenario: Filter empty copy
    Given the buffer holds one line "alpha"
    When the user types "zzz" into the filter
    Then the stream body shows "No lines match the filter"
    And the state is dlog.filtered

  @SRS-IN-17 @SRS-IN-18
  Scenario: Close sends debug_stop and keeps the buffer
    Given the overlay is open
    And the buffer holds 3 lines
    When the user clicks Close or presses Escape
    Then the overlay is hidden
    And Infini sends debug_stop on :9878
    And the buffer still holds 3 lines

  @SRS-IN-17 @SRS-IN-19
  Scenario: Document types on 9878 are dropped
    Given a live debug decoder on :9878
    When a viewport or doc_change or stroke_begin line is parsed
    Then it is dropped
    And it is never forwarded to the :9877 decoder
    And VectorDocument is unchanged

  @SRS-IN-19
  Scenario: Paint and apply do no debug-port I/O
    Given the Device Log panel is open
    And debug_log lines arrive at 200 per second
    When the canvas paints and a doc_change applies
    Then those stacks perform 0 TCP reads or writes on :9878
