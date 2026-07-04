"""Discover SSH-reachable hosts on local/USB-tethered network interfaces."""

from __future__ import annotations

import re
import socket
import struct
import subprocess
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from typing import Iterable

SSH_PORT = 22
_PROBE_TIMEOUT = 0.35
_MAX_SCAN_HOSTS = 64

# Skip loopback, VPN tunnels, and common Wi‑Fi (still scan its subnet via others).
_SKIP_IFACES = frozenset({"lo0", "bridge0", "gif0", "stf0"})


@dataclass(frozen=True)
class NetworkInterface:
    name: str
    address: str
    netmask: str
    network: str
    broadcast: str
    prefix_len: int


def _mask_to_prefix(mask: str) -> int:
    if mask.startswith("0x"):
        bits = int(mask, 16)
    else:
        parts = [int(x) for x in mask.split(".")]
        bits = struct.unpack("!I", bytes(parts))[0]
    return bin(bits).count("1")


def _ipv4_to_int(ip: str) -> int:
    return struct.unpack("!I", socket.inet_aton(ip))[0]


def _int_to_ipv4(n: int) -> str:
    return socket.inet_ntoa(struct.pack("!I", n))


def list_network_interfaces() -> list[NetworkInterface]:
    """Parse `ifconfig` for IPv4 interfaces."""
    try:
        result = subprocess.run(
            ["ifconfig"],
            capture_output=True,
            text=True,
            timeout=5,
            check=False,
        )
    except OSError:
        return []

    interfaces: list[NetworkInterface] = []
    current: str | None = None

    for line in result.stdout.splitlines():
        if line and not line[0].isspace():
            match = re.match(r"^(\w+):", line)
            if match:
                current = match.group(1)
            continue

        if current is None or current in _SKIP_IFACES:
            continue
        if current.startswith("utun"):
            continue

        stripped = line.strip()
        if not stripped.startswith("inet "):
            continue

        parts = stripped.split()
        # inet 10.11.99.12 netmask 0xffffffe0 broadcast 10.11.99.31
        try:
            addr = parts[1]
            mask_idx = parts.index("netmask")
            mask = parts[mask_idx + 1]
        except (IndexError, ValueError):
            continue

        if ":" in addr:
            continue

        prefix = _mask_to_prefix(mask)
        addr_i = _ipv4_to_int(addr)
        mask_i = (0xFFFFFFFF << (32 - prefix)) & 0xFFFFFFFF
        network_i = addr_i & mask_i
        broadcast_i = network_i | (~mask_i & 0xFFFFFFFF)

        interfaces.append(
            NetworkInterface(
                name=current,
                address=addr,
                netmask=mask,
                network=_int_to_ipv4(network_i),
                broadcast=_int_to_ipv4(broadcast_i),
                prefix_len=prefix,
            )
        )

    return interfaces


def _candidate_ips(iface: NetworkInterface) -> list[str]:
    """Build probe list for a subnet (gateway .1 first, then rest)."""
    net_i = _ipv4_to_int(iface.network)
    bcast_i = _ipv4_to_int(iface.broadcast)
    host_count = bcast_i - net_i - 1
    if host_count <= 0:
        return []

    limit = min(host_count, _MAX_SCAN_HOSTS)
    candidates: list[str] = []

    gateway = _int_to_ipv4(net_i + 1)
    if gateway != iface.address:
        candidates.append(gateway)

    for offset in range(1, limit + 1):
        ip = _int_to_ipv4(net_i + offset)
        if ip == iface.address or ip == iface.broadcast:
            continue
        if ip not in candidates:
            candidates.append(ip)

    return candidates


def _probe_ssh(ip: str, port: int = SSH_PORT, timeout: float = _PROBE_TIMEOUT) -> str | None:
    try:
        with socket.create_connection((ip, port), timeout=timeout):
            return ip
    except OSError:
        return None


def discover_ssh_hosts(
    interfaces: Iterable[NetworkInterface] | None = None,
    max_workers: int = 16,
) -> set[str]:
    """Scan local interface subnets for hosts with SSH open.

    Does not require a preconfigured device IP — discovers gateways and peers
    on USB-tethered subnets (e.g. reMarkable at 10.11.99.1 on en8).
    """
    ifaces = list(interfaces) if interfaces is not None else list_network_interfaces()
    candidates: list[str] = []
    seen: set[str] = set()

    for iface in ifaces:
        for ip in _candidate_ips(iface):
            if ip not in seen:
                seen.add(ip)
                candidates.append(ip)

    if not candidates:
        return set()

    found: set[str] = set()
    workers = min(max_workers, max(1, len(candidates)))
    with ThreadPoolExecutor(max_workers=workers) as pool:
        futures = {pool.submit(_probe_ssh, ip): ip for ip in candidates}
        for future in as_completed(futures):
            result = future.result()
            if result:
                found.add(result)

    return found


def discover_usb_ssh_hosts() -> set[str]:
    """Prefer scanning non-primary `en*` interfaces (typical USB Ethernet gadgets)."""
    ifaces = list_network_interfaces()
    usb_like = [
        i
        for i in ifaces
        if i.name.startswith("en") and i.name not in {"en0", "en1", "en2", "en3"}
    ]
    if usb_like:
        return discover_ssh_hosts(usb_like)
    return discover_ssh_hosts(ifaces)
