/**
 * STORY-EP-034 / STORY-EP-036 — gadget-up vs gadget-down; do not bounce when Infini refused.
 */
#include "usbgadget.hpp"

#include <cstdio>
#include <cstring>

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        if (!(cond)) {                                                                         \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);               \
            return 1;                                                                          \
        }                                                                                      \
    } while (0)

using epaper::usbgadget::classify;
using epaper::usbgadget::HostTcpHint;
using epaper::usbgadget::LinkClass;
using epaper::usbgadget::shouldRestoreGadget;
using epaper::usbgadget::Snapshot;

int main()
{
    Snapshot up;
    up.ifacePresent = true;
    up.flagsUp = true;
    up.hasTabletAddr = true;
    CHECK(classify(up) == LinkClass::GadgetUp);
    CHECK(!shouldRestoreGadget(classify(up)));
    CHECK(!shouldRestoreGadget(classify(up), HostTcpHint::InfiniRefused));

    Snapshot pingDead;
    pingDead.ifacePresent = true;
    pingDead.flagsUp = false;
    pingDead.hasTabletAddr = false;
    CHECK(classify(pingDead) == LinkClass::GadgetDown);
    CHECK(shouldRestoreGadget(classify(pingDead)));

    Snapshot missing;
    CHECK(classify(missing) == LinkClass::GadgetDown);

    Snapshot addrButDown;
    addrButDown.ifacePresent = true;
    addrButDown.hasTabletAddr = true;
    addrButDown.flagsUp = false;
    CHECK(classify(addrButDown) == LinkClass::GadgetDown);

    Snapshot noCarrier;
    noCarrier.ifacePresent = true;
    noCarrier.flagsUp = true;
    noCarrier.hasTabletAddr = true;
    noCarrier.carrier = false;
    CHECK(classify(noCarrier) == LinkClass::GadgetUp);

    Snapshot infiniDownUsbAlive;
    infiniDownUsbAlive.ifacePresent = true;
    infiniDownUsbAlive.flagsUp = true;
    infiniDownUsbAlive.hasTabletAddr = true;
    infiniDownUsbAlive.carrier = true;
    CHECK(classify(infiniDownUsbAlive) == LinkClass::GadgetUp);
    CHECK(!shouldRestoreGadget(classify(infiniDownUsbAlive), HostTcpHint::InfiniRefused));
    CHECK(shouldRestoreGadget(classify(infiniDownUsbAlive), HostTcpHint::PathDead));

    CHECK(epaper::usbgadget::kRestoreOrderN == 3);
    CHECK(std::strcmp(epaper::usbgadget::kRestoreOrder[0], "configfs-udc") == 0);
    CHECK(std::strcmp(epaper::usbgadget::kRestoreOrder[2], "usb0-addr") == 0);

    std::fprintf(stdout, "usbgadget_test OK\n");
    return 0;
}
