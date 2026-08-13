/**
 * @implements [SRS-IN-18] Device Log overlay — WindowFrame chrome, not WorldLayer
 */
import { useCallback, useEffect, useMemo, useState, type ReactNode } from "react";
import { filterDebugLog, type DebugLogRecord } from "./filterDebugLog";

const COPY = {
  button: "Device Log",
  title: "Device Log",
  filter: "Filter",
  empty: "No device log yet",
  disconnected: "Device log not connected",
  disconnectedHint: "Enable EPAPER_DEBUG_LOG on the tablet and keep Infini listening on TCP 9878.",
  close: "Close",
  clear: "Clear",
  filterEmpty: "No lines match the filter",
} as const;

type OverlayState = "dlog.closed" | "dlog.open_empty" | "dlog.streaming" | "dlog.disconnected" | "dlog.filtered";

function overlayState(open: boolean, connected: boolean, bufferLen: number, filter: string, shownLen: number): OverlayState {
  if (!open) return "dlog.closed";
  if (filter.trim() !== "") return "dlog.filtered";
  if (!connected) return "dlog.disconnected";
  if (bufferLen === 0) return "dlog.open_empty";
  if (shownLen >= 1) return "dlog.streaming";
  return "dlog.open_empty";
}

export function DeviceLogChrome() {
  const [open, setOpen] = useState(false);
  const [filter, setFilter] = useState("");
  const [buffer, setBuffer] = useState<DebugLogRecord[]>([]);
  const [connected, setConnected] = useState(false);

  const native = typeof window !== "undefined" ? window.infiniNative : undefined;

  useEffect(() => {
    if (!native?.onDebugLog) return;
    void native.debugLogSnapshot?.().then((rows) => {
      if (Array.isArray(rows) && rows.length) setBuffer(rows);
    });
    void native.debugClientCount?.().then((n) => setConnected((n ?? 0) > 0));
    const offLog = native.onDebugLog((rec) => {
      setBuffer((prev) => {
        const next = prev.concat(rec);
        const overflow = next.length - 10000;
        return overflow > 0 ? next.slice(overflow) : next;
      });
    });
    const offClient = native.onDebugClient?.((ev) => {
      setConnected(ev.n > 0);
    });
    return () => {
      offLog();
      offClient?.();
    };
  }, [native]);

  const sendControl = useCallback(
    (type: "debug_request" | "debug_start" | "debug_stop") => {
      void native?.sendDebugControl?.(type);
    },
    [native],
  );

  const openOverlay = useCallback(() => {
    setOpen(true);
    void native?.setDebugPanelOpen?.(true);
    sendControl("debug_request");
    sendControl("debug_start");
  }, [native, sendControl]);

  const closeOverlay = useCallback(() => {
    setOpen(false);
    void native?.setDebugPanelOpen?.(false);
    sendControl("debug_stop");
  }, [native, sendControl]);

  const clearLog = useCallback(() => {
    setBuffer([]);
    void native?.clearDebugLog?.();
  }, [native]);

  const toggle = useCallback(() => {
    if (open) closeOverlay();
    else openOverlay();
  }, [open, openOverlay, closeOverlay]);

  useEffect(() => {
    if (!open) return;
    const onKey = (e: KeyboardEvent) => {
      if (e.key === "Escape") {
        e.preventDefault();
        closeOverlay();
      }
    };
    window.addEventListener("keydown", onKey);
    return () => window.removeEventListener("keydown", onKey);
  }, [open, closeOverlay]);

  const shown = useMemo(() => filterDebugLog(buffer, filter), [buffer, filter]);
  const state = overlayState(open, connected, buffer.length, filter, shown.length);

  let streamBody: ReactNode;
  if (!open) {
    streamBody = null;
  } else if (state === "dlog.disconnected" && buffer.length === 0) {
    streamBody = (
      <>
        <p data-copy="copy.device_log.disconnected">{COPY.disconnected}</p>
        <p data-copy="copy.device_log.disconnected_hint">{COPY.disconnectedHint}</p>
      </>
    );
  } else if (state === "dlog.filtered" && shown.length === 0) {
    streamBody = <p data-copy="copy.device_log.filter_empty">{COPY.filterEmpty}</p>;
  } else if (buffer.length === 0) {
    streamBody = <p data-copy="copy.device_log.empty">{COPY.empty}</p>;
  } else {
    streamBody = shown.map((r, i) => (
      <div key={`${r.ts}-${i}`} className="c-device-log-line">
        <span className="c-device-log-level">{r.level}</span>
        <span className="c-device-log-msg">{r.msg}</span>
      </div>
    ));
  }

  return (
    <>
      <button
        type="button"
        className="c-device-log-btn"
        data-region="DeviceLogButton"
        data-control="btn.device_log"
        aria-pressed={open}
        onClick={toggle}
      >
        {COPY.button}
      </button>
      <div
        className="c-device-log-overlay"
        data-region="DeviceLogOverlay"
        data-state={state}
        hidden={!open}
        aria-hidden={!open}
      >
        <div className="c-device-log-toolbar" data-region="DeviceLogToolbar">
          <h2 data-copy="copy.device_log.title">{COPY.title}</h2>
          <label>
            <span className="visually-hidden">{COPY.filter}</span>
            <input
              type="search"
              data-control="field.device_log_filter"
              placeholder={COPY.filter}
              value={filter}
              onChange={(e) => setFilter(e.target.value)}
            />
          </label>
          <button
            type="button"
            data-control="btn.device_log_clear"
            data-copy="copy.device_log.clear"
            onClick={clearLog}
          >
            {COPY.clear}
          </button>
          <button
            type="button"
            data-control="btn.device_log_close"
            data-copy="copy.device_log.close"
            onClick={closeOverlay}
          >
            {COPY.close}
          </button>
        </div>
        <div
          className="c-device-log-stream"
          data-region="DeviceLogStream"
          data-control="list.device_log_stream"
          role="log"
          aria-live="polite"
        >
          {streamBody}
        </div>
      </div>
    </>
  );
}
