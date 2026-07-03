"""py2app entry point for the Reawa menu bar app.

The bundle filename is derived from this script's name, so keeping it
``Reawa.py`` yields ``Reawa.app``.
"""

from __future__ import annotations

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
for candidate in (os.path.join(HERE, "src"), os.path.dirname(HERE)):
    if candidate not in sys.path:
        sys.path.insert(0, candidate)

from reawa.app import main

if __name__ == "__main__":
    main()
