@SRS-IN-02
Feature: Pan zoom pinch gesture inputs
  As a creator
  I need desktop pan and zoom gestures on the canvas
  So that I can navigate the infinite world without modal chrome

  # STORY-IN-004 — Spec states canvas.gesturing / canvas.resized

  @SRS-IN-02
  Scenario: Pan updates translate without teleport jumps
    Given Infini is focused on the CanvasStage
    And viewport translate starts at (0, 0)
    When the user pans via trackpad two-finger pan
    Then viewport.translate changes continuously from the start value
    And the world does not jump to an unrelated origin mid-gesture
    When the user pans via mouse drag on the canvas background
    Then viewport.translate updates in the drag direction
    When the user pans via mouse wheel without a zoom modifier
    Then viewport.translate updates without changing scale

  @SRS-IN-02
  Scenario: Zoom applies uniform scale about focus
    Given Infini is focused on the CanvasStage
    And viewport.scale starts at 1.0
    When the user zooms via trackpad pinch
    Then viewport.scale changes and remains uniform (scale_x = scale_y)
    And zoom focuses about the gesture focal point when provided
    When the user zooms via keyboard modifier plus wheel
    Then viewport.scale changes uniformly
    And StatusZoom shows round(scale * 100) percent

  @SRS-IN-02
  Scenario: Gesturing state has no modal chrome
    Given a pan or zoom gesture is in progress
    When ui.gesturing is true (canvas.gesturing)
    Then no modal or sheet overlays the CanvasStage
    And the canvas remains the interactive composition

  @SRS-IN-02
  Scenario: Window resize keeps world center anchor
    Given a populated canvas with a known world point under the window center
    When the Electron window is resized (canvas.resized)
    Then that world point remains under the window center
    And the resize anchor matches the Spec decision "center"
