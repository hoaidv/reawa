/**
 * @implements [SRS-IN-01] Infini app shell — Electron/React host
 * @implements [SRS-IN-02] WindowFrame composition from UI Spec
 * @implements [SRS-IN-18] Device Log overlay is a WindowFrame child, not WorldLayer
 */

import { CanvasStage } from "./canvas/CanvasStage";
import { DeviceLogChrome } from "./debuglog/DeviceLogChrome";

export function App() {
  return (
    <div data-region="WindowFrame" data-platform="desktop">
      <CanvasStage />
      <DeviceLogChrome />
    </div>
  );
}
