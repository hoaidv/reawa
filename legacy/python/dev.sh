#!/usr/bin/env bash

set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARENT_DIR="$(dirname "$HERE")"
PYTHON_BIN="$HERE/.venv/bin/python"

if [ -x "$PYTHON_BIN" ]; then
  PYTHON="$PYTHON_BIN"
else
  PYTHON="${PYTHON:-python3}"
fi

cd "$HERE"
export PYTHONPATH="$PARENT_DIR${PYTHONPATH:+:$PYTHONPATH}"

exec "$PYTHON" -m reawa "$@"
