/**
 * Session transport that forwards viewport/handshake over Electron IPC → RM TCP.
 * @implements [SRS-IN-07] Infini → Epaper viewport + handshake-gated load
 */

import type {
  DocLoadMessage,
  DocOpMessage,
  DrainAckMessage,
  SessionTransport,
  ViewportMessage,
} from "./types";

export class IpcRmTransport implements SessionTransport {
  sendViewport(msg: ViewportMessage): void {
    void window.infiniNative?.sendToRm?.(msg);
  }

  sendDrainAck(msg: DrainAckMessage): void {
    void window.infiniNative?.sendToRm?.(msg);
  }

  sendDocLoad(msg: DocLoadMessage): void {
    void window.infiniNative?.sendToRm?.(msg);
  }

  sendDocOp(_msg: DocOpMessage): void {
    // Viewer only — Infini must not emit document ops ([SRS-IN-07] / IN-027).
  }
}
