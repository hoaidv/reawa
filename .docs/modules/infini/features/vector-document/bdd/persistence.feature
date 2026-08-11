@SRS-IN-09
Feature: SVG persistence and shared op fixtures
  As Infini and Epaper peers
  I need round-trip SVG and shared op envelopes
  So that disk and wire agree on document shape

  # STORY-IN-008 — SRS-IN-09

  @SRS-IN-09
  Scenario: SVG save load preserves ink sample channels
    Given a materialised tree with Ink samples that include x, y, pressure 0.42, tiltX 0.1, and extras.distance 0
    When save serializes to the Infini SVG profile and load parses that file back
    Then the reloaded Ink samples retain x and y
    And pressure, tiltX, and extras.distance match the pre-save values

  @SRS-IN-09
  Scenario: Unknown Infini-required structure fails closed
    Given an SVG file that claims Infini structure but omits a required Infini element or attribute
    When parse runs in v0
    Then load fails closed and the prior tree is unchanged
    Given an SVG file with foreign fluff elements outside Infini-required structure
    When parse runs in v0
    Then foreign fluff may be skipped with a warning log and Infini-required content still loads

  @SRS-IN-09
  Scenario: Dual TS and Qt fixtures share op envelope
    Given SRS-IN-09 op envelope schema { opId, type, payload, source }
    When dual fixtures are authored for TypeScript and Qt consumers
    Then both fixture sets include the same example ops for append_ink and at least one structure op
    And each example has opId, type, payload, and source of "epaper" or "infini"
