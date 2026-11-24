#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
root="$PWD"
export PATH="$root/.local/env/bin:$PATH"
mkdir -p reports
cmake -S . -B build-mac -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$root/.local/env" -DOCCT_ENABLE_GUI_TESTS=ON 2>&1 | tee reports/configure-mac.log
cmake --build build-mac --parallel 6 2>&1 | tee reports/build-mac.log
ctest --test-dir build-mac --output-on-failure --output-junit "$root/reports/ctest-mac.xml" 2>&1 | tee reports/ctest-mac.log
echo "Open: open build-mac/OcctQtStarter.app"
