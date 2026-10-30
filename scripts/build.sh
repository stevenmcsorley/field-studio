#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel "${FIELD_BUILD_JOBS:-2}"
if command -v xvfb-run >/dev/null 2>&1 && [[ -z "${DISPLAY:-}" ]]; then
  xvfb-run -a ctest --test-dir build --output-on-failure
else
  ctest --test-dir build --output-on-failure
fi
