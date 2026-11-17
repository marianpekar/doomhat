#!/usr/bin/env bash
# Build and run Doom on the Waveshare 1.44" LCD HAT.

# Usage:
#   ./run.sh                 build (if needed) and run with the default WAD
#   ./run.sh -c              clean build, then run
#   ./run.sh -b              build only
#   ./run.sh -w file.wad     use a specific WAD
#   ./run.sh -s              run with sudo (if GPIO/SPI permission errors)

#   Extra arguments after "--" are passed to the game, e.g.  ./run.sh -- -skill 3

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENGINE="$ROOT/doomgeneric"
MAKEFILE="Makefile.st7735"
BIN="$ENGINE/doomgeneric"
JOBS=2

CLEAN=0
BUILD_ONLY=0
WAD=""

while getopts "cbsw:j:h" opt; do
    case "$opt" in
        c) CLEAN=1 ;;
        b) BUILD_ONLY=1 ;;
        s) USE_SUDO=1 ;;
        w) WAD="$OPTARG" ;;
        j) JOBS="$OPTARG" ;;
        h) sed -n '2,11p' "$0"; exit 0 ;;
        *) exit 1 ;;
    esac
done
shift $((OPTIND - 1))
[ "${1:-}" = "--" ] && shift

# Build
cd "$ENGINE"
if [ "$CLEAN" -eq 1 ]; then
    echo "Cleaning"
    make -f "$MAKEFILE" clean
fi

echo "Building"
make -f "$MAKEFILE" -j"$JOBS"

if [ "$BUILD_ONLY" -eq 1 ]; then
    echo "Build done: $BIN"
    exit 0
fi

# Pick a WAD
if [ -z "$WAD" ]; then
    WAD="$(find "$ROOT" -maxdepth 1 -type f -iname '*.wad' | sort | head -n 1)"
fi

# Run
if [ -z "$WAD" ] || [ ! -f "$WAD" ]; then
    echo "No WAD found. Pass one with: $0 -w /path/to/file.wad" >&2
    exit 1
fi

echo "Running $WAD"
exec "$BIN" -iwad "$WAD" "$@"