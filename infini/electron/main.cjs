const { app, BrowserWindow, ipcMain } = require("electron");
const net = require("node:net");
const path = require("node:path");
const {
  DEBUG_RING_CAP,
  decodeDebugLine,
  pushDebugRing,
  controlLine,
} = require("./debugLogChannel.cjs");

/** @type {BrowserWindow | null} */
let mainWindow = null;
/** @type {import('node:net').Server | null} */
let strokeServer = null;
/** @type {Set<import('node:net').Socket>} */
const rmClients = new Set();

const STROKE_PORT = Number(process.env.INFINI_STROKE_PORT || 9877);

/** @implements [SRS-IN-17] sidecar listen — separate socket from :9877 */
const DEBUG_PORT = Number(process.env.INFINI_DEBUG_PORT || 9878);
/** @type {import('node:net').Server | null} */
let debugServer = null;
/** @type {Set<import('node:net').Socket>} */
const debugClients = new Set();
/** @type {object[]} */
const debugRing = [];
let debugPanelOpen = false;

function broadcastStroke(obj) {
  if (mainWindow && !mainWindow.isDestroyed()) {
    mainWindow.webContents.send("rm-stroke", obj);
  }
}

/** Push current RM client count when renderer attaches (STORY-IN-019). */
function pushRmClientSync(webContents) {
  if (!webContents || webContents.isDestroyed()) return;
  webContents.send("rm-client", { type: "sync", n: rmClients.size });
}

/** Infini → Epaper (viewport / region_refresh) over the same TCP clients. */
function sendToRmClients(obj) {
  const line = `${JSON.stringify(obj)}\n`;
  for (const socket of rmClients) {
    if (socket.destroyed) {
      rmClients.delete(socket);
      continue;
    }
    try {
      socket.write(line);
    } catch (e) {
      console.warn("[stroke-ingest] write to RM failed", e.message);
    }
  }
  return rmClients.size;
}

/**
 * Bidirectional JSON-lines with Epaper StrokeSync.
 * RM → Mac: hello, doc_change, queue_empty, load_ack, stroke_*
 * Mac → RM: drain_ack, doc_load, viewport
 */
function startStrokeIngestServer() {
  if (strokeServer) return;
  strokeServer = net.createServer((socket) => {
    let buf = "";
    rmClients.add(socket);
    console.log("[stroke-ingest] client connected", socket.remoteAddress, "n=", rmClients.size);
    if (mainWindow && !mainWindow.isDestroyed()) {
      mainWindow.webContents.send("rm-client", { type: "connected", n: rmClients.size });
    }
    socket.on("data", (chunk) => {
      buf += chunk.toString("utf8");
      let nl;
      while ((nl = buf.indexOf("\n")) >= 0) {
        const line = buf.slice(0, nl).trim();
        buf = buf.slice(nl + 1);
        if (!line) continue;
        try {
          const obj = JSON.parse(line);
          const t = obj && obj.type;
          if (t && t !== "stroke_point") {
            console.log("[stroke-ingest]", t, t === "doc_change" ? (obj.op && obj.op.type) : "");
          }
          broadcastStroke(obj);
        } catch (e) {
          console.warn("[stroke-ingest] bad line", line.slice(0, 120));
        }
      }
    });
    socket.on("close", () => {
      rmClients.delete(socket);
      console.log("[stroke-ingest] client closed n=", rmClients.size);
      if (mainWindow && !mainWindow.isDestroyed()) {
        mainWindow.webContents.send("rm-client", { type: "closed", n: rmClients.size });
      }
    });
    socket.on("error", (err) => console.warn("[stroke-ingest] socket", err.message));
  });
  strokeServer.listen(STROKE_PORT, "0.0.0.0", () => {
    console.log(`[stroke-ingest] listening on 0.0.0.0:${STROKE_PORT}`);
  });
  strokeServer.on("error", (err) => {
    console.error("[stroke-ingest] server error", err.message);
  });
}

function sendDebugControl(type) {
  const line = controlLine(type);
  for (const socket of debugClients) {
    if (socket.destroyed) {
      debugClients.delete(socket);
      continue;
    }
    try {
      socket.write(line);
    } catch (e) {
      console.warn("[debug-log] write failed", e.message);
    }
  }
  return debugClients.size;
}

function pushDebugClientSync(webContents) {
  if (!webContents || webContents.isDestroyed()) return;
  webContents.send("debug-client", { type: "sync", n: debugClients.size });
}

function startDebugLogServer() {
  if (debugServer) return;
  debugServer = net.createServer((socket) => {
    let buf = "";
    debugClients.add(socket);
    console.log("[debug-log] client connected", socket.remoteAddress, "n=", debugClients.size);
    if (mainWindow && !mainWindow.isDestroyed()) {
      mainWindow.webContents.send("debug-client", { type: "connected", n: debugClients.size });
    }
    if (debugPanelOpen) {
      try {
        socket.write(controlLine("debug_request"));
        socket.write(controlLine("debug_start"));
      } catch (e) {
        console.warn("[debug-log] start on connect failed", e.message);
      }
    }
    socket.on("data", (chunk) => {
      buf += chunk.toString("utf8");
      let nl;
      while ((nl = buf.indexOf("\n")) >= 0) {
        const line = buf.slice(0, nl).trim();
        buf = buf.slice(nl + 1);
        if (!line) continue;
        const decoded = decodeDebugLine(line);
        if (decoded.kind !== "log") continue;
        pushDebugRing(debugRing, decoded.record, DEBUG_RING_CAP);
        if (mainWindow && !mainWindow.isDestroyed()) {
          mainWindow.webContents.send("debug-log", decoded.record);
        }
      }
    });
    socket.on("close", () => {
      debugClients.delete(socket);
      console.log("[debug-log] client closed n=", debugClients.size);
      if (mainWindow && !mainWindow.isDestroyed()) {
        mainWindow.webContents.send("debug-client", { type: "closed", n: debugClients.size });
      }
    });
    socket.on("error", (err) => console.warn("[debug-log] socket", err.message));
  });
  debugServer.listen(DEBUG_PORT, "0.0.0.0", () => {
    console.log(`[debug-log] listening on 0.0.0.0:${DEBUG_PORT}`);
  });
  debugServer.on("error", (err) => {
    console.error("[debug-log] server error", err.message);
  });
}

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1280,
    height: 800,
    minWidth: 960,
    minHeight: 640,
    title: "Infini",
    backgroundColor: "#F2F4F7",
    webPreferences: {
      preload: path.join(__dirname, "preload.cjs"),
      contextIsolation: true,
      nodeIntegration: false,
    },
  });

  const isDev = !app.isPackaged;
  if (isDev) {
    void mainWindow.loadURL("http://127.0.0.1:5173");
  } else {
    void mainWindow.loadFile(path.join(__dirname, "../dist/index.html"));
  }

  // Eager sync: if Epaper connected before React subscribed, renderer still learns n on load.
  mainWindow.webContents.on("did-finish-load", () => {
    pushRmClientSync(mainWindow.webContents);
    pushDebugClientSync(mainWindow.webContents);
  });
}

app.whenReady().then(() => {
  startStrokeIngestServer();
  startDebugLogServer();
  createWindow();
  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on("window-all-closed", () => {
  // Keep stroke ingest listening on macOS even when the last window closes —
  // otherwise RM_SYNC reconnects against a dead :9877 while Electron stays in the dock
  // (stale "connected" UI / silent tablet link).
  if (process.platform !== "darwin") {
    if (strokeServer) {
      strokeServer.close();
      strokeServer = null;
    }
    if (debugServer) {
      debugServer.close();
      debugServer = null;
    }
    rmClients.clear();
    debugClients.clear();
    app.quit();
  }
});

app.on("before-quit", () => {
  if (strokeServer) {
    strokeServer.close();
    strokeServer = null;
  }
  if (debugServer) {
    debugServer.close();
    debugServer = null;
  }
  rmClients.clear();
  debugClients.clear();
});

ipcMain.handle("stroke-ingest-port", () => STROKE_PORT);
ipcMain.handle("rm-send", (_event, obj) => sendToRmClients(obj));
ipcMain.handle("rm-client-count", () => rmClients.size);
ipcMain.handle("debug-port", () => DEBUG_PORT);
ipcMain.handle("debug-client-count", () => debugClients.size);
ipcMain.handle("debug-log-snapshot", () => debugRing.slice());
ipcMain.handle("debug-log-clear", () => {
  debugRing.length = 0;
  return true;
});
ipcMain.handle("debug-send", (_event, type) => sendDebugControl(type));
ipcMain.handle("debug-set-panel-open", (_event, open) => {
  debugPanelOpen = !!open;
  return debugPanelOpen;
});
