@SRS-EP-07
Feature: Device stroke ingestion into the local tree
  As an RM2 creator
  I need finished pen strokes to become Ink nodes in the on-device tree
  So that the panel paints local document authority with no peer picture and no ingest round trip

  # New 2026-08-13 — STORY-EP-014 / SRS-EP-07 ingestion + SRS-EP-09 sample retention and
  # ops/ fixtures. Undo stays in undo-ring.feature; publish stays in one-way-sync.feature.
  # Do not rewrite those files. Op type names are the SRS-IN-09 transmit set.

  @SRS-EP-07
  Scenario: Finished strokes become Ink nodes with no session
    Given no session has connected
    And the Pen tool is armed
    When the creator draws 20 strokes to completion
    Then each finished stroke is an Ink node in the local tree within p95 50 ms after pen-up
    And the panel paints that tree
    And zero inbound peer pictures are a paint source

  @SRS-EP-07
  Scenario: Selection armed does not ingest ink
    Given Selection is armed
    And the local tree has zero Ink nodes
    When the pen moves on the panel
    Then no stroke begins
    And no Ink node is ingested

  @SRS-EP-09
  Scenario: Digitizer channels survive on the stored node
    Given a finished stroke whose digitizer reported pressure, tilt, and extras
    When the Ink node is stored
    Then 100 percent of those reported channels are present on the node
    And the preview stream may omit tilt and extras

  @SRS-EP-09
  Scenario: Shared ops fixtures agree device and desktop
    Given the shared fixtures at features/vector-document/fixtures/ops/
    When the device applies the same op sequence as the desktop
    Then the trees agree 100 percent
    And any divergence is a CHL not a widened tolerance

  @SRS-EP-07
  Scenario: Ingestion does not steal the next stroke's ink budget
    Given the previous stroke has already been ingested as an Ink node
    When the next stroke is measured from pen-down to pixel
    Then pen-down to pixel p95 remains 30 ms or less
    And zero samples are dropped or delayed by ingestion
