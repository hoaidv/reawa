"""py2app build for the Reawa menu bar app.

Build (from this directory, using the project venv)::

    ../.venv/bin/python setup.py py2app

Produces ``dist/Reawa.app``. No SSH keys, secrets, or the dev venv are
included: only the discovered Python modules plus the menu-bar icon asset.
"""

import os
import sys

from setuptools import setup

HERE = os.path.dirname(os.path.abspath(__file__))          # remarkable/packaging
PKG_DIR = os.path.dirname(HERE)                            # remarkable
ASSETS = os.path.join(PKG_DIR, "assets")
LEGAL_FILES = [
    os.path.join(PKG_DIR, "LICENSE"),
    os.path.join(PKG_DIR, "NOTICE"),
    os.path.join(PKG_DIR, "THIRD_PARTY_LICENSES.md"),
]

# Build from a sanitized staging copy of the sources (created by build.sh /
# rsync into ./src) that contains only .py files -- never the dev .venv, SSH
# keys, or build output. py2app may copy a package wholesale when it isn't
# zip-safe, so the package it discovers must be clean.
STAGING = os.path.join(HERE, "src")
if not os.path.isdir(os.path.join(STAGING, "remarkable")):
    raise SystemExit(
        "Missing staged sources at %s/remarkable. Run build.sh (it rsyncs the "
        "package's .py files into ./src) before building." % STAGING
    )
sys.path.insert(0, STAGING)

APP = ["Reawa.py"]

# Ship runtime assets and legal notices into Contents/Resources.
DATA_FILES = [
    ("assets", [os.path.join(ASSETS, "menu_icon.png")]),
    ("legal", LEGAL_FILES),
]

OPTIONS = {
    "argv_emulation": False,
    "iconfile": os.path.join(HERE, "app_icon.icns"),
    "plist": {
        "CFBundleName": "Reawa",
        "CFBundleDisplayName": "Reawa",
        "CFBundleIdentifier": "com.howard.reawa",
        "CFBundleVersion": "0.2.0",
        "CFBundleShortVersionString": "0.2.0",
        # Menu-bar agent: no Dock icon at launch (the app manages Dock
        # visibility itself when the settings window opens).
        "LSUIElement": True,
        "NSHighResolutionCapable": True,
        "LSMinimumSystemVersion": "12.0",
    },
    # Copy these as full packages so their compiled extensions / data /
    # backend submodules are bundled intact. NOTE: `remarkable` is deliberately
    # NOT listed here -- py2app copies `packages` directories wholesale, which
    # would pull in the dev .venv, .ssh keys, and __pycache__. Instead the
    # package's modules are discovered by import analysis and zipped, and the
    # one needed asset is shipped via DATA_FILES.
    "packages": [
        "rumps",
        "paramiko",
        "cryptography",
        "nacl",
        "cffi",
        "keyring",
    ],
    "includes": [
        "keyring.backends.macOS",
    ],
    # Trim weight and avoid shipping anything sensitive or unnecessary.
    "excludes": [
        "PyObjCTest",
        "pip",
        "setuptools",
        "wheel",
        "tkinter",
        "test",
        "unittest",
        "pydoc_data",
        "lib2to3",
    ],
}

setup(
    app=APP,
    data_files=DATA_FILES,
    options={"py2app": OPTIONS},
    setup_requires=["py2app"],
)
