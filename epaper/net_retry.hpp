#pragma once

namespace epaper {

/** App TCP retry (StrokeSync :9877 and debug-log :9878). Not USB infra. */
inline constexpr int kAppTcpRetryMs = 5000;

/** UsbLink worker: probe/restore interval. Never run on the GUI thread. */
inline constexpr int kUsbLinkCheckMs = 5000;

} // namespace epaper
