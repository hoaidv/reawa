const { contextBridge, ipcRenderer } = require("electron");

/**
 * Bridge for RM2 stroke ingest + Infini→Epaper viewport/refresh.
 * @implements [SRS-IN-07]
 */
contextBridge.exposeInMainWorld("infiniNative", {
  onRmStroke: (cb) => {
    const handler = (_event, payload) => {
      try {
        cb(payload);
      } catch (e) {
        console.error("onRmStroke handler", e);
      }
    };
    ipcRenderer.on("rm-stroke", handler);
    return () => ipcRenderer.removeListener("rm-stroke", handler);
  },
  strokeIngestPort: () => ipcRenderer.invoke("stroke-ingest-port"),
  sendToRm: (obj) => ipcRenderer.invoke("rm-send", obj),
  rmClientCount: () => ipcRenderer.invoke("rm-client-count"),
  onRmClient: (cb) => {
    const handler = (_event, payload) => {
      try {
        cb(payload);
      } catch (e) {
        console.error("onRmClient handler", e);
      }
    };
    ipcRenderer.on("rm-client", handler);
    return () => ipcRenderer.removeListener("rm-client", handler);
  },
  /** @implements [SRS-IN-17] debug sidecar IPC — not the :9877 document channel */
  onDebugLog: (cb) => {
    const handler = (_event, payload) => {
      try {
        cb(payload);
      } catch (e) {
        console.error("onDebugLog handler", e);
      }
    };
    ipcRenderer.on("debug-log", handler);
    return () => ipcRenderer.removeListener("debug-log", handler);
  },
  onDebugClient: (cb) => {
    const handler = (_event, payload) => {
      try {
        cb(payload);
      } catch (e) {
        console.error("onDebugClient handler", e);
      }
    };
    ipcRenderer.on("debug-client", handler);
    return () => ipcRenderer.removeListener("debug-client", handler);
  },
  debugPort: () => ipcRenderer.invoke("debug-port"),
  debugClientCount: () => ipcRenderer.invoke("debug-client-count"),
  debugLogSnapshot: () => ipcRenderer.invoke("debug-log-snapshot"),
  sendDebugControl: (type) => ipcRenderer.invoke("debug-send", type),
  setDebugPanelOpen: (open) => ipcRenderer.invoke("debug-set-panel-open", open),
});
