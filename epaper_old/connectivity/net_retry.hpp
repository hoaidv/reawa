#pragma once

namespace epaper {

/** App TCP retry interval. Hard cap — never reset except plug-edge or manual USB tap. */
inline constexpr int kAppTcpRetryMs = 5000;
inline constexpr int kTcpRetryLimit = 3;

/** UsbLink HUD probe. Read-only sysfs; never restore/ioctl on this interval. */
inline constexpr int kUsbLinkCheckMs = 5000;

} // namespace epaper
