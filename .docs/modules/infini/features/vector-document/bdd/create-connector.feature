@SRS-IN-09
Feature: Infini mirrors create_connector and derived warp
  As Infini session persistence
  I need to apply the device envelope and re-derive geometry
  So that the desktop viewer matches the tablet with 0 extra connector ops

  # STORY-IN-030 — SRS-IN-09 / ADR-0020. Infini does not author connectors this campaign.

  @SRS-IN-09
  Scenario: create_connector envelope round-trips with 0 loss
    Given a device create_connector op with id, from, to, warpStyle, body, and restShape
    And from and to each carry kind edge or centre, nodeId, edge, t, and drawnEdgeLocal
    When Infini applies the op
    Then from, to, warpStyle, body, and restShape round-trip with 0 loss
    And Infini does not stream warped samples on the wire

  @SRS-IN-09
  Scenario: Shared rest shape yields byte-comparable samples
    Given the same rest shape, endpoint states, and warpStyle on device and Infini
    When both ends warp
    Then sample lists are byte-comparable
    And divergent connector nodes equal 0

  @SRS-IN-09
  Scenario: set_smart_transform emits 0 connector ops
    Given a connector bound to Smart Group A
    When Infini applies set_smart_transform on A
    Then Infini emits 0 create_connector or connector-mutate ops
    And Infini re-derives connector geometry from rest shape and live poses

  @SRS-IN-09
  Scenario: Missing bound node uses last live pose
    Given a connector whose from.nodeId is absent from the tree
    When Infini draws the connector
    Then it uses the last live pose cache
    And it does not mark the connector invalid
