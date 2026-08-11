const { contextBridge, ipcRenderer } = require("electron");

/**
 * Bridge for RM2 stroke ingest (StrokeSync → :9877).
 * Full ADR-0009 TabletSession wire lands in later stories.
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
});
