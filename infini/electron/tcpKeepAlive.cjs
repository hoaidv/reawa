/**
 * TCP keepalive on Infini listen sockets (:9877 / :9878).
 * @fix [STORY-EP-034] TCP keepalive so half-open USB links drop without unplug
 * @implements [SRS-IN-07] tablet sync session
 */

/**
 * @param {import('node:net').Socket} socket
 * @param {number} [initialDelayMs]
 */
function enableTcpKeepAlive(socket, initialDelayMs = 5000) {
  if (socket && typeof socket.setKeepAlive === "function") {
    socket.setKeepAlive(true, initialDelayMs);
  }
  return socket;
}

module.exports = { enableTcpKeepAlive };
