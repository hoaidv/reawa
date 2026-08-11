/**
 * Session transport that forwards viewport/doc over Electron IPC → RM TCP clients.
 * @implements [SRS-IN-07] Infini → Epaper viewport channel
 */

import type { DocOpMessage, SessionTransport, ViewportMessage } from "./types";

export class IpcRmTransport implements SessionTransport {
  sendViewport(msg: ViewportMessage): void {
    void window.infiniNative?.sendToRm?.(msg);
  }

  sendDocOp(msg: DocOpMessage): void {
    void window.infiniNative?.sendToRm?.(msg);
  }
}
