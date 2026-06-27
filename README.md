# Reawa

**Reawa** (Remarkable as Wacom) is a macOS menu bar app that turns a reMarkable 2 into a pen input device for your Mac.

## Supported reMarkable devices


| Device                 | Status                        |
| ---------------------- | ----------------------------- |
| **reMarkable 2** (USB) | Supported and actively tested |


**Calling for testers** on other reMarkable devices (for example reMarkable 1, Paper Pro, or Move). If you try Reawa on a different model, please [open an issue](https://github.com/hoaidv/reawa/issues) with your device, firmware version, and what worked or failed.

**Calling for testers** on different macOS versions. Binary releases are built on macOS 15 (Apple Silicon). The app declares a minimum of macOS 12.0, but that combination has not been fully validated — reports from Monterey, Ventura, Sonoma, and Sequoia on both Apple Silicon and Intel are welcome.

## For users

### Install

Download `Reawa.app` from [Releases](https://github.com/hoaidv/reawa/releases) (when available), move it to **Applications**, and open it. Reawa runs from the menu bar (pen-on-tablet icon).

On your MacBook, grant **Accessibility** permission when prompted (**System Settings → Privacy & Security → Accessibility**). `Reawa` needs this to move the cursor and snap to application windows.

### Connect your tablet

1. **Enable SSH on the reMarkable.** On the tablet, turn on SSH (this may require accepting developer-mode terms, depending on your firmware).
2. **Find the root password** on the tablet: **Settings → Help → Copyrights and licenses**. Copy it — you need it once for setup.
3. **Plug the tablet into your Mac via USB.** Reawa talks to the tablet over USB Ethernet (not Wi‑Fi).
4. In the menu bar, choose **Open** to open the settings window.
5. Click **Scan devices** to refresh the **Discovered** list, or enter the connection details manually:
  - **IP** — usually `10.11.99.1` when connected over USB. **Scan devices** or a macOS notification can pre-fill this when the tablet is found.
  - **Password** — the **root SSH password** from **Settings → Help → Copyrights and licenses**. Reawa stores it in the macOS Keychain and uses it only to install a per-device SSH key on first connect.
6. Click **Add connection**, then connect from the menu bar or enable **Auto-connect** for plug-and-play.

> **SSH security:** Reawa only saves the SSH key on your machine and does not share it with anyone. On first connect it installs the public key on your tablet; the private key and password never leave your Mac.

After the first successful setup, Reawa uses the installed SSH key — you do not need to enter the password again unless you reset SSH on the tablet.

### Use

- **Relative** mode (default): pen hover moves the cursor like a trackpad; touch clicks and drags.
- **Absolute** mode: maps the tablet to a chosen application window (pick a window when switching modes).
- Toggle connections and modes from the menu bar. Only one tablet can be active at a time.

## For developers

### Prerequisites

- macOS 12 or later (development is done on macOS 15, Apple Silicon)
- Python 3.14 (or the version used by the project `.venv`)
- Xcode command-line tools (`xcode-select --install`)

### Build from source

```bash
git clone https://github.com/hoaidv/reawa.git
cd reawa

python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt

# Stage sources so imports resolve as `remarkable` (same rsync step as packaging)
mkdir -p packaging/src
rsync -a --prune-empty-dirs \
  --exclude='packaging' --exclude='.venv' --exclude='.ssh' \
  --exclude='__pycache__' --exclude='.docs' --exclude='.git' \
  --include='*/' --include='*.py' --exclude='*' \
  ./ packaging/src/remarkable/

PYTHONPATH=packaging/src python -m remarkable
```

### Package `Reawa.app`

The release bundle is built with [py2app](https://py2app.readthedocs.io/) via `packaging/build.sh`. It stages a sanitized copy of the Python sources (no `.venv`, SSH keys, or secrets) and produces `packaging/dist/Reawa.app`.

```bash
source .venv/bin/activate
pip install py2app
./packaging/build.sh
```

Output: `packaging/dist/Reawa.app`

When distributing the app, include `LICENSE`, `NOTICE`, and `THIRD_PARTY_LICENSES.md` with the release. The py2app configuration bundles these files into `Contents/Resources/legal`.

## License

This project is licensed under the [MIT License](LICENSE).

See [NOTICE](NOTICE) for trademark disclaimers and third-party attribution.
See [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md) for binary release license notes.

## Trademarks

reMarkable® is a registered trademark of reMarkable AS. Reawa is an independent project and is not affiliated with, endorsed by, or sponsored by reMarkable AS.