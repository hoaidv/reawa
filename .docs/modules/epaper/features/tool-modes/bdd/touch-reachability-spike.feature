@SRS-EP-04
Feature: RM2 capacitive touch reachability spike
  As Epaper tool-modes pilot
  I need to know if finger touch reaches the Qt app
  So that toolbar design is not based on a false assumption

  # STORY-EP-004 — spike

  @SRS-EP-04
  Scenario: Spike records touch reachability
    Given the Epaper Qt event filter is instrumented for touch
    When the spike runs (device or harness)
    Then a written result states whether capacitive touch events are reachable (yes or no)
    And the result names the API path used (for example QEvent TouchBegin via event filter)

  @SRS-EP-04
  Scenario: Unreachable touch has an explicit fallback
    Given the spike result is "no" or touch is not handled by the app today
    When the spike closes
    Then a written fallback recommendation exists (pen-on-strip or hardware button)
    And STORY-EP-003 may leave draft only after that recommendation is recorded

  @SRS-EP-04
  Scenario: Pen path is untouched by the spike
    Given TabletAppFilter already forwards pen TabletPress/Move/Release
    When the touch probe is added
    Then pen event handling remains the primary ink path
    And touch events are not required for pen ink to work
