from __future__ import annotations

import keyring

SERVICE_NAME = "Reawa"
LEGACY_SERVICE_NAME = "remarkable-rm2"


class KeychainStore:
    def save_password(self, connection_id: str, password: str) -> None:
        keyring.set_password(SERVICE_NAME, connection_id, password)

    def get_password(self, connection_id: str) -> str | None:
        password = keyring.get_password(SERVICE_NAME, connection_id)
        if password is not None:
            return password

        legacy = keyring.get_password(LEGACY_SERVICE_NAME, connection_id)
        if legacy is None:
            return None

        self.save_password(connection_id, legacy)
        try:
            keyring.delete_password(LEGACY_SERVICE_NAME, connection_id)
        except keyring.errors.PasswordDeleteError:
            pass
        return legacy

    def delete_password(self, connection_id: str) -> None:
        for service in (SERVICE_NAME, LEGACY_SERVICE_NAME):
            try:
                keyring.delete_password(service, connection_id)
            except keyring.errors.PasswordDeleteError:
                pass
