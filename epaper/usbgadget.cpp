#include "usbgadget.hpp"

#ifdef __linux__
#include <arpa/inet.h>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>
#endif

namespace epaper {
namespace usbgadget {
namespace {

void writeSys(const char *path, const char *val)
{
#ifdef __linux__
    std::ofstream f(path);
    if (f)
        f << val;
#else
    (void)path;
    (void)val;
#endif
}

#ifdef __linux__

std::string readFileTrim(const char *path)
{
    std::ifstream f(path);
    if (!f)
        return {};
    std::string s;
    std::getline(f, s);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
        s.pop_back();
    return s;
}

std::vector<std::string> listDir(const char *path)
{
    std::vector<std::string> out;
    DIR *d = opendir(path);
    if (!d)
        return out;
    while (struct dirent *e = readdir(d)) {
        if (e->d_name[0] == '.')
            continue;
        out.emplace_back(e->d_name);
    }
    closedir(d);
    return out;
}

bool usb0CarrierUp()
{
    const std::string c = readFileTrim("/sys/class/net/usb0/carrier");
    if (c == "0")
        return false;
    const std::string op = readFileTrim("/sys/class/net/usb0/operstate");
    if (op == "down" || op == "lowerlayerdown" || op == "nolearrier")
        return false;
    if (c.empty() && op.empty())
        return true;
    return c != "0";
}

void rebindPlatformUdc()
{
    for (const std::string &name : listDir("/sys/class/udc")) {
        const std::string unbind = std::string("/sys/class/udc/") + name + "/device/driver/unbind";
        const std::string bind = std::string("/sys/class/udc/") + name + "/device/driver/bind";
        writeSys(unbind.c_str(), name.c_str());
        usleep(200000);
        writeSys(bind.c_str(), name.c_str());
        usleep(200000);
    }
}

void rebindConfigfsUdc()
{
    for (const std::string &g : listDir("/sys/kernel/config/usb_gadget")) {
        const std::string udcPath = std::string("/sys/kernel/config/usb_gadget/") + g + "/UDC";
        const std::string udc = readFileTrim(udcPath.c_str());
        writeSys(udcPath.c_str(), "");
        usleep(200000);
        if (!udc.empty())
            writeSys(udcPath.c_str(), (udc + "\n").c_str());
        else {
            const auto udcs = listDir("/sys/class/udc");
            if (!udcs.empty())
                writeSys(udcPath.c_str(), (udcs.front() + "\n").c_str());
        }
        usleep(200000);
    }
}

bool assignUsb0Address()
{
    writeSys("/sys/class/net/usb0/device/power/control", "on");
    writeSys("/sys/class/net/usb0/device/power/autosuspend_delay_ms", "-1");
    writeSys("/sys/class/net/usb0/power/control", "on");

    const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        return false;

    ifreq ifr;
    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, kIface, IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0) {
        ifr.ifr_flags |= IFF_UP;
        ioctl(fd, SIOCSIFFLAGS, &ifr);
    }

    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, kIface, IFNAMSIZ - 1);
    auto *addr = reinterpret_cast<sockaddr_in *>(&ifr.ifr_addr);
    addr->sin_family = AF_INET;
    inet_pton(AF_INET, kTabletAddr, &addr->sin_addr);
    ioctl(fd, SIOCSIFADDR, &ifr);

    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, kIface, IFNAMSIZ - 1);
    auto *mask = reinterpret_cast<sockaddr_in *>(&ifr.ifr_netmask);
    mask->sin_family = AF_INET;
    inet_pton(AF_INET, "255.255.255.0", &mask->sin_addr);
    ioctl(fd, SIOCSIFNETMASK, &ifr);

    std::memset(&ifr, 0, sizeof(ifr));
    std::strncpy(ifr.ifr_name, kIface, IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0) {
        ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
        ioctl(fd, SIOCSIFFLAGS, &ifr);
    }
    ::close(fd);
    const Snapshot s = probe();
    return s.hasTabletAddr && s.flagsUp;
}

#endif

} // namespace

Snapshot probe()
{
    Snapshot s;
#ifdef __linux__
    ifaddrs *list = nullptr;
    if (getifaddrs(&list) != 0)
        return s;
    for (ifaddrs *p = list; p; p = p->ifa_next) {
        if (!p->ifa_name)
            continue;
        const bool usb0 = std::strcmp(p->ifa_name, kIface) == 0;
        if (usb0)
            s.ifacePresent = true;
        const unsigned flags = p->ifa_flags;
        const bool up = (flags & IFF_UP) != 0;
        if (usb0 && up)
            s.flagsUp = true;
        if (!p->ifa_addr || p->ifa_addr->sa_family != AF_INET)
            continue;
        const auto *in = reinterpret_cast<sockaddr_in *>(p->ifa_addr);
        char buf[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &in->sin_addr, buf, sizeof(buf));
        if (std::strcmp(buf, kTabletAddr) == 0) {
            s.hasTabletAddr = true;
            if (up)
                s.flagsUp = true;
            s.ifacePresent = true;
        }
    }
    freeifaddrs(list);
    s.carrier = usb0CarrierUp();
#endif
    return s;
}

bool restoreWithoutUnplug()
{
#ifdef __linux__
    // @fix [DEF-0001] UDC detach/attach only — never modprobe -r g_ether (bricks until reboot)
    rebindConfigfsUdc();
    rebindPlatformUdc();
    return assignUsb0Address();
#else
    return false;
#endif
}

} // namespace usbgadget
} // namespace epaper
