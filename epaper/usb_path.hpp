#pragma once

#include <atomic>

namespace epaper {

/** usb0 carrier + UDC configured. TCP apps must not connectToHost when false. */
inline std::atomic<bool> g_usbEthernetLive{false};

inline bool usbEthernetLive()
{
    return g_usbEthernetLive.load(std::memory_order_acquire);
}

inline void setUsbEthernetLive(bool live)
{
    g_usbEthernetLive.store(live, std::memory_order_release);
}

} // namespace epaper
