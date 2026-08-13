@SRS-IN-13 @retired
Feature: Tool intent transport
  As Infini ↔ Epaper sync
  I need stroke intent, pickables, and tool_intent on the wire
  So that device tools work without syncing tool mode

  # RETIRED 2026-08-13 — CHL-0008 / ADR-0014. Superseded by epaper SRS-EP-08 (sync
  # contract) and SRS-EP-11 (local hit-test and manipulation). The device no longer
  # asks a peer what it may touch, so intent, pickables, and tool_intent left the wire.
  #
  # Kept for history and traceability per .agent/rules/lifecycle.md — do not implement,
  # do not run. The one rule below that survived is re-asserted on the device side in
  # epaper/features/device-document/bdd/one-way-sync.feature: tool mode is never synced.

  # STORY-IN-018 — SRS-IN-13

  @SRS-IN-13 @retired
  Scenario: stroke_begin carries enclose or ink intent
    Given an Epaper stroke_begin message
    When intent is enclose or ink or absent
    Then Infini treats absent or unrecognised as ink
    And enclose rides through to enclose recognition

  @SRS-IN-13 @retired
  Scenario: doc_snapshot includes pickables for Smart Groups
    Given Infini has one or more SmartGroup nodes
    When doc_snapshot is published
    Then pickables lists each smart_group with id kind and world bounds AABB

  @SRS-IN-13 @retired
  Scenario: tool_intent applies on Infini without bidirectional doc_op
    Given a tool_intent move or resize for a known SmartGroup nodeId
    When Infini receives it
    Then set_smart_transform is applied locally
    And no tool mode field is required on the message

  @SRS-IN-13 @retired
  Scenario: tool mode is never synced
    Given tool mode changes on either device
    When observing outbound session messages
    Then no message type carries a device tool mode field
