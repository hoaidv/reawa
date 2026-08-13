@SRS-IN-07
Feature: Infini tablet session viewport load and change channels
  As Infini in an ADR-0014 session
  I publish viewport, send exactly one document load per epoch, and apply device changes
  So that the drawing region stays in sync and the mirror follows the device

  # Revised 2026-08-13 — CHL-0008 / ADR-0014 / ADR-0015. doc_snapshot pushes are gone:
  # the desktop sends one handshake-gated doc_load per epoch and otherwise only viewport.

  # Shipped wire — STORY-IN-009 / IN-011

  @SRS-IN-07
  Scenario: Pan zoom emits viewport message
    Given a live session between Infini and Epaper
    When Infini pans to translate (-120, 40) and zooms to uniform scale 1.5
    Then a viewport message is emitted toward Epaper
    And the message includes type "viewport", translate, scale, drawingRegion AABB, orientation, and monotonic seq

  @SRS-IN-07
  Scenario: Epaper stroke stream renders as a transient preview
    Given a live session and Infini WorldLayer
    When Epaper sends stroke_begin with world brush width then stroke_point panel samples then stroke_end
    Then Infini maps panel coords through gut UV into drawingRegion world space
    And a preview path appears keyed by stroke id with that world stroke width
    And the preview is not written into the mirror

  @SRS-IN-07
  Scenario: Connect sends one document load after the queue drains
    Given the Epaper client connects and reports queued changes
    When Infini completes the handshake
    Then Infini sends drain_ack first
    And Infini applies every inbound doc_change in seq order
    And Infini sends doc_load only after queue_empty
    And Infini does not push a region_refresh PNG for that content
    When Infini later pans or zooms
    Then only viewport messages are required for Epaper to re-rasterize locally

  @SRS-IN-07
  Scenario: No document message follows the load
    Given a session that has completed its initial load
    When Infini edits nothing and the session is traced
    Then zero outbound document messages other than that load are observed

  @SRS-IN-07
  Scenario: Applied change replaces the preview
    Given a preview path exists for stroke "s_9"
    When the doc_change carrying that stroke's node applies to the mirror
    Then the preview is removed
    And the mirror node paints in its place

  @SRS-IN-07
  Scenario: Sequence gap marks the mirror suspect
    Given the mirror last applied seq 12
    When a doc_change arrives with baseSeq 15
    Then the mirror is marked suspect
    And Infini requests an explicit resync
    And Infini does not save a suspect mirror silently

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

  # Library / applier helpers — not live CanvasStage wire

  @SRS-IN-07
  Scenario: append_ink from Epaper updates tree and WorldLayer
    Given VectorDocument session helpers under unit test
    When an Epaper-shaped doc_change carrying append_ink with opId "ink_1" is applied
    Then the materialised tree contains the new Ink node
    When the same opId "ink_1" is applied again
    Then the tree is unchanged (idempotent)

  @SRS-IN-07 @future
  Scenario: Infini authors no document ops during a session
    Given a live session and desktop interaction with the canvas
    When outbound messages are observed for the whole session
    Then Infini emits zero document ops
    And the only outbound document message is the epoch's doc_load
