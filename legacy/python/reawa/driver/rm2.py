"""Shared reMarkable 2 SSH connection and pen input event parsing."""

from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Generator, Iterator, Tuple

import paramiko
from paramiko import RSAKey

# reMarkable 2 defaults
RM2_IP = "10.11.99.1"
RM2_USER = "root"
RM2_PEN_FILE = "/dev/input/event1"

# Digitizer coordinate range (Wacom, not display pixels)
PEN_X_MAX = 20967
PEN_Y_MAX = 15725
RM2_ASPECT = PEN_X_MAX / PEN_Y_MAX
RM2_DPI = 2531

SSH_KEY_BITS = 3072
SSH_KEY_COMMENT = "remarkable-rm2-driver"

E_FORMAT = "2IHHi"
E_SZ = struct.calcsize(E_FORMAT)

# Event type constants
EV_SYN = 0
EV_KEY = 1
EV_ABS = 3

# Event codes
ABS_X = 0
ABS_Y = 1
ABS_PRESSURE = 24
ABS_DISTANCE = 25
ABS_TILT_Y = 27
ABS_TILT_X = 26
BTN_TOOL_PEN = 320
BTN_TOUCH = 330
BTN_STYLUS = 331
SYN_REPORT = 0

TYPE_NAMES = {
    EV_SYN: "EV_SYN",
    EV_KEY: "EV_KEY",
    EV_ABS: "EV_ABS",
}

CODE_NAMES = {
    EV_SYN: {SYN_REPORT: "SYN_REPORT"},
    EV_KEY: {
        BTN_TOUCH: "BTN_TOUCH",
        BTN_STYLUS: "BTN_STYLUS",
        BTN_TOOL_PEN: "BTN_TOOL_PEN",
    },
    EV_ABS: {
        ABS_X: "ABS_X",
        ABS_Y: "ABS_Y",
        ABS_PRESSURE: "ABS_PRESSURE",
        ABS_DISTANCE: "ABS_DISTANCE",
        ABS_TILT_Y: "ABS_TILT_Y",
        ABS_TILT_X: "ABS_TILT_X",
    },
}

RawEvent = Tuple[int, int, int, int, int]


def _new_ssh_client() -> paramiko.SSHClient:
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    return client


def load_ssh_key(key_path: Path) -> RSAKey | None:
    if not key_path.exists():
        return None
    return RSAKey.from_private_key_file(str(key_path))


def ensure_ssh_key(key_path: Path) -> RSAKey:
    existing = load_ssh_key(key_path)
    if existing is not None:
        return existing

    key_path.parent.mkdir(parents=True, exist_ok=True)
    key = RSAKey.generate(SSH_KEY_BITS)
    key.write_private_key_file(str(key_path))
    key_path.chmod(0o600)

    pub_path = key_path.with_name(f"{key_path.name}.pub")
    pub_path.write_text(f"{key.get_name()} {key.get_base64()} {SSH_KEY_COMMENT}\n")
    pub_path.chmod(0o644)
    return key


def public_key_line(key: RSAKey) -> str:
    return f"{key.get_name()} {key.get_base64()} {SSH_KEY_COMMENT}"


def install_authorized_key(client: paramiko.SSHClient, pubkey_line: str) -> None:
    pubkey_escaped = pubkey_line.replace("'", "'\"'\"'")
    script = f"""
SSH_DIR="$HOME/.ssh"
AUTH_KEYS="$SSH_DIR/authorized_keys"
mkdir -p "$SSH_DIR"
chmod 700 "$SSH_DIR"
touch "$AUTH_KEYS"
chmod 600 "$AUTH_KEYS"
grep -qxF '{pubkey_escaped}' "$AUTH_KEYS" 2>/dev/null || echo '{pubkey_escaped}' >> "$AUTH_KEYS"
"""
    _, stdout, stderr = client.exec_command(script)
    if stdout.channel.recv_exit_status() != 0:
        raise RuntimeError(f"Failed to install SSH key: {stderr.read().decode().strip()}")

    _, vout, _ = client.exec_command(
        f"grep -qxF '{pubkey_escaped}' \"$HOME/.ssh/authorized_keys\" && echo OK"
    )
    if vout.read().decode().strip() != "OK":
        raise RuntimeError("SSH key was not found in authorized_keys after install.")


def connect_with_key(
    ip: str,
    key_path: Path,
    user: str = RM2_USER,
    timeout: int = 10,
) -> paramiko.SSHClient:
    key = load_ssh_key(key_path)
    if key is None:
        raise FileNotFoundError(f"SSH key not found: {key_path}")

    client = _new_ssh_client()
    client.connect(
        ip,
        username=user,
        pkey=key,
        timeout=timeout,
        look_for_keys=False,
        allow_agent=False,
    )
    return client


def setup_key(
    ip: str,
    password: str,
    key_path: Path,
    user: str = RM2_USER,
    timeout: int = 10,
) -> paramiko.SSHClient:
    client = _new_ssh_client()
    client.connect(ip, username=user, password=password, timeout=timeout)
    key = ensure_ssh_key(key_path)
    install_authorized_key(client, public_key_line(key))
    return client


def connect_ssh(
    ip: str,
    key_path: Path,
    password: str | None = None,
    user: str = RM2_USER,
) -> paramiko.SSHClient:
    """Connect with key; fall back to password setup when key auth fails."""
    try:
        return connect_with_key(ip, key_path, user=user)
    except (paramiko.AuthenticationException, FileNotFoundError):
        if not password:
            raise
    return setup_key(ip, password, key_path, user=user)


def connect_pen_stream(
    ip: str,
    key_path: Path,
    password: str | None = None,
    user: str = RM2_USER,
    pen_file: str = RM2_PEN_FILE,
) -> tuple[paramiko.SSHClient, paramiko.ChannelFile]:
    client = connect_ssh(ip, key_path, password=password, user=user)
    cmd = f"dd bs={E_SZ} if={pen_file} 2>/dev/null"
    _, stdout, _ = client.exec_command(cmd, bufsize=E_SZ, timeout=None)
    return client, stdout


def read_raw_events(stdout: paramiko.ChannelFile) -> Iterator[RawEvent]:
    while True:
        data = stdout.read(E_SZ)
        if len(data) < E_SZ:
            break
        yield struct.unpack(E_FORMAT, data)


@dataclass(frozen=True)
class PenFrame:
    tv_sec: int
    tv_usec: int
    x: int
    y: int
    pressure: int | None
    touching: bool
    in_proximity: bool


def read_pen_frames(stdout: paramiko.ChannelFile) -> Generator[PenFrame, None, None]:
    pen_x: int | None = None
    pen_y: int | None = None
    pen_pressure: int | None = None
    pen_touching = False
    in_proximity = False

    for tv_sec, tv_usec, e_type, e_code, e_value in read_raw_events(stdout):
        if e_type == EV_ABS:
            if e_code == ABS_X:
                pen_x = e_value
            elif e_code == ABS_Y:
                pen_y = e_value
            elif e_code == ABS_PRESSURE:
                pen_pressure = e_value
        elif e_type == EV_KEY:
            if e_code == BTN_TOUCH:
                pen_touching = e_value == 1
            elif e_code == BTN_TOOL_PEN:
                in_proximity = e_value == 1

        if e_type == EV_SYN and e_code == SYN_REPORT:
            if pen_x is not None and pen_y is not None:
                yield PenFrame(
                    tv_sec=tv_sec,
                    tv_usec=tv_usec,
                    x=pen_x,
                    y=pen_y,
                    pressure=pen_pressure,
                    touching=pen_touching,
                    in_proximity=in_proximity,
                )
