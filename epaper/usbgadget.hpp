/**
 * USB Ethernet gadget probe/restore.
 * Blocking sysfs, getifaddrs, ioctl — never call from the GUI/ink thread.
 * @implements [SRS-EP-08] link-down vs gadget-down
 */
#pragma once

namespace epaper {
namespace usbgadget {

inline constexpr const char *kIface = "usb0";
inline constexpr const char *kTabletAddr = "10.11.99.1";

enum class LinkClass {
    GadgetUp,
    GadgetDown,
};

/** TCP fail from StrokeSync — never ICMP on the UI thread. */
enum class HostTcpHint {
    Unknown,
    /** :9877 refused — Infini down, USB still up. Do not bounce gadget. */
    InfiniRefused,
    /** Timeout / unreachable — Mac USB path likely dead. */
    PathDead,
};

struct Snapshot {
    bool ifacePresent = false;
    bool flagsUp = false;
    bool hasTabletAddr = false;
    bool carrier = true;
    /** UDC not in "not attached" — cable/host session, not Mac Infini. */
    bool physicallyPlugged = false;
};

inline LinkClass classify(const Snapshot &s)
{
    // g_ether often reports sysfs carrier=0 while Mac ping works. Address + IFF_UP is the link.
    if (s.hasTabletAddr && s.flagsUp)
        return LinkClass::GadgetUp;
    return LinkClass::GadgetDown;
}

inline bool shouldRestoreGadget(LinkClass c, HostTcpHint hint = HostTcpHint::Unknown)
{
    if (hint == HostTcpHint::InfiniRefused)
        return false;
    if (c == LinkClass::GadgetDown)
        return true;
    return hint == HostTcpHint::PathDead;
}

inline const char *classLabel(LinkClass c)
{
    return c == LinkClass::GadgetUp ? "gadget-up — TCP retry only (EP-034)"
                                    : "gadget-down — no carrier or no 10.11.99.1 (EP-036)";
}

Snapshot probe();
/** Set usb0 10.11.99.1 / IFF_UP. Does not unbind UDC. Off GUI thread. */
bool assignUsb0Address();
/** Blocking sysfs/ioctl. Do not call from a timer — UDC unbind bricks Mac USB until tablet reboot. */
bool restoreWithoutUnplug();

inline constexpr const char *kRestoreOrder[] = {
    "configfs-udc",
    "udc-rebind",
    "usb0-addr",
};
inline constexpr int kRestoreOrderN = 3;

} // namespace usbgadget
} // namespace epaper
