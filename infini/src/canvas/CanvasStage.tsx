/**
 * @implements [SRS-IN-02] CanvasStage — gestures + host surface
 * @implements [SRS-IN-01] paint host
 * @implements [SRS-IN-03] optional rAF frame counter
 * @implements [SRS-IN-07] tablet drawing-region marker + viewport + region refresh
 * @implements [SRS-IN-04] tree-backed live ink ingestion on stroke_end
 * @implements [SRS-IN-10] enclose recognition when intent enclose
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
import { rmClientSyncHint, shouldPublishOnRmClient, type RmClientEvent } from "../session/rmClientSync";
import { ToolStrip } from "./ToolStrip";
import {
  createSelectionSession,
  handleSelectionPointer,
  pickFreeInkAt,
  selectionOverlayScreenRect,
  type SelectionSession,
} from "../document/selection";
import { screenToWorld } from "./Viewport";
import {
  VectorDocument,
  commitStrokeWithEncloseRecognition,
  createSmartGroupFromSelection,
  buildPickables,
  applyToolIntent,
  normalizeStrokeIntent,
  UndoRing,
} from "../document";
import type { SmartGroupNode } from "../document/types";
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
  const selectionRef = useRef<SelectionSession>(createSelectionSession());
  const undoRef = useRef(new UndoRing());
  const overlayRef = useRef<HTMLDivElement>(null);
  const rafPaintRef = useRef(0);
  const rafStats = useRef({ frames: 0, drops: 0, last: 0, painted: 0 });
  const gestureEndTimer = useRef(0);
  const gesturingRef = useRef(false);
  const ctxRef = useRef<CanvasRenderingContext2D | null>(null);
  const orientationRef = useRef<TabletOrientation>("gutToLeft");
  const snapshotSentRef = useRef(false);
  /** In-flight RM strokes only — committed ink lives in VectorDocument (STORY-IN-012). */
  const rmStrokesRef = useRef(
    new Map<
      string,
      {
        points: { x: number; y: number }[];
        width: number;
        intent?: "ink" | "enclose";
      }
    >(),
  );
  const rmPanelRef = useRef({ w: 1404, h: 1872 });

  const [emptyHint, setEmptyHint] = useState(!populated);
  const [syncHint, setSyncHint] = useState("");
  const [orientation, setOrientation] = useState<TabletOrientation>("gutToLeft");
  const [tool, setTool] = useState<"selection" | "ink_box">("selection");
  const [refuseReason, setRefuseReason] = useState<string | null>(null);
  const [inkSelCount, setInkSelCount] = useState(0);

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
    const nodes = primitivesToSnapshotNodes([...docRef.current.all()]);
    const pickables = buildPickables(treeRef.current);
    void api.sendToRm({ type: "doc_snapshot", nodes, pickables });
    snapshotSentRef.current = true;
  };

  /** Push fresh vector doc + pickables after tree mutations (STORY-IN-020). */
  const pushDocSnapshotToRm = () => {
    sendDocSnapshot();
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

  const syncSelectionOverlay = () => {
    const el = overlayRef.current;
    if (!el) return;
    const sel = selectionRef.current;
    const id = sel.selectedId;
    if (!id || !allowIndividualInteraction(vpRef.current)) {
      el.hidden = true;
      return;
    }
    let node = treeRef.current.indexById().get(id);
    if (!node || node.kind !== "smart_group") {
      el.hidden = true;
      return;
    }
    const sg = node as SmartGroupNode;
    if (sel.preview && sel.preview.id === sg.id) {
      node = {
        ...sg,
        transform: sel.preview.liveTransform,
        bounds: sel.preview.liveBounds,
      };
    }
    const rect = selectionOverlayScreenRect(node as SmartGroupNode, vpRef.current);
    el.hidden = false;
    el.style.left = `${rect.left}px`;
    el.style.top = `${rect.top}px`;
    el.style.width = `${rect.width}px`;
    el.style.height = `${rect.height}px`;
    el.dataset.state = sel.preview?.moved
      ? "tool.selection.dragging"
      : "tool.selection.selected";
  };

  /**
   * Paint WorldLayer from demo figures + VectorDocument + optional in-flight live strokes.
   * Demo figures are WorldLayer SoT ([SRS-IN-07]); tree ink must not replace them.
   * @implements [SRS-IN-04] tree-backed live ink — syncFromVectorDoc is SoT for committed ink
   * @fix [BUG] tablet ink must not wipe desktop demo figures
   */
  const rebuildWithRmInk = () => {
    const base = populated ? demoPrimitives() : [];
    docRef.current.syncFromVectorDoc(treeRef.current);
    const fromTree = [...docRef.current.all()];
    const inkStyle = { stroke: "#1C2430", strokeWidth: 2.5 };
    const live = [...rmStrokesRef.current.entries()]
      .filter(([, s]) => s.points.length >= 2)
      .map(([id, s]) =>
        makePath(`rm-live-${id}`, s.points, {
          ...inkStyle,
          strokeWidth: s.width,
        }),
      );
    // Always keep demo figures under tree ink + live (SRS-IN-07 WorldLayer SoT).
    const merged = [...base, ...fromTree, ...live];
    docRef.current.setPrimitives(merged);
    if (merged.length) setEmptyHint(false);
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
    syncSelectionOverlay();

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

    const handleRmClient = (ev: RmClientEvent) => {
      if (shouldPublishOnRmClient(ev)) {
        snapshotSentRef.current = false;
        sendDocSnapshot();
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
      sessionRef.current.connect();
    }
    void api.strokeIngestPort?.().then((p) => {
      setSyncHint((prev) => (prev.startsWith("RM connected") ? prev : `RM sync :${p}`));
    });
    void api.rmClientCount?.().then((n) => {
      handleRmClient({ type: "sync", n });
    });

    const unsub = api.onRmStroke((msg: RmStrokeMsg | import("../native").RmToolIntentMsg) => {
      if (msg.type === "tool_intent") {
        applyToolIntent(treeRef.current, undoRef.current, msg);
        rebuildWithRmInk();
        pushDocSnapshotToRm();
        return;
      }
      const cssW = sizeRef.current.w || 800;
      const cssH = sizeRef.current.h || 600;
      const orient = currentOrientation();
      const frame = tabletDrawingFrameCss(cssW, cssH, orient);
      const region = frameWorldAabb(frame, vpRef.current);

      if (msg.type === "stroke_begin") {
        if (msg.cw && msg.ch) rmPanelRef.current = { w: msg.cw, h: msg.ch };
        // brush.width is world units (ADR-0012); legacy panel-px ignored.
        const widthWorld = msg.brush?.width ?? 2.5;
        rmStrokesRef.current.set(msg.id, {
          points: [],
          width: widthWorld,
          intent: msg.intent === "enclose" ? "enclose" : "ink",
        });
        return;
      }
      if (msg.type === "stroke_point") {
        let stroke = rmStrokesRef.current.get(msg.id);
        // Recover if stroke_begin was dropped on the wire (intermittent miss).
        if (!stroke) {
          stroke = {
            points: [],
            width: 2.5,
            intent: "ink",
          };
          rmStrokesRef.current.set(msg.id, stroke);
        }
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
        // Live paint every sample — enclose boxes must stay visible while drawing.
        rebuildWithRmInk();
        return;
      }
      if (msg.type === "stroke_end") {
        const stroke = rmStrokesRef.current.get(msg.id);
        let encKind: string | undefined;
        if (stroke && stroke.points.length >= 2) {
          const encResult = commitStrokeWithEncloseRecognition(
            treeRef.current,
            undoRef.current,
            {
              id: msg.id,
              points: stroke.points,
              width: stroke.width,
              intent: normalizeStrokeIntent(stroke.intent),
              source: "epaper",
            },
          );
          encKind = encResult.kind;
        }
        rmStrokesRef.current.delete(msg.id);
        // Rebuild BEFORE snapshot so tablet receives flattened Smart Group, not stale free ink.
        rebuildWithRmInk();
        if (encKind === "created") {
          pushDocSnapshotToRm();
        }
        // Ordinary ink: no snapshot — device already drew locally (race avoidance).
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
    const host = hostRef.current;
    if (!host) return;
    const rect = host.getBoundingClientRect();
    const screen = { x: e.clientX - rect.left, y: e.clientY - rect.top };
    dragRef.current = { x: e.clientX, y: e.clientY };
    const result = handleSelectionPointer(
      treeRef.current,
      selectionRef.current,
      "down",
      screen,
      vpRef.current,
    );
    selectionRef.current = result.session;
    if (!result.consumed && result.session.tool === "selection") {
      const world = screenToWorld(screen, vpRef.current);
      const ink = pickFreeInkAt(treeRef.current, world);
      if (ink) {
        const ids = new Set(selectionRef.current.selectedInkIds);
        if (ids.has(ink.id)) ids.delete(ink.id);
        else ids.add(ink.id);
        selectionRef.current = {
          ...selectionRef.current,
          selectedInkIds: [...ids],
          selectedId: null,
        };
        setInkSelCount(ids.size);
        setRefuseReason(null);
      }
    }
    applyPreviewToTree();
    markGesturing(true);
    schedulePaint();
  };

  const applyPreviewToTree = () => {
    const prev = selectionRef.current.preview;
    if (!prev) return;
    const node = treeRef.current.indexById().get(prev.id);
    if (!node || node.kind !== "smart_group") return;
    node.transform = { ...prev.liveTransform };
    node.bounds = { ...prev.liveBounds };
    rebuildWithRmInk();
  };

  const restorePreviewOrigin = () => {
    const prev = selectionRef.current.preview;
    if (!prev) return;
    const node = treeRef.current.indexById().get(prev.id);
    if (!node || node.kind !== "smart_group") return;
    node.transform = { ...prev.originTransform };
    node.bounds = { ...prev.originBounds };
  };

  const onPointerMove = (e: React.PointerEvent) => {
    const drag = dragRef.current;
    if (!drag) return;
    const host = hostRef.current;
    if (!host) return;
    const rect = host.getBoundingClientRect();
    const screen = { x: e.clientX - rect.left, y: e.clientY - rect.top };
    const lastScreen = {
      x: drag.x - rect.left,
      y: drag.y - rect.top,
    };
    const clientDx = e.clientX - drag.x;
    const clientDy = e.clientY - drag.y;
    const result = handleSelectionPointer(
      treeRef.current,
      selectionRef.current,
      "move",
      screen,
      vpRef.current,
      lastScreen,
    );
    selectionRef.current = result.session;
    dragRef.current = { x: e.clientX, y: e.clientY };

    if (result.consumed && result.session.preview) {
      applyPreviewToTree();
      schedulePaint();
      return;
    }

    if (!result.consumed) {
      vpRef.current = panByScreenDelta(vpRef.current, clientDx, clientDy);
      publishViewportCoalesced(false);
    }
    schedulePaint();
  };

  const onPointerUp = (e: React.PointerEvent) => {
    const host = hostRef.current;
    const screen = host
      ? {
          x: e.clientX - host.getBoundingClientRect().left,
          y: e.clientY - host.getBoundingClientRect().top,
        }
      : { x: 0, y: 0 };
    if (selectionRef.current.preview) {
      restorePreviewOrigin();
    }
    const result = handleSelectionPointer(
      treeRef.current,
      selectionRef.current,
      "up",
      screen,
      vpRef.current,
    );
    selectionRef.current = result.session;
    if (result.commit) {
      undoRef.current.applyWithUndo(treeRef.current, {
        opId: `sg_xf_${Date.now()}_${result.commit.id}`,
        type: "set_smart_transform",
        payload: {
          id: result.commit.id,
          transform: result.commit.transform,
          ...(result.commit.bounds ? { bounds: result.commit.bounds } : {}),
        },
      });
      rebuildWithRmInk();
      pushDocSnapshotToRm();
    }
    dragRef.current = null;
    // Settle flush after pan/selection drag — tablet must sharp-rasterize once more.
    publishViewportCoalesced(true);
    markGesturing(false);
    schedulePaint();
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
      <ToolStrip
        tool={tool}
        onToolChange={(t) => {
          setTool(t);
          selectionRef.current = { ...selectionRef.current, tool: t };
          setRefuseReason(null);
        }}
        onCreateSmartGroup={() => {
          const ids = selectionRef.current.selectedInkIds;
          const result = createSmartGroupFromSelection(
            treeRef.current,
            undoRef.current,
            ids,
          );
          if (result.kind === "refused") {
            setRefuseReason(result.reason);
            return;
          }
          setRefuseReason(null);
          selectionRef.current = {
            ...selectionRef.current,
            selectedInkIds: [],
            selectedId: result.smartGroupId,
          };
          setInkSelCount(0);
          rebuildWithRmInk();
          pushDocSnapshotToRm();
          schedulePaint();
        }}
        createDisabled={inkSelCount < 2}
        refuseReason={refuseReason}
      />
      <canvas ref={canvasRef} data-region="WorldLayer" />
      <div
        ref={overlayRef}
        className="c-selection-overlay"
        data-region="SelectionOverlay"
        data-state="tool.selection.idle"
        hidden
        aria-hidden="true"
      >
        <span className="c-handle c-handle-nw" data-handle="nw" />
        <span className="c-handle c-handle-n" data-handle="n" />
        <span className="c-handle c-handle-ne" data-handle="ne" />
        <span className="c-handle c-handle-e" data-handle="e" />
        <span className="c-handle c-handle-se" data-handle="se" />
        <span className="c-handle c-handle-s" data-handle="s" />
        <span className="c-handle c-handle-sw" data-handle="sw" />
        <span className="c-handle c-handle-w" data-handle="w" />
      </div>
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
