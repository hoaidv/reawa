@SRS-EP-10
Feature: Closure-first recognizer dispatch
  As an RM2 creator with Pen and both recognizers available
  I need exactly one document verdict per pen-up
  So that endpoint-ink, membership, enclose, connector, and ordinary ink never double-commit

  # STORY-EP-029 / STORY-EP-047 — SRS-EP-10 / ADR-0022. Membership uses boundary-ink length, not AABB sample-count.

  @SRS-EP-10
  Scenario: One verdict and one recog log line when both recognizers are armed
    Given exclusive tool pen is armed
    And tgl.recog.ink_box and tgl.recog.connector are armed at pen-down
    When pen-up runs dispatch
    Then exactly one of endpoint_ink, membership, enclose, connector, or ink is committed
    And the device writes exactly one [recog] line with outcome endpoint_ink or membership or enclose or connector or ink

  @SRS-EP-10
  Scenario: Draw-into membership beats enclose when the stroke is inside an existing box
    Given a Smart Group already on the document
    And a closed-ish stroke with at least 80 percent of its polyline length inside that group's boundary ink
    When pen-up runs dispatch
    Then create_smart_group is not committed for that stroke
    And draw-into membership reparents the stroke as role content
    And zero dual verdicts exist for that pen-up

  @SRS-EP-10
  Scenario: EP-016 enclose and EP-017 membership fixtures replay under new dispatch
    Given the STORY-EP-016 enclose fixtures and STORY-EP-017 membership fixtures
    When each fixture is replayed through ADR-0022 dispatch
    Then every verdict is unchanged except the deliberate failed-enclose-inside-box fall-through
    And no shipped enclose or membership threshold constant is retuned

  @SRS-EP-10
  Scenario: Dispatch does not regress ink latency
    Given exclusive tool pen is armed and both recognizers are armed
    When a stroke is painted
    Then pen-down to pixel p95 stays at most 30 ms
