#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"
mkdir -p ./data ./logs ./snapshots ./recordings ./conf
if [[ ! -f ./conf/client.ini ]]; then
  cp ./conf/client.ini.template ./conf/client.ini 2>/dev/null || true
fi
exec ./bin/qt_client
