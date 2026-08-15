@SRS-EP-10
Feature: Closure-first recognizer dispatch
  As an RM2 creator with Pen and both recognizers available
  I need exactly one document verdict per pen-up
  So that enclose, membership, connector, and ordinary ink never double-commit

  # STORY-EP-029 — SRS-EP-10 / ADR-0022. Do not retune shipped enclose or membership thresholds.

  @SRS-EP-10
  Scenario: One verdict and one recog log line when both recognizers are armed
    Given exclusive tool pen is armed
    And tgl.recog.ink_box and tgl.recog.connector are armed at pen-down
    When pen-up runs dispatch
    Then exactly one of enclose, membership, connector, or ink is committed
    And the device writes exactly one [recog] line with outcome enclose or membership or connector or ink

  @SRS-EP-10
  Scenario: Failed enclose inside an existing box may fall through to membership
    Given a Smart Group already on the document
    And a closed-ish stroke whose enclose guards fail
    And at least 80 percent of that stroke samples lie inside the existing group world bounds
    When pen-up runs dispatch
    Then create_smart_group is not committed for that stroke
    And draw-into membership may reparent the stroke as role content
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
