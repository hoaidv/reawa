/**
 * @implements [SRS-IN-02] CanvasStage — gestures + host surface
 * @implements [SRS-IN-01] paint host
 * @implements [SRS-IN-03] optional rAF frame counter
 * @implements [SRS-IN-07] tablet drawing-region marker + viewport + doc_change applier
 * @implements [SRS-IN-04] WorldLayer from VectorDocument mirror
 * @implements [SRS-IN-14] no desktop ToolStrip / SelectionOverlay (deprecated)
 * @implements [SRS-IN-26] follower local-nav turns follow off
 * @implements [SRS-IN-20] apply inbound tablet viewport to WorldLayer
 * @implements [SRS-IN-27] FollowToggle is WindowFrame chrome, not WorldLayer
 *
 * Perf: coalesce paints to one rAF; no React setState on the gesture hot path.
 * STORY-IN-031: Infini is a review/mirror window — pan/zoom only; no ink-box chrome.
 */

import { useEffect, useRef, useState } from "react";
import { CanvasRenderer } from "./CanvasRenderer";
import { FollowToggle } from "./FollowToggle";
import { InfiniDocument } from "./Document";
import { demoPrimitives } from "./primitives";
import {
  frameWorldAabb,
  identityViewport,
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
import type { RmInboundMsg } from "../native";
import { IpcRmTransport, MemoryTransport, TabletSession } from "../session";
import { rmClientSyncHint, type RmClientEvent } from "../session/rmClientSync";
import { VectorDocument } from "../document";
import type { DocChangeMessage, HelloMessage, ViewportFollowMessage, ViewportMessage } from "../session";
import { followToggleView, type FollowToggleView } from "../session/viewportFollow";

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

export function CanvasStage({ populated = false }: CanvasStageProps) {
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

  const [emptyHint, setEmptyHint] = useState(!populated);
  const [syncHint, setSyncHint] = useState("");
  const [orientation, setOrientation] = useState<TabletOrientation>("gutToLeft");
  const [followView, setFollowView] = useState<FollowToggleView>(() =>
    followToggleView({ connected: false, direction: "none", offKind: "default" }),
  );

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
  }

  const currentOrientation = (): TabletOrientation =>
    sessionRef.current?.getOrientation() ?? orientationRef.current;

  const syncFollowUi = () => {
    const view = sessionRef.current?.followView();
    if (view) setFollowView(view);
  };

  /**
   * Follower apply — WorldLayer camera from inbound tablet pose.
   * @implements [SRS-IN-20] apply translate + uniform scale
   */
  const applyFollowViewport = (vp: Viewport) => {
    vpRef.current = { translate: { x: vp.translate.x, y: vp.translate.y }, scale: vp.scale };
    schedulePaint();
  };

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
   * Viewport only — Infini does not push documents mid-session (IN-028 owns doc_load).
   * @implements [SRS-IN-07] 0 outbound document ops
   */
  const publishViewportCoalesced = (force = false) => {
    const session = sessionRef.current;
    if (!session) return;
    session.setCssSize(sizeRef.current.w || 800, sizeRef.current.h || 600);
    session.setOrientation(orientationRef.current);
    if (force) session.flushViewport(vpRef.current);
    else session.publishViewport(vpRef.current);
  };

  /**
   * Paint WorldLayer from the VectorDocument mirror + transient previews.
   * Withdrawn: rebuildWithRmInk as the document source (flat RM primitives).
   * @implements [SRS-IN-07] WorldLayer paints the mirror
   */
  const paintMirror = () => {
    const session = sessionRef.current;
    session?.paintMirror();
    // @fix [STORY-IN-032] never re-seed demoPrimitives onto the live mirror
    if (docRef.current.size) setEmptyHint(false);
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
      // Final settle flush — clears soft-refresh ghosting on the tablet.
      publishViewportCoalesced(true);
      markGesturing(false);
    }, 120);
  };

  const cycleOrientation = () => {
    const i = TABLET_ORIENTATIONS.indexOf(orientationRef.current);
    const next = TABLET_ORIENTATIONS[(i + 1) % TABLET_ORIENTATIONS.length];
    orientationRef.current = next;
    setOrientation(next);
    sessionRef.current?.setOrientation(next);
    sessionRef.current?.noteInfiniSideAction();
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

    const handleRmClient = (ev: RmClientEvent) => {
      const session = sessionRef.current;
      if (session) {
        if (ev.n > 0) {
          if (!session.connected) session.connect();
        } else if (session.connected) {
          session.disconnect();
        }
        syncFollowUi();
      }
      if (
        (ev.type === "connected" || ev.type === "sync") &&
        ev.n > 0 &&
        session?.followDirection === "infini_to_epaper"
      ) {
        publishViewportCoalesced(true);
      }
      setSyncHint(rmClientSyncHint(ev));
    };

    // Subscribe before async rmClientCount — avoids missing connect during mount race (IN-019).
    const unsubClient = api.onRmClient?.(handleRmClient) ?? (() => {});

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
      syncFollowUi();
    }
    void api.strokeIngestPort?.().then((p) => {
      setSyncHint((prev) => (prev.startsWith("RM connected") ? prev : `RM sync :${p}`));
    });
    void api.rmClientCount?.().then((n) => {
      handleRmClient({ type: "sync", n });
    });

    const unsub = api.onRmStroke((msg: RmInboundMsg) => {
      if (msg.type === "hello") {
        sessionRef.current?.receiveHello(msg as HelloMessage);
        return;
      }
      if (msg.type === "viewport_follow") {
        sessionRef.current?.receiveViewportFollow(msg as ViewportFollowMessage);
        syncFollowUi();
        return;
      }
      if (msg.type === "viewport") {
        const applied = sessionRef.current?.receiveTabletViewport(msg as ViewportMessage);
        if (applied?.applied && applied.viewport) applyFollowViewport(applied.viewport);
        return;
      }
      if (msg.type === "queue_empty") {
        sessionRef.current?.receiveQueueEmpty();
        return;
      }
      if (msg.type === "load_ack") {
        sessionRef.current?.receiveLoadAck();
        return;
      }
      if (msg.type === "doc_change") {
        sessionRef.current?.receiveDocChange(msg as DocChangeMessage);
        paintMirror();
        return;
      }
      if (msg.type === "manip_preview") {
        sessionRef.current?.applyManipPreview(msg.id, msg.transform, msg.bounds);
        paintMirror();
        return;
      }
      if (msg.type === "tool_intent") {
        // Retired with SRS-IN-13 — Infini is not the document authority.
        return;
      }
      const session = sessionRef.current;

      if (msg.type === "stroke_begin") {
        const widthWorld = msg.brush?.width ?? 2.5;
        session?.previewBegin(msg.id, widthWorld);
        return;
      }
      if (msg.type === "stroke_point") {
        // @implements [SRS-IN-07] live preview is world — same space as append_ink
        session?.previewPoint(msg.id, msg.x, msg.y);
        paintMirror();
        return;
      }
      if (msg.type === "stroke_end") {
        session?.previewEnd(msg.id);
        paintMirror();
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
      // Local pan/pinch: none **before** the gesture applies ([SRS-IN-20] / [SRS-IN-26]).
      if (sessionRef.current?.noteFollowerLocalNav()) syncFollowUi();
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
    // Local pan: none **before** the drag applies ([SRS-IN-20] / [SRS-IN-26]).
    if (sessionRef.current?.noteFollowerLocalNav()) syncFollowUi();
    dragRef.current = { x: e.clientX, y: e.clientY };
    markGesturing(true);
    schedulePaint();
  };

  const onPointerMove = (e: React.PointerEvent) => {
    const drag = dragRef.current;
    if (!drag) return;
    const clientDx = e.clientX - drag.x;
    const clientDy = e.clientY - drag.y;
    dragRef.current = { x: e.clientX, y: e.clientY };
    vpRef.current = panByScreenDelta(vpRef.current, clientDx, clientDy);
    publishViewportCoalesced(false);
    schedulePaint();
  };

  const onPointerUp = () => {
    dragRef.current = null;
    publishViewportCoalesced(true);
    markGesturing(false);
    schedulePaint();
  };

  const onFollowToggle = () => {
    const result = sessionRef.current?.clickFollowToggle();
    if (!result) return;
    if (result.applied) applyFollowViewport(result.applied);
    syncFollowUi();
  };

  return (
    <>
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
    <div className="c-chrome-trailing">
      <FollowToggle view={followView} onToggle={onFollowToggle} />
      <div
        ref={zoomElRef}
        className="c-zoom-readout"
        data-region="StatusZoom"
        aria-live="polite"
      >
        100%
      </div>
    </div>
    </>
  );
}
