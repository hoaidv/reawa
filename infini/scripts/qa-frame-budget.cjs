/**
 * QA probe for STORY-IN-005 / SRS-IN-03 — synthetic pan+zoom under ?trace=1.
 * Does not modify product source. Exit 0 if drops/s ≤ 2 over the window.
 */
const { app, BrowserWindow } = require("electron");

const PAN_MS = 5500;
const ZOOM_MS = 5500;

app.whenReady().then(async () => {
  const win = new BrowserWindow({
    width: 1280,
    height: 800,
    show: false,
    webPreferences: { contextIsolation: true },
  });

  await win.loadURL("http://127.0.0.1:5173/?trace=1");
  await new Promise((r) => setTimeout(r, 800));

  const result = await win.webContents.executeJavaScript(`
    (async () => {
      const host = document.querySelector("[data-region=CanvasStage]");
      if (!host) return { err: "no CanvasStage" };
      const rect = host.getBoundingClientRect();
      const cx = rect.left + rect.width / 2;
      const cy = rect.top + rect.height / 2;

      const fireWheel = (dx, dy, ctrlKey) => {
        host.dispatchEvent(new WheelEvent("wheel", {
          bubbles: true, cancelable: true,
          clientX: cx, clientY: cy,
          deltaX: dx, deltaY: dy, deltaMode: 0, ctrlKey,
        }));
      };

      const reset = () => {
        // expose counters via beforeunload dump pattern — read from DOM after gestures
      };

      // Warm paint
      fireWheel(0, 1, false);
      await new Promise((r) => requestAnimationFrame(r));

      const t0 = performance.now();
      let frames0 = 0, drops0 = 0;
      // Hook into same counters the stage keeps — inject temporary observer via console
      // We cannot access React refs; approximate by counting rAF ourselves during gesture.
      let frames = 0, drops = 0, last = 0;
      let measuring = true;
      const loop = (t) => {
        if (!measuring) return;
        if (last > 0) {
          const dt = t - last;
          frames++;
          if (dt > 1000 / 60 + 4) drops++;
        }
        last = t;
        requestAnimationFrame(loop);
      };
      requestAnimationFrame(loop);

      const panStart = performance.now();
      while (performance.now() - panStart < ${PAN_MS}) {
        fireWheel(12, 8, false);
        await new Promise((r) => setTimeout(r, 8));
      }
      const panFrames = frames;
      const panDrops = drops;
      const panElapsed = (performance.now() - panStart) / 1000;

      frames = 0; drops = 0; last = 0;
      const zoomStart = performance.now();
      while (performance.now() - zoomStart < ${ZOOM_MS}) {
        fireWheel(0, 20, true);
        await new Promise((r) => setTimeout(r, 8));
      }
      const zoomFrames = frames;
      const zoomDrops = drops;
      const zoomElapsed = (performance.now() - zoomStart) / 1000;
      measuring = false;

      const regions = {
        WindowFrame: !!document.querySelector('[data-region="WindowFrame"]'),
        CanvasStage: !!document.querySelector('[data-region="CanvasStage"]'),
        WorldLayer: !!document.querySelector('[data-region="WorldLayer"]'),
        StatusZoom: !!document.querySelector('[data-region="StatusZoom"]'),
      };
      const zoomText = document.querySelector('[data-region="StatusZoom"]')?.textContent || "";
      const stats = document.querySelector(".gesture-legend span")?.textContent || "";

      return {
        regions,
        zoomText,
        stats,
        pan: { frames: panFrames, drops: panDrops, elapsed_s: panElapsed, dropsPerSec: panDrops / panElapsed },
        zoom: { frames: zoomFrames, drops: zoomDrops, elapsed_s: zoomElapsed, dropsPerSec: zoomDrops / zoomElapsed },
      };
    })()
  `);

  console.log(JSON.stringify(result, null, 2));
  const panOk = result.pan && result.pan.dropsPerSec <= 2.05;
  const zoomOk = result.zoom && result.zoom.dropsPerSec <= 2.05;
  const regionsOk =
    result.regions &&
    result.regions.WindowFrame &&
    result.regions.CanvasStage &&
    result.regions.WorldLayer &&
    result.regions.StatusZoom;
  app.exit(panOk && zoomOk && regionsOk ? 0 : 1);
});

setTimeout(() => {
  console.error("qa-frame-budget timeout");
  app.exit(2);
}, 30000);
