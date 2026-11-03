#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "$(uname)" == Darwin ]]; then
  open "build/FieldStudio_artefacts/Release/Standalone/OPVYN.app"
else
  exec "build/FieldStudio_artefacts/Release/Standalone/OPVYN" "$@"
fi
