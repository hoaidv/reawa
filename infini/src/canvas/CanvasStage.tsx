/**
 * @implements [SRS-IN-02] CanvasStage — gestures + host surface
 * @implements [SRS-IN-01] paint host
 * @implements [SRS-IN-03] optional rAF frame counter
 *
 * Perf: coalesce paints to one rAF; no React setState on the gesture hot path
 * (ml-mindmap-style — we previously re-rendered React every wheel tick → &lt;30 fps).
 */

import { useEffect, useRef, useState } from "react";
import { CanvasRenderer } from "./CanvasRenderer";
import { InfiniDocument } from "./Document";
import { demoPrimitives } from "./primitives";
import {
  identityViewport,
  panByScreenDelta,
  preserveCenterOnResize,
  zoomAtScreenPoint,
  type Viewport,
} from "./Viewport";
import { allowIndividualInteraction } from "./TileCache";

export interface CanvasStageProps {
  populated?: boolean;
}

/** Place world (0,0) at the window center. */
function viewportCentered(cssW: number, cssH: number, scale = 1): Viewport {
  return {
    scale,
    translate: { x: cssW / (2 * scale), y: cssH / (2 * scale) },
  };
}

function get2dContext(canvas: HTMLCanvasElement): CanvasRenderingContext2D | null {
  // Avoid desynchronized: it can fail to present on some Electron/macOS GPUs.
  return canvas.getContext("2d", { alpha: false }) ?? canvas.getContext("2d");
}

export function CanvasStage({ populated = true }: CanvasStageProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const hostRef = useRef<HTMLDivElement>(null);
  const zoomElRef = useRef<HTMLDivElement>(null);
  const statsElRef = useRef<HTMLSpanElement>(null);
  const docRef = useRef(new InfiniDocument());
  const rendererRef = useRef(new CanvasRenderer());
  const vpRef = useRef<Viewport>(identityViewport());
  const sizeRef = useRef({ w: 0, h: 0 });
  const centeredRef = useRef(false);
  const dragRef = useRef<{ x: number; y: number } | null>(null);
  const rafPaintRef = useRef(0);
  const rafStats = useRef({ frames: 0, drops: 0, last: 0, painted: 0 });
  const gestureEndTimer = useRef(0);
  const ctxRef = useRef<CanvasRenderingContext2D | null>(null);

  const [emptyHint, setEmptyHint] = useState(!populated);

  const paintNow = () => {
    const canvas = canvasRef.current;
    const host = hostRef.current;
    if (!canvas || !host) return;

    const cssW = Math.max(1, sizeRef.current.w || Math.floor(host.clientWidth));
    const cssH = Math.max(1, sizeRef.current.h || Math.floor(host.clientHeight));
    if (!centeredRef.current && cssW > 1 && cssH > 1) {
      vpRef.current = viewportCentered(cssW, cssH, 1);
      centeredRef.current = true;
    }

    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    const bw = Math.floor(cssW * dpr);
    const bh = Math.floor(cssH * dpr);
    if (canvas.width !== bw || canvas.height !== bh) {
      canvas.width = bw;
      canvas.height = bh;
      canvas.style.width = `${cssW}px`;
      canvas.style.height = `${cssH}px`;
      ctxRef.current = null;
    }
    if (!ctxRef.current) {
      ctxRef.current = get2dContext(canvas);
    }
    const ctx = ctxRef.current;
    if (!ctx) {
      if (statsElRef.current) statsElRef.current.textContent = "canvas context unavailable";
      return;
    }

    const stats = rendererRef.current.paint(
      ctx,
      cssW,
      cssH,
      dpr,
      vpRef.current,
      docRef.current,
    );
    rafStats.current.painted++;

    const pct = Math.round(vpRef.current.scale * 100);
    if (zoomElRef.current) zoomElRef.current.textContent = `${pct}%`;
    if (statsElRef.current) {
      statsElRef.current.textContent =
        `${stats.indexMode} · ${stats.candidates} visible` +
        (stats.usedTileLod ? " · tile-LOD" : "") +
        (allowIndividualInteraction(vpRef.current) ? "" : " · no pick");
    }
  };

  const schedulePaint = () => {
    if (rafPaintRef.current) return;
    rafPaintRef.current = requestAnimationFrame(() => {
      rafPaintRef.current = 0;
      paintNow();
    });
  };

  const cancelScheduledPaint = () => {
    if (!rafPaintRef.current) return;
    cancelAnimationFrame(rafPaintRef.current);
    // Must clear: React Strict Mode cancels the first rAF on remount; if the
    // id is left truthy, schedulePaint() no-ops forever (blank canvas).
    rafPaintRef.current = 0;
  };

  const markGesturing = (on: boolean) => {
    const host = hostRef.current;
    if (!host) return;
    host.classList.toggle("is-gesturing", on);
  };

  const bumpGestureEnd = () => {
    markGesturing(true);
    window.clearTimeout(gestureEndTimer.current);
    gestureEndTimer.current = window.setTimeout(() => markGesturing(false), 100);
  };

  // Document seed
  useEffect(() => {
    if (populated) {
      docRef.current.setPrimitives(demoPrimitives());
      setEmptyHint(false);
    } else {
      docRef.current.clear();
      setEmptyHint(true);
    }
    rendererRef.current.invalidateTiles();
    schedulePaint();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [populated]);

  // Size + resize
  useEffect(() => {
    const host = hostRef.current;
    if (!host) return;
    const syncSize = () => {
      const nw = Math.max(1, Math.floor(host.clientWidth));
      const nh = Math.max(1, Math.floor(host.clientHeight));
      const { w: ow, h: oh } = sizeRef.current;
      if (ow > 0 && oh > 0 && (ow !== nw || oh !== nh)) {
        vpRef.current = preserveCenterOnResize(vpRef.current, ow, oh, nw, nh);
      }
      sizeRef.current = { w: nw, h: nh };
      schedulePaint();
    };
    syncSize();
    const ro = new ResizeObserver(syncSize);
    ro.observe(host);
    return () => ro.disconnect();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // Native wheel (non-passive) — single handler for pan/zoom + preventDefault
  useEffect(() => {
    const host = hostRef.current;
    if (!host) return;

    const onWheel = (e: WheelEvent) => {
      e.preventDefault();
      const rect = host.getBoundingClientRect();
      const sx = e.clientX - rect.left;
      const sy = e.clientY - rect.top;
      bumpGestureEnd();

      if (e.ctrlKey || e.metaKey) {
        // Stepped factors (ml-mindmap WheelLayer) — continuous 0.0015 felt too slow.
        const zoomFactorDelta =
          Math.abs(e.deltaY) > 10 ? 0.1 : Math.abs(e.deltaY) > 5 ? 0.05 : 0.01;
        const factor = e.deltaY > 0 ? 1 - zoomFactorDelta : 1 + zoomFactorDelta;
        vpRef.current = zoomAtScreenPoint(
          vpRef.current,
          { x: sx, y: sy },
          vpRef.current.scale * factor,
        );
      } else {
        // deltaMode: 0=pixel, 1=line, 2=page
        const mul = e.deltaMode === 1 ? 16 : e.deltaMode === 2 ? 32 : 1;
        vpRef.current = panByScreenDelta(
          vpRef.current,
          -e.deltaX * mul,
          -e.deltaY * mul,
        );
      }
      schedulePaint();
    };

    host.addEventListener("wheel", onWheel, { passive: false });
    return () => host.removeEventListener("wheel", onWheel);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // Frame counter (?trace=1)
  useEffect(() => {
    const params = new URLSearchParams(window.location.search);
    if (params.get("trace") !== "1") return;
    let id = 0;
    const loop = (t: number) => {
      const last = rafStats.current.last;
      if (last > 0) {
        const dt = t - last;
        rafStats.current.frames++;
        if (dt > 1000 / 60 + 4) rafStats.current.drops++;
      }
      rafStats.current.last = t;
      id = requestAnimationFrame(loop);
    };
    id = requestAnimationFrame(loop);
    const dump = () => {
      const { frames, drops, painted } = rafStats.current;
      console.info(`[infini] frames=${frames} drops≈${drops} paints=${painted}`);
    };
    window.addEventListener("beforeunload", dump);
    return () => {
      cancelAnimationFrame(id);
      window.removeEventListener("beforeunload", dump);
      dump();
    };
  }, []);

  useEffect(() => {
    return () => {
      cancelScheduledPaint();
      window.clearTimeout(gestureEndTimer.current);
    };
  }, []);

  const onPointerDown = (e: React.PointerEvent) => {
    if (e.button !== 0) return;
    (e.currentTarget as HTMLElement).setPointerCapture?.(e.pointerId);
    dragRef.current = { x: e.clientX, y: e.clientY };
    markGesturing(true);
  };

  const onPointerMove = (e: React.PointerEvent) => {
    const drag = dragRef.current;
    if (!drag) return;
    const dx = e.clientX - drag.x;
    const dy = e.clientY - drag.y;
    dragRef.current = { x: e.clientX, y: e.clientY };
    vpRef.current = panByScreenDelta(vpRef.current, dx, dy);
    schedulePaint();
  };

  const onPointerUp = () => {
    dragRef.current = null;
    markGesturing(false);
  };

  return (
    <div
      ref={hostRef}
      data-region="CanvasStage"
      data-platform="desktop"
      className="c-canvas-stage"
      tabIndex={0}
      role="application"
      aria-label="Infinity canvas"
      onPointerDown={onPointerDown}
      onPointerMove={onPointerMove}
      onPointerUp={onPointerUp}
      onPointerCancel={onPointerUp}
    >
      <canvas ref={canvasRef} data-region="WorldLayer" />
      <div
        ref={zoomElRef}
        className="c-zoom-readout"
        data-region="StatusZoom"
        aria-live="polite"
      >
        100%
      </div>
      <div className="app-mark" aria-hidden="true">
        Infini
      </div>
      {emptyHint && (
        <p className="empty-hint">Pan and zoom — trackpad, drag, or wheel</p>
      )}
      <p className="gesture-legend">
        <strong>Gestures</strong>
        Trackpad pan · Mouse drag pan · Wheel pan · ⌘/Ctrl+wheel zoom · Pinch
        <br />
        <span ref={statsElRef} />
      </p>
    </div>
  );
}
