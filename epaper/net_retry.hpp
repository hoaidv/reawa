#pragma once

namespace epaper {

/** App TCP retry interval. At most kTcpRetryLimit attempts per outage, then wait for the Infini button. */
inline constexpr int kAppTcpRetryMs = 5000;
inline constexpr int kTcpRetryLimit = 3;

/** UsbLink worker: HUD probe only. Never restore/ioctl on this interval. */
inline constexpr int kUsbLinkCheckMs = 1000;

} // namespace epaper
