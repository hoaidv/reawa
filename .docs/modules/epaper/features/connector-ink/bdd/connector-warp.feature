@SRS-EP-18
Feature: Ink and Curve warp on bound-node drag
  As an RM2 creator moving a connected Smart Group
  I need the connector to re-warp live from the rest shape
  So that the line stays attached without a jump at pen-up

  # STORY-EP-031 — SRS-EP-18 / SRS-EP-20. Reimplement ADR-0020; do not copy EXP-0002 probe code.

  @SRS-EP-18
  Scenario: Bound box drag re-warps live without full-panel invalidation
    Given a connector bound to Smart Group A and Smart Group C
    When the creator moves or resizes A
    Then the connector re-warps at least 5 times per second while the gesture is live
    And zero full-panel invalidations occur
    And committed geometry equals the last previewed geometry
    And UI freeze stays at most 200 ms
    And region ConnSelect stays hidden unless the connector is selected

  @SRS-EP-18
  Scenario: Morph at rest is bitwise the rest-shape reconstruction
    Given a connector with warpStyle morph
    And neither bound node has turned since rest
    When warp runs
    Then the output samples are bitwise identical to reconstructing the rest shape

  @SRS-EP-20
  Scenario: Never-re-bake round-trip drifts 0.000 u on host fixtures
    Given a connector rest shape on host fixtures
    When 200 poses are applied and then returned to the start pose
    And the rest shape is never re-baked
    Then endpoint drift is 0.000 world units

  @SRS-EP-18
  Scenario: Deleting a bound box keeps the connector
    Given a connector bound to nodeId A
    When A is deleted
    Then the connector remains drawn using the last live world pose of A
    And the connector is not marked invalid
    When undo restores A
    Then the same nodeId A resolves live again

  @SRS-EP-20
  Scenario: Default-on false positives are a ship gate
    Given both recognizers armed on a labelled pen corpus
    When Initiative 2 of EXP-0002 is scored
    Then false positives are at most 2 percent of pen strokes
    And a fresh page first 20 strokes produce 0 unnamed connectors
    And this bar does not block STORY-EP-028 implementation
