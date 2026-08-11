@SRS-IN-03
Feature: Gesture frame budget on 60Hz display
  As a creator
  I need pan and zoom to stay smooth on a 60 Hz display
  So that Infini meets the quality target without silently switching shells

  # STORY-IN-005

  @SRS-IN-03
  Scenario: Continuous pan meets dropped-frame budget
    Given Infini is displaying on a 60 Hz display
    And a requestAnimationFrame counter or equivalent is recording frames
    When the user pans continuously for at least 5 seconds
    Then perceived dropped frames are at most 2 per second
    And the measurement evidence is recorded in the story notes or test output

  @SRS-IN-03
  Scenario: Continuous zoom meets budget and preserves circle aspect
    Given a circle primitive is visible on the canvas
    And Infini is on a 60 Hz display
    When the user pinches or uses modifier plus wheel to zoom continuously for at least 5 seconds
    Then perceived dropped frames are at most 2 per second
    And the circle remains circular (aspect 1:1 within 1 CSS px)

  @SRS-IN-03
  Scenario: Budget failure files a challenge not a silent shell swap
    Given ADR-0008 lists Electron gesture jank as a risk
    When the frame-budget spike fails the ≤2 dropped frames/s target
    Then a challenge CHL-* is filed for PM/Architect triage
    And the application shell is not silently switched away from Electron
