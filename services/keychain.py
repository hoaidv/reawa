from __future__ import annotations

import keyring

SERVICE_NAME = "remarkable-rm2"


class KeychainStore:
    def save_password(self, connection_id: str, password: str) -> None:
        keyring.set_password(SERVICE_NAME, connection_id, password)

    def get_password(self, connection_id: str) -> str | None:
        return keyring.get_password(SERVICE_NAME, connection_id)

    def delete_password(self, connection_id: str) -> None:
        try:
            keyring.delete_password(SERVICE_NAME, connection_id)
        except keyring.errors.PasswordDeleteError:
            pass
