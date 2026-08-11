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

  # STORY-EP-002 — coalesce refresh + panel→region map + stroke parity

  @SRS-EP-02
  Scenario: Panel local maps through drawingRegion not Infini screen formula
    Given Epaper panel size 200x100 and drawingRegion world AABB min (0,0) max (40,20)
    When a pen sample arrives at local (100, 50)
    Then world position is (20, 10)
    And the Infini screen formula local/scale - translate is not used for that sample

  @SRS-EP-03
  Scenario: Region refresh coalesces under viewport spam
    Given Epaper has a live map and refresh pending is clear
    When Infini sends 10 viewport messages within 100 ms
    Then the map seq equals the latest viewport seq immediately after each message
    And at most one region refresh paint completes within any 250 ms window
    And a later run after 250 ms paints the latest pending map and doc pair

  @SRS-EP-03
  Scenario: Settle flush refreshes promptly after last viewport
    Given a region refresh is pending after viewport spam
    When Infini gesture settles and Epaper runs settle refresh within 100 ms
    Then one coherent paint runs even if the 250 ms floor has not elapsed

  @SRS-EP-02
  Scenario: Viewport settle rasterizes vectors sharply from local doc
    Given Epaper holds a doc_snapshot of vector nodes and a valid drawingRegion
    When Infini sends viewport with settle true
    Then Epaper clears and paints document intersect drawingRegion from vectors
    And the paint is a sharp full redraw not a downscaled bitmap blit

  @SRS-EP-03
  Scenario: Stroke panel width follows world width and region scale
    Given world strokeWidth 2.0 and drawingRegion width 40 mapped to panel width 200
    When Epaper computes panel line width
    Then lineWidth_px equals 10.0
    When drawingRegion width becomes 80 at the same panel width
    Then lineWidth_px equals 5.0

  @SRS-EP-02
  Scenario: ADR-0009 session owns drawing region map over StrokeSync
    Given ADR-0009 RegionSession is connected and owns the viewport map
    When legacy StrokeSync would otherwise set the drawing-region map
    Then RegionSession map remains authoritative for pen→world
