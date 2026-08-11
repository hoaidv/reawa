@SRS-IN-07
Feature: Infini tablet session viewport and document channel
  As Infini in an ADR-0009 session (interim wire)
  I publish viewport and doc_snapshot and ingest Epaper strokes
  So that drawing region and region picture stay in sync

  # Shipped wire — STORY-IN-009 / IN-011

  @SRS-IN-07
  Scenario: Pan zoom emits viewport message
    Given a live session between Infini and Epaper
    When Infini pans to translate (-120, 40) and zooms to uniform scale 1.5
    Then a viewport message is emitted toward Epaper
    And the message includes type "viewport", translate, scale, drawingRegion AABB, orientation, and monotonic seq

  @SRS-IN-07
  Scenario: Epaper stroke stream becomes WorldLayer path
    Given a live session and Infini WorldLayer
    When Epaper sends stroke_begin with world brush width then stroke_point panel samples then stroke_end
    Then Infini maps panel coords through gut UV into drawingRegion world space
    And a path primitive appears on WorldLayer with that world stroke width

  @SRS-IN-07
  Scenario: Initial sync pushes vector doc_snapshot not bitmap
    Given a live session and Infini has world primitives under the tablet frame
    When the Epaper client connects or Infini first publishes viewport
    Then Infini sends a doc_snapshot with vector nodes (line rect ellipse path)
    And Infini does not push a region_refresh PNG for that content
    When Infini later pans or zooms
    Then only viewport messages are required for Epaper to re-rasterize locally

  @SRS-IN-07
  Scenario: Drawing-region marker hidden when idle
    Given Infini canvas is shown with tablet sync session active
    And the user is not panning or zooming
    Then the tablet drawing-region marker is not visible

  @SRS-IN-07
  Scenario: Drawing-region marker visible during pan zoom
    Given Infini canvas with tablet sync session active
    When the user starts a pan or zoom gesture
    Then the tablet drawing-region marker is visible
    And the marker outline matches the tablet drawing frame in CSS
    And viewport drawingRegion equals the world AABB of that frame

  @SRS-IN-07
  Scenario: Drawing-region marker hides after gesture settles
    Given the tablet drawing-region marker is visible during a gesture
    When the pan or zoom gesture ends and settles
    Then the marker is not visible as permanent chrome

  @SRS-IN-07
  Scenario: Viewport drawingRegion is tablet frame not full window
    Given a live session and CSS host 800x600 with a centered tablet frame
    When Infini publishes viewport at translate (0, 0) scale 1
    Then drawingRegion equals the world AABB of the tablet CSS frame
    And drawingRegion is strictly inside the full-window world AABB unless the frame fills the host

  @SRS-IN-07
  Scenario: Rapid pan zoom coalesces viewport publishes
    Given a live session
    When Infini receives 60 viewport updates within one second during a gesture
    Then outbound viewport messages are at most 30
    And the last emitted message carries the latest translate scale and drawingRegion
    When the gesture ends
    Then a final viewport flush is emitted with settle true

  @SRS-IN-07
  Scenario: Gut orientation cycles four poses
    Given Infini Sync orientation control
    When the user cycles orientation
    Then orientation is one of gutToLeft gutOnTop gutAtBottom gutToRight
    And the tablet frame aspect is tall for gutToLeft and gutToRight
    And the tablet frame aspect is wide for gutOnTop and gutAtBottom

  @SRS-IN-08
  Scenario: Viewport map apply meets latency budget
    Given a live session
    When Infini emits a viewport change
    Then Epaper map apply completes within p95 100 ms (panel refresh may trail)

  @SRS-IN-08
  Scenario: World stroke width scales with viewport on Infini
    Given world ink with strokeWidth 2.0 and tablet frame width F_css
    When Infini paints at scale 1.0 then at scale 0.5
    Then CSS line width halves when scale halves
    And relative thickness lineWidth_css / F_css stays consistent with ADR-0012

  # Library / future op-log — not live CanvasStage wire

  @SRS-IN-07 @future
  Scenario: append_ink from Epaper updates tree and WorldLayer
    Given VectorDocument session helpers under unit test
    When Epaper-shaped doc_op append_ink with opId "ink_1" is applied
    Then the materialised tree contains the new Ink node
    When the same opId "ink_1" is applied again
    Then the tree is unchanged (idempotent)

  @SRS-IN-07 @future
  Scenario: Infini structure emit respects in-flight Epaper stroke
    Given session queue helpers for structure ops
    And an Epaper stroke is in flight
    When Infini would emit a structure or create_smart_group op
    Then Infini does not race the in-flight stroke (pause or queue)
