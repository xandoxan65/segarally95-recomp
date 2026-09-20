#!/usr/bin/env bash
# Run lift pilot and show output paths.
set -euo pipefail
cd "$(dirname "$0")/.."
export PYTHONPATH="${PYTHONPATH:-}:$(pwd)"
./build.sh lift --pilot
echo ""
echo "Lifted C:"
ls -la src/libc/*.c 2>/dev/null || true
