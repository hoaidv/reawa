@SRS-IN-01
Feature: Infini Electron shell and canvas host
  As a creator on macOS
  I need Infini to boot an Electron window with a full-pane canvas
  So that the infinity surface from [UI-IN-01] has a stable host

  # STORY-IN-002

  @SRS-IN-01
  Scenario: Electron window hosts React canvas root
    Given ADR-0008 Electron + React is the Infini shell
    And the Infini app is launched on macOS
    When the main window finishes loading
    Then an Electron BrowserWindow is visible
    And a React root mounts a full-pane element with data-region "CanvasStage"
    And the surface corresponds to scene.canvas from the UI Spec

  @SRS-IN-01
  Scenario: Stable mount point without Reawa Swift reuse
    Given the Infini shell has opened its main window
    When the canvas host is inspected
    Then a stable DOM mount point exists for the infinity canvas renderer
    And no production Reawa Swift UI module is loaded into that host

  @SRS-IN-01
  Scenario: Chrome regions follow approved hi-fi Spec
    Given design package ".plan/iter-002/design/infinity-canvas/" is approved
    When chrome is rendered in the shell
    Then regions WindowFrame, CanvasStage, WorldLayer, and StatusZoom are present
    And layout tokens match the package tokens (no invented region tree)
