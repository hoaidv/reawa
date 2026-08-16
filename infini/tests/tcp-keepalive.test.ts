/**
 * STORY-EP-034 — Infini listen-socket TCP keepalive.
 */
import { createRequire } from "node:module";
import { describe, expect, it } from "vitest";

const require = createRequire(import.meta.url);
const { enableTcpKeepAlive } = require("../electron/tcpKeepAlive.cjs");

describe("STORY-EP-034 TCP keepalive", () => {
  it("enables keepalive with 5s initial delay", () => {
    const calls: [boolean, number][] = [];
    enableTcpKeepAlive({
      setKeepAlive: (on: boolean, ms: number) => {
        calls.push([on, ms]);
      },
    });
    expect(calls).toEqual([[true, 5000]]);
  });
});
