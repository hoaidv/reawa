@SRS-EP-02
Feature: Epaper region sync map append_ink and refresh
  As Epaper in an ADR-0009 session
  I apply Infini viewport, emit world ink, and refresh the drawing region
  So that pen and panel stay coherent with Infini

  # STORY-EP-001 — SRS-EP-02 (+ SRS-EP-03 quality where cited)

  @SRS-EP-02
  Scenario: Viewport updates map before next pen sample
    Given Epaper is in a live session with last-good viewport map M0
    When Infini sends a viewport message with new translate, scale, drawingRegion, and seq
    Then Epaper updates input-to-world and ink transform to the new map before the next pen sample
    And a region refresh is enqueued

  @SRS-EP-02
  Scenario: Local stroke emits append_ink without on-device Smart Group
    Given Epaper has current viewport map applied
    When a local pen stroke completes with reported pressure and tilt channels
    Then Epaper emits append_ink on the document channel with samples in world space
    And emitted samples include x, y and every reported tablet channel from capture
    And Epaper does not run Smart Group enclose recognition on-device in v0

  @SRS-EP-02
  Scenario: Remote doc_op then refresh keeps map and doc coherent
    Given Epaper has current viewport map Mv and materialised tree T0
    When a remote doc_op with opId "remote_1" is applied idempotently yielding tree T1
    And the next region refresh runs
    Then the paint uses document T1 intersected with the current drawing region
    And that paint pass does not mix stale map with stale document (or vice versa)

  @SRS-EP-02
  Scenario: append_ink send failure does not block local ink hot path
    Given local pen ink is being captured
    When append_ink send fails on the network path
    Then retry or backoff is scheduled on a non-pen thread
    And the local ink hot path continues without waiting on socket I/O

  @SRS-EP-03
  Scenario: Hot path never runs socket I/O on pen sample callback
    Given pen samples arrive on the input callback
    When each sample is handled
    Then socket I/O is not executed on that callback (queued to net thread)
