@SRS-IN-07
Feature: Infini tablet session viewport and document channel
  As Infini in an ADR-0009 session
  I publish viewport and apply Epaper document ops
  So that drawing region and tree stay in sync

  # STORY-IN-009 — SRS-IN-07 (+ SRS-IN-08 quality where cited)

  @SRS-IN-07
  Scenario: Pan zoom emits viewport message
    Given a live session between Infini and Epaper
    When Infini pans to translate (-120, 40) and zooms to uniform scale 1.5
    Then a viewport message is emitted on the viewport channel toward Epaper
    And the message includes type "viewport", translate, scale, drawingRegion AABB, and monotonic seq

  @SRS-IN-07
  Scenario: append_ink from Epaper updates tree and WorldLayer
    Given a live session and an empty or open Infini document
    When Epaper sends doc_op append_ink with opId "ink_1" and world-space samples
    And Infini applies that op
    Then the materialised tree contains the new Ink node
    And WorldLayer reflects the new ink on the next paint
    When the same opId "ink_1" is applied again
    Then the tree is unchanged (idempotent)

  @SRS-IN-07
  Scenario: Infini structure emit respects in-flight Epaper stroke
    Given the v0 emit matrix allows Infini to emit structure and Smart Group ops
    And an Epaper stroke is in flight (samples still arriving for the current stroke)
    When Infini would emit a structure or create_smart_group op
    Then Infini does not race the in-flight stroke (pause or queue per product rule)
    When no Epaper stroke is in flight
    And Infini edits structure
    Then those ops may be emitted on the document channel

  @SRS-IN-07
  Scenario: Duplicate opId ignored and unknown type logged
    Given a live Infini document channel consumer
    When a doc_op with a previously applied opId arrives
    Then the duplicate is ignored and the tree is unchanged
    When a doc_op with an unknown type arrives
    Then the event is logged
    And the process does not crash
    And the prior tree is retained

  @SRS-IN-08
  Scenario: Viewport map apply meets latency budget
    Given a live session
    When Infini emits a viewport change
    Then Epaper map apply completes within p95 100 ms (panel refresh may trail)
