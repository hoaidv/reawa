/**
 * @implements [SRS-IN-02] CanvasStage — gestures + host surface
 * @implements [SRS-IN-01] paint host
 * @implements [SRS-IN-03] optional rAF frame counter
 * @implements [SRS-IN-07] tablet drawing-region marker + viewport + region refresh
 *
 * Perf: coalesce paints to one rAF; no React setState on the gesture hot path.
 */

import { useEffect, useRef, useState } from "react";
import { CanvasRenderer } from "./CanvasRenderer";
import { InfiniDocument } from "./Document";
import { demoPrimitives, makePath } from "./primitives";
import {
  frameWorldAabb,
  identityViewport,
  panelToFrameUv,
  panByScreenDelta,
  preserveCenterOnResize,
  TABLET_ORIENTATIONS,
  tabletDrawingFrameCss,
  tabletOrientationLabel,
  tabletOrientationMeta,
  zoomAtScreenPoint,
  type TabletOrientation,
  type Viewport,
} from "./Viewport";
import { allowIndividualInteraction } from "./TileCache";
import type { RmStrokeMsg } from "../native";
import { IpcRmTransport, MemoryTransport, TabletSession } from "../session";
import { VectorDocument } from "../document";
import type { Primitive } from "./primitives";

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
  return canvas.getContext("2d", { alpha: false }) ?? canvas.getContext("2d");
}

/** Serialize WorldLayer primitives for Epaper vector rasterize (not a bitmap). */
function primitivesToSnapshotNodes(prims: Primitive[]): Record<string, unknown>[] {
  return prims.map((p) => {
    const base = { id: p.id, strokeWidth: p.style.strokeWidth };
    switch (p.kind) {
      case "line":
        return { ...base, kind: "line", x1: p.x1, y1: p.y1, x2: p.x2, y2: p.y2 };
      case "rect":
        return { ...base, kind: "rect", x: p.x, y: p.y, w: p.w, h: p.h };
      case "ellipse":
        return { ...base, kind: "ellipse", cx: p.cx, cy: p.cy, rx: p.rx, ry: p.ry };
      case "path":
        return { ...base, kind: "path", points: p.points };
    }
  });
}

export function CanvasStage({ populated = true }: CanvasStageProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const hostRef = useRef<HTMLDivElement>(null);
  const zoomElRef = useRef<HTMLDivElement>(null);
  const statsElRef = useRef<HTMLSpanElement>(null);
  const markerRef = useRef<HTMLDivElement>(null);
  const docRef = useRef(new InfiniDocument());
  const treeRef = useRef(new VectorDocument());
  const transportRef = useRef(
    typeof window !== "undefined" && window.infiniNative?.sendToRm
      ? new IpcRmTransport()
      : new MemoryTransport(),
  );
  const sessionRef = useRef<TabletSession | null>(null);
  const rendererRef = useRef(new CanvasRenderer());
  const vpRef = useRef<Viewport>(identityViewport());
  const sizeRef = useRef({ w: 0, h: 0 });
  const centeredRef = useRef(false);
  const dragRef = useRef<{ x: number; y: number } | null>(null);
  const rafPaintRef = useRef(0);
  const rafStats = useRef({ frames: 0, drops: 0, last: 0, painted: 0 });
  const gestureEndTimer = useRef(0);
  const gesturingRef = useRef(false);
  const ctxRef = useRef<CanvasRenderingContext2D | null>(null);
  const orientationRef = useRef<TabletOrientation>("gutToLeft");
  const snapshotSentRef = useRef(false);
  const rmStrokesRef = useRef(
    new Map<string, { points: { x: number; y: number }[]; width: number }>(),
  );
  const rmDoneRef = useRef<
    { id: string; points: { x: number; y: number }[]; width: number }[]
  >([]);
  const rmPanelRef = useRef({ w: 1404, h: 1872 });

  const [emptyHint, setEmptyHint] = useState(!populated);
  const [syncHint, setSyncHint] = useState("");
  const [orientation, setOrientation] = useState<TabletOrientation>("gutToLeft");

  if (!sessionRef.current) {
    // Prefer IPC when preload already exposed sendToRm (Electron).
    if (window.infiniNative?.sendToRm) {
      transportRef.current = new IpcRmTransport();
    }
    sessionRef.current = new TabletSession({
      tree: treeRef.current,
      world: docRef.current,
      transport: transportRef.current,
      cssWidth: 800,
      cssHeight: 600,
      orientation: "gutToLeft",
    });
    sessionRef.current.connect();
  }

  const currentOrientation = (): TabletOrientation =>
    sessionRef.current?.getOrientation() ?? orientationRef.current;

  const syncMarkerDom = () => {
    const marker = markerRef.current;
    const host = hostRef.current;
    if (!marker || !host) return;
    const cssW = Math.max(1, sizeRef.current.w || Math.floor(host.clientWidth));
    const cssH = Math.max(1, sizeRef.current.h || Math.floor(host.clientHeight));
    const frame = tabletDrawingFrameCss(cssW, cssH, currentOrientation());
    marker.style.left = `${frame.x}px`;
    marker.style.top = `${frame.y}px`;
    marker.style.width = `${frame.w}px`;
    marker.style.height = `${frame.h}px`;
    marker.classList.toggle("is-visible", gesturingRef.current);
    marker.setAttribute("aria-hidden", gesturingRef.current ? "false" : "true");
  };

  /**
   * One-shot (or rare) vector document push to Epaper — not a bitmap.
   * Subsequent pan/zoom only sends viewport; tablet re-rasterizes locally.
   */
  const sendDocSnapshot = () => {
    const api = window.infiniNative;
    if (!api?.sendToRm) return;
    const nodes = primitivesToSnapshotNodes(docRef.current.all());
    void api.sendToRm({ type: "doc_snapshot", nodes });
    snapshotSentRef.current = true;
  };

  const publishViewportCoalesced = (force = false) => {
    const session = sessionRef.current;
    if (!session) return;
    session.setCssSize(sizeRef.current.w || 800, sizeRef.current.h || 600);
    session.setOrientation(orientationRef.current);
    if (!snapshotSentRef.current) sendDocSnapshot();
    if (force) session.flushViewport(vpRef.current);
    else session.publishViewport(vpRef.current);
  };

  const rebuildWithRmInk = () => {
    const base = populated ? demoPrimitives() : [];
    const inkStyle = { stroke: "#1C2430", strokeWidth: 2.5 };
    const rm = [
      ...rmDoneRef.current.map((s) =>
        makePath(`rm-done-${s.id}`, s.points, {
          ...inkStyle,
          strokeWidth: s.width,
        }),
      ),
      ...[...rmStrokesRef.current.entries()]
        .filter(([, s]) => s.points.length >= 2)
        .map(([id, s]) =>
          makePath(`rm-live-${id}`, s.points, {
            ...inkStyle,
            strokeWidth: s.width,
          }),
        ),
    ];
    docRef.current.setPrimitives([...base, ...rm]);
    if (rm.length) setEmptyHint(false);
    rendererRef.current.invalidateTiles();
    schedulePaint();
  };

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
    syncMarkerDom();

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
    rafPaintRef.current = 0;
  };

  const markGesturing = (on: boolean) => {
    const host = hostRef.current;
    if (!host) return;
    gesturingRef.current = on;
    host.classList.toggle("is-gesturing", on);
    syncMarkerDom();
  };

  const bumpGestureEnd = () => {
    markGesturing(true);
    publishViewportCoalesced(false);
    window.clearTimeout(gestureEndTimer.current);
    gestureEndTimer.current = window.setTimeout(() => {
      publishViewportCoalesced(true);
      markGesturing(false);
    }, 100);
  };

  const cycleOrientation = () => {
    const i = TABLET_ORIENTATIONS.indexOf(orientationRef.current);
    const next = TABLET_ORIENTATIONS[(i + 1) % TABLET_ORIENTATIONS.length];
    orientationRef.current = next;
    setOrientation(next);
    sessionRef.current?.setOrientation(next);
    snapshotSentRef.current = false;
    sendDocSnapshot();
    syncMarkerDom();
    publishViewportCoalesced(true);
    schedulePaint();
  };

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

  useEffect(() => {
    const api = window.infiniNative;
    if (!api?.onRmStroke) {
      setSyncHint("no native bridge (browser-only?)");
      return;
    }
    // Upgrade transport if preload arrived after first render
    if (api.sendToRm && !(transportRef.current instanceof IpcRmTransport)) {
      transportRef.current = new IpcRmTransport();
      sessionRef.current = new TabletSession({
        tree: treeRef.current,
        world: docRef.current,
        transport: transportRef.current,
        cssWidth: sizeRef.current.w || 800,
        cssHeight: sizeRef.current.h || 600,
        orientation: orientationRef.current,
      });
      sessionRef.current.connect();
    }
    void api.strokeIngestPort?.().then((p) => {
      setSyncHint(`RM sync :${p}`);
    });
    void api.rmClientCount?.().then((n) => {
      if (n > 0) {
        snapshotSentRef.current = false;
        sendDocSnapshot();
        publishViewportCoalesced(true);
        setSyncHint(`RM connected (n=${n})`);
      }
    });

    const unsubClient = api.onRmClient?.((ev) => {
      if (ev.type === "connected") {
        snapshotSentRef.current = false;
        sendDocSnapshot();
        publishViewportCoalesced(true);
        setSyncHint(`RM connected (n=${ev.n})`);
      }
    });

    const unsub = api.onRmStroke((msg: RmStrokeMsg) => {
      const cssW = sizeRef.current.w || 800;
      const cssH = sizeRef.current.h || 600;
      const orient = currentOrientation();
      const frame = tabletDrawingFrameCss(cssW, cssH, orient);
      const region = frameWorldAabb(frame, vpRef.current);

      if (msg.type === "stroke_begin") {
        if (msg.cw && msg.ch) rmPanelRef.current = { w: msg.cw, h: msg.ch };
        // brush.width is world units (ADR-0012); legacy panel-px ignored.
        const widthWorld = msg.brush?.width ?? 2.5;
        rmStrokesRef.current.set(msg.id, { points: [], width: widthWorld });
        return;
      }
      if (msg.type === "stroke_point") {
        const stroke = rmStrokesRef.current.get(msg.id);
        if (!stroke) return;
        const { u, v } = panelToFrameUv(
          msg.x,
          msg.y,
          rmPanelRef.current.w,
          rmPanelRef.current.h,
          orient,
        );
        stroke.points.push({
          x: region.minX + u * (region.maxX - region.minX),
          y: region.minY + v * (region.maxY - region.minY),
        });
        if (stroke.points.length === 2 || stroke.points.length % 3 === 0) {
          rebuildWithRmInk();
        }
        return;
      }
      if (msg.type === "stroke_end") {
        const stroke = rmStrokesRef.current.get(msg.id);
        if (stroke && stroke.points.length >= 2) {
          rmDoneRef.current.push({ id: msg.id, ...stroke });
        }
        rmStrokesRef.current.delete(msg.id);
        rebuildWithRmInk();
      }
    });
    return () => {
      unsub();
      unsubClient?.();
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [populated]);

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
      sessionRef.current?.setCssSize(nw, nh);
      schedulePaint();
    };
    syncSize();
    const ro = new ResizeObserver(syncSize);
    ro.observe(host);
    return () => ro.disconnect();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

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
        const zoomFactorDelta =
          Math.abs(e.deltaY) > 10 ? 0.1 : Math.abs(e.deltaY) > 5 ? 0.05 : 0.01;
        const factor = e.deltaY > 0 ? 1 - zoomFactorDelta : 1 + zoomFactorDelta;
        vpRef.current = zoomAtScreenPoint(
          vpRef.current,
          { x: sx, y: sy },
          vpRef.current.scale * factor,
        );
      } else {
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
    publishViewportCoalesced(false);
    schedulePaint();
  };

  const onPointerUp = () => {
    dragRef.current = null;
    publishViewportCoalesced(true);
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
        ref={markerRef}
        className="c-tablet-region-marker"
        data-region="TabletDrawingRegionMarker"
        aria-hidden="true"
      />
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
      <button
        type="button"
        className="c-orient-toggle"
        data-region="TabletOrientationToggle"
        onClick={(e) => {
          e.stopPropagation();
          cycleOrientation();
        }}
        onPointerDown={(e) => e.stopPropagation()}
      >
        Sync: {tabletOrientationLabel(orientation)}
        {tabletOrientationMeta(orientation).landscape ? " · wide" : " · tall"}
      </button>
      {emptyHint && (
        <p className="empty-hint">Pan and zoom — trackpad, drag, or wheel</p>
      )}
      <p className="gesture-legend">
        <strong>Gestures</strong>
        Trackpad pan · Mouse drag pan · Wheel pan · ⌘/Ctrl+wheel zoom · Pinch
        <br />
        <span ref={statsElRef} />
        {syncHint ? (
          <>
            <br />
            <span data-region="SyncStatus">{syncHint}</span>
          </>
        ) : null}
      </p>
    </div>
  );
}
