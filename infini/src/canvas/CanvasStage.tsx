/**
 * @implements [SRS-IN-02] CanvasStage — gestures + host surface
 * @implements [SRS-IN-01] paint host
 * @implements [SRS-IN-03] optional rAF frame counter (RM_INK_TRACE-style)
 *
 * Gestures learned from ml-mindmap WheelLayer (wheel pan, ctrl/meta+wheel zoom)
 * plus mouse-drag pan and center-preserving resize from Infini SRS.
 */

import { useCallback, useEffect, useRef, useState } from "react";
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
  /** When false, empty canvas (canvas.empty). */
  populated?: boolean;
}

export function CanvasStage({ populated = true }: CanvasStageProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const hostRef = useRef<HTMLDivElement>(null);
  const docRef = useRef(new InfiniDocument());
  const rendererRef = useRef(new CanvasRenderer());
  const vpRef = useRef<Viewport>(identityViewport());
  const sizeRef = useRef({ w: 0, h: 0 });
  const gesturingRef = useRef(false);
  const dragRef = useRef<{ x: number; y: number } | null>(null);
  const rafStats = useRef({ frames: 0, drops: 0, last: 0 });

  const [zoomPct, setZoomPct] = useState(100);
  const [gesturing, setGesturing] = useState(false);
  const [emptyHint, setEmptyHint] = useState(!populated);
  const [statsLabel, setStatsLabel] = useState("");

  const paint = useCallback(() => {
    const canvas = canvasRef.current;
    const host = hostRef.current;
    if (!canvas || !host) return;
    const rect = host.getBoundingClientRect();
    const cssW = Math.max(1, Math.floor(rect.width));
    const cssH = Math.max(1, Math.floor(rect.height));
    const dpr = window.devicePixelRatio || 1;
    if (canvas.width !== Math.floor(cssW * dpr) || canvas.height !== Math.floor(cssH * dpr)) {
      canvas.width = Math.floor(cssW * dpr);
      canvas.height = Math.floor(cssH * dpr);
      canvas.style.width = `${cssW}px`;
      canvas.style.height = `${cssH}px`;
    }
    sizeRef.current = { w: cssW, h: cssH };
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    const stats = rendererRef.current.paint(
      ctx,
      cssW,
      cssH,
      dpr,
      vpRef.current,
      docRef.current,
    );
    setZoomPct(Math.round(vpRef.current.scale * 100));
    setStatsLabel(
      `${stats.indexMode} · ${stats.candidates} visible` +
        (stats.usedTileLod ? " · tile-LOD" : "") +
        (allowIndividualInteraction(vpRef.current) ? "" : " · no pick"),
    );
  }, []);

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
    paint();
  }, [populated, paint]);

  // ResizeObserver — preserve world under center
  useEffect(() => {
    const host = hostRef.current;
    if (!host) return;
    const ro = new ResizeObserver(() => {
      const { w: ow, h: oh } = sizeRef.current;
      const rect = host.getBoundingClientRect();
      const nw = Math.max(1, Math.floor(rect.width));
      const nh = Math.max(1, Math.floor(rect.height));
      if (ow > 0 && oh > 0 && (ow !== nw || oh !== nh)) {
        vpRef.current = preserveCenterOnResize(vpRef.current, ow, oh, nw, nh);
      }
      paint();
    });
    ro.observe(host);
    paint();
    return () => ro.disconnect();
  }, [paint]);

  // Non-passive wheel — stop browser page zoom (ml-mindmap WheelLayer lesson).
  useEffect(() => {
    const host = hostRef.current;
    if (!host) return;
    const block = (e: WheelEvent) => e.preventDefault();
    host.addEventListener("wheel", block, { passive: false });
    return () => host.removeEventListener("wheel", block);
  }, []);

  // Frame counter for STORY-IN-005 evidence (enable with ?trace=1).
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
      const { frames, drops } = rafStats.current;
      // eslint-disable-next-line no-console
      console.info(`[infini] frame-trace frames=${frames} drops≈${drops}`);
    };
    window.addEventListener("beforeunload", dump);
    return () => {
      cancelAnimationFrame(id);
      window.removeEventListener("beforeunload", dump);
      dump();
    };
  }, []);

  const setGesturingBoth = (v: boolean) => {
    gesturingRef.current = v;
    setGesturing(v);
  };

  const onWheel = (e: React.WheelEvent) => {
    e.preventDefault();
    const host = hostRef.current;
    if (!host) return;
    const rect = host.getBoundingClientRect();
    const sx = e.clientX - rect.left;
    const sy = e.clientY - rect.top;
    setGesturingBoth(true);
    if (e.ctrlKey || e.metaKey) {
      // Trackpad pinch arrives as ctrl+wheel on Chromium (ml-mindmap lesson).
      const delta =
        Math.abs(e.deltaY) > 10 ? 0.1 : Math.abs(e.deltaY) > 5 ? 0.05 : 0.01;
      const factor = e.deltaY > 0 ? 1 - delta : 1 + delta;
      vpRef.current = zoomAtScreenPoint(
        vpRef.current,
        { x: sx, y: sy },
        vpRef.current.scale * factor,
      );
    } else {
      vpRef.current = panByScreenDelta(vpRef.current, -e.deltaX, -e.deltaY);
    }
    paint();
    window.setTimeout(() => setGesturingBoth(false), 120);
  };

  const onPointerDown = (e: React.PointerEvent) => {
    if (e.button !== 0) return;
    (e.target as HTMLElement).setPointerCapture?.(e.pointerId);
    dragRef.current = { x: e.clientX, y: e.clientY };
    setGesturingBoth(true);
  };

  const onPointerMove = (e: React.PointerEvent) => {
    const drag = dragRef.current;
    if (!drag) return;
    const dx = e.clientX - drag.x;
    const dy = e.clientY - drag.y;
    dragRef.current = { x: e.clientX, y: e.clientY };
    vpRef.current = panByScreenDelta(vpRef.current, dx, dy);
    paint();
  };

  const onPointerUp = () => {
    dragRef.current = null;
    setGesturingBoth(false);
  };

  return (
    <div
      ref={hostRef}
      data-region="CanvasStage"
      data-platform="desktop"
      className={`c-canvas-stage${gesturing ? " is-gesturing" : ""}`}
      tabIndex={0}
      role="application"
      aria-label="Infinity canvas"
      onWheel={onWheel}
      onPointerDown={onPointerDown}
      onPointerMove={onPointerMove}
      onPointerUp={onPointerUp}
      onPointerCancel={onPointerUp}
    >
      <canvas ref={canvasRef} data-region="WorldLayer" />
      <div className="c-zoom-readout" data-region="StatusZoom" aria-live="polite">
        {zoomPct}%
      </div>
      <div className="app-mark" aria-hidden="true">
        Infini
      </div>
      {emptyHint && (
        <p className="empty-hint">Pan and zoom — trackpad, drag, or wheel</p>
      )}
      <p className="gesture-legend">
        <strong>Gestures</strong>
        Trackpad pan · Mouse drag pan · Wheel pan · ⌘/Ctrl+wheel zoom · Pinch (ctrl+wheel)
        <br />
        {statsLabel}
      </p>
    </div>
  );
}
