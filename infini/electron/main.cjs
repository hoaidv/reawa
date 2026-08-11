const { app, BrowserWindow, ipcMain } = require("electron");
const net = require("node:net");
const path = require("node:path");

/** @type {BrowserWindow | null} */
let mainWindow = null;
/** @type {import('node:net').Server | null} */
let strokeServer = null;

const STROKE_PORT = Number(process.env.INFINI_STROKE_PORT || 9877);

function broadcastStroke(obj) {
  if (mainWindow && !mainWindow.isDestroyed()) {
    mainWindow.webContents.send("rm-stroke", obj);
  }
}

/**
 * Legacy EXP stroke ingest (Epaper StrokeSync → Mac).
 * JSON-lines: stroke_begin | stroke_point | stroke_end
 */
function startStrokeIngestServer() {
  if (strokeServer) return;
  strokeServer = net.createServer((socket) => {
    let buf = "";
    console.log("[stroke-ingest] client connected", socket.remoteAddress);
    socket.on("data", (chunk) => {
      buf += chunk.toString("utf8");
      let nl;
      while ((nl = buf.indexOf("\n")) >= 0) {
        const line = buf.slice(0, nl).trim();
        buf = buf.slice(nl + 1);
        if (!line) continue;
        try {
          const obj = JSON.parse(line);
          broadcastStroke(obj);
        } catch (e) {
          console.warn("[stroke-ingest] bad line", line.slice(0, 120));
        }
      }
    });
    socket.on("close", () => console.log("[stroke-ingest] client closed"));
    socket.on("error", (err) => console.warn("[stroke-ingest] socket", err.message));
  });
  strokeServer.listen(STROKE_PORT, "0.0.0.0", () => {
    console.log(`[stroke-ingest] listening on 0.0.0.0:${STROKE_PORT}`);
  });
  strokeServer.on("error", (err) => {
    console.error("[stroke-ingest] server error", err.message);
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
}

app.whenReady().then(() => {
  startStrokeIngestServer();
  createWindow();
  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on("window-all-closed", () => {
  if (strokeServer) {
    strokeServer.close();
    strokeServer = null;
  }
  if (process.platform !== "darwin") app.quit();
});

ipcMain.handle("stroke-ingest-port", () => STROKE_PORT);
