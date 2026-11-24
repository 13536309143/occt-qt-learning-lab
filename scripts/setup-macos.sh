#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
root="$PWD"
mkdir -p .local/bin reports
arch="$(uname -m)"
case "$arch" in arm64) platform=osx-arm64 ;; x86_64) platform=osx-64 ;; *) exit 1 ;; esac
mm="$root/.local/bin/micromamba"
if [[ ! -x "$mm" ]]; then
  curl -fL --retry 2 --connect-timeout 20 "https://github.com/mamba-org/micromamba-releases/releases/latest/download/micromamba-$platform" -o "$mm"
  chmod +x "$mm"
fi
"$mm" create -y --no-rc -r "$root/.local/mamba" -p "$root/.local/env" -c conda-forge \
  'occt=7.9.3' 'qt6-main=6.8.4' 'cmake>=3.21' ninja 2>&1 | tee reports/dependency-install.log
echo "Next: bash scripts/build-macos.sh"
