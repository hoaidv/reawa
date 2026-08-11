@SRS-EP-02
Feature: Epaper region sync map stroke stream and refresh
  As Epaper in an ADR-0009 session (interim wire)
  I apply Infini viewport and doc_snapshot, emit stroke_*, and refresh the drawing region
  So that pen and panel stay coherent with Infini

  # Shipped Qt path — STORY-EP-001 / EP-002

  @SRS-EP-02
  Scenario: Viewport updates map before next pen sample
    Given Epaper is in a live session with last-good viewport map M0
    When Infini sends a viewport message with new translate, scale, drawingRegion, orientation, and seq
    Then Epaper updates input-to-world via gut UV before the next pen sample
    And a region vector rasterize is scheduled

  @SRS-EP-02
  Scenario: Local stroke emits stroke stream without on-device Smart Group
    Given Epaper has current viewport map applied
    When a local pen stroke completes
    Then Epaper emits stroke_begin with world brush width then stroke_point panel samples then stroke_end
    And Epaper does not run Smart Group enclose recognition on-device

  @SRS-EP-02
  Scenario: doc_snapshot then settle keeps map and vectors coherent
    Given Epaper has current viewport map and receives doc_snapshot nodes
    When Infini sends viewport with settle true
    Then Epaper paints those vectors intersected with the current drawing region
    And the paint is a sharp full redraw not a downscaled bitmap blit

  @SRS-EP-02
  Scenario: region_refresh bitmap is ignored
    Given Epaper is in a live session
    When Infini sends a legacy region_refresh PNG message
    Then Epaper ignores it and keeps using doc_snapshot vectors

  @SRS-EP-02
  Scenario: Stroke send failure does not block local ink hot path
    Given local pen ink is being captured
    When stroke send fails on the network path
    Then the local ink hot path continues without waiting on socket I/O

  @SRS-EP-03
  Scenario: Hot path never runs blocking socket I/O on pen sample callback
    Given pen samples arrive on the input callback
    When each sample is handled
    Then local ink proceeds without blocking on socket I/O

  @SRS-EP-02
  Scenario: Panel local maps through drawingRegion not Infini screen formula
    Given Epaper panel size 200x100 and drawingRegion world AABB min (0,0) max (40,20)
    And orientation gutToLeft
    When a pen sample arrives at local (100, 50)
    Then world position is (20, 10)
    And the Infini screen formula local/scale - translate is not used for that sample

  @SRS-EP-03
  Scenario: Region refresh coalesces under viewport spam
    Given Epaper has a live map and soft refresh pending is clear
    When Infini sends 10 viewport messages with settle false within 100 ms
    Then the map seq equals the latest viewport seq immediately after each message
    And at most one soft region rasterize completes within any 250 ms window

  @SRS-EP-03
  Scenario: Settle flush refreshes promptly after last viewport
    Given a soft region refresh is pending after viewport spam
    When Infini sends viewport with settle true
    Then one sharp coherent paint runs even if the 250 ms floor has not elapsed

  @SRS-EP-03
  Scenario: Stroke panel width follows world width and region scale
    Given world strokeWidth 2.0 and drawingRegion width 40 mapped to panel width 200
    When Epaper computes panel line width
    Then lineWidth_px equals 10.0
    When drawingRegion width becomes 80 at the same panel width
    Then lineWidth_px equals 5.0

  @SRS-EP-03
  Scenario: Live ink uses world times panel scale after zoom out
    Given drawingRegion grew so panelScale halved versus prior zoom
    When the user draws a new stroke with the same pressure
    Then live lineWidth_px is about half the prior live thickness
    And it matches vector-rasterized thickness for the same world width

  # Library regionsync/ — unit tests only (not Qt binary)

  @SRS-EP-02 @library
  Scenario: RegionSession owns drawing region map in host tests
    Given ADR-0009 RegionSession is constructed in regionsync_test
    When viewport map and append_ink are exercised
    Then RegionSession map and PaintPass coalesce APIs pass host assertions
