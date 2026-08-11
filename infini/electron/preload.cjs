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
});
