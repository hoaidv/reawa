# Third-Party Licenses

Reawa is a native Swift application with no bundled third-party open-source
packages. This file is a release checklist and attribution summary.

## Runtime Dependencies

The application uses macOS system frameworks and invokes system tools at
runtime. These components are not bundled with Reawa and are provided by the
operating system:

- OpenSSH (`ssh`, `ssh-keygen`) — OpenSSH License
- Apple system frameworks — subject to macOS and Xcode license terms

## Binary Distribution Checklist

- Include `LICENSE`, `NOTICE`, and this file in the app bundle and release
  archive.
- Do not imply Apple, reMarkable, or Wacom affiliation (see `NOTICE`).
