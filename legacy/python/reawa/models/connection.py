from __future__ import annotations

import uuid
from dataclasses import asdict, dataclass, field
from enum import Enum
from typing import Any, Literal

OutputMode = Literal["RELATIVE", "ABSOLUTE"]

RM2_ASPECT = 20967 / 15725  # digitizer aspect ratio (portrait)


class ConnectionStatus(str, Enum):
    OFFLINE = "offline"      # device IP not reachable
    ONLINE = "online"        # device IP reachable, not connected
    CONNECTED = "connected"  # pen stream active
    ERROR = "error"          # connect attempt failed


@dataclass
class AbsoluteConfig:
    region_x: float = 100.0
    region_y: float = 100.0
    region_width: float = 400.0
    region_height: float = 400.0 / RM2_ASPECT
    border_color: str = "#3B82F6"
    border_style: str = "solid"  # solid | dashed
    snap_window_enabled: bool = False
    snapped_window_ref: str | None = None

    def __post_init__(self) -> None:
        self.lock_aspect()

    def lock_aspect(self) -> None:
        """Keep region dimensions at RM2 aspect ratio."""
        self.region_height = self.region_width / RM2_ASPECT

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)

    @classmethod
    def from_dict(cls, data: dict[str, Any] | None) -> AbsoluteConfig:
        if not data:
            return default_absolute_config()
        cfg = cls(**{k: v for k, v in data.items() if k in cls.__dataclass_fields__})
        cfg.lock_aspect()
        return cfg


@dataclass
class DeviceConfig:
    output_mode: OutputMode = "RELATIVE"
    scale: float | None = None
    swap_xy: bool = False
    invert_x: bool = False
    invert_y: bool = False
    absolute: AbsoluteConfig = field(default_factory=AbsoluteConfig)

    def to_dict(self) -> dict[str, Any]:
        data = asdict(self)
        return data

    @classmethod
    def from_dict(cls, data: dict[str, Any] | None) -> DeviceConfig:
        if not data:
            return default_device_config()
        absolute = AbsoluteConfig.from_dict(data.get("absolute"))
        return cls(
            output_mode=data.get("output_mode", "RELATIVE"),
            scale=data.get("scale"),
            swap_xy=bool(data.get("swap_xy", False)),
            invert_x=bool(data.get("invert_x", False)),
            invert_y=bool(data.get("invert_y", False)),
            absolute=absolute,
        )


@dataclass
class Connection:
    name: str
    ip: str
    auto_connect: bool = False
    device_config: DeviceConfig = field(default_factory=DeviceConfig)
    id: str = field(default_factory=lambda: str(uuid.uuid4()))

    def to_dict(self) -> dict[str, Any]:
        return {
            "id": self.id,
            "name": self.name,
            "ip": self.ip,
            "auto_connect": self.auto_connect,
            "device_config": self.device_config.to_dict(),
        }

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> Connection:
        return cls(
            id=data["id"],
            name=data["name"],
            ip=data["ip"],
            auto_connect=bool(data.get("auto_connect", False)),
            device_config=DeviceConfig.from_dict(data.get("device_config")),
        )


def default_absolute_config() -> AbsoluteConfig:
    return AbsoluteConfig()


def default_device_config() -> DeviceConfig:
    return DeviceConfig()
