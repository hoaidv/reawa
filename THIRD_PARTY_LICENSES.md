# Third-Party Licenses

Reawa depends on third-party open-source packages. This file is a release
checklist and attribution summary; when publishing a binary build, include the
full license texts from the exact dependency versions that were bundled.

## Runtime Dependencies

- Paramiko: LGPL-2.1
- PyObjC: MIT License
- rumps: BSD License
- keyring: MIT License
- cryptography: Apache License 2.0 or BSD License
- cffi: MIT License
- PyNaCl: Apache License 2.0

## Binary Distribution Checklist

- Include `LICENSE`, `NOTICE`, and this file in the app bundle and release
  archive.
- Include the full license texts for the exact versions of bundled third-party
  packages.
- For Paramiko's LGPL-2.1 license, make it possible for recipients to replace
  or modify Paramiko and rebuild or relink the application.
- Keep source code or a written source offer available for any LGPL-covered
  components included in a frozen binary release.

