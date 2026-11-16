#!/usr/bin/env bash
# Build and run the desk test: the firmware's serial behaviour, on a PC.
# Usage: desk_test/run.sh [scenario]     VERBOSE=1 lists passing checks too.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
repo="$(dirname "$here")"
build="$here/build"
mkdir -p "$build"

# The Builder's page/element enum, so the test uses the same IDs as the firmware.
sed -n '/^\/\/<Enum !Start!>/,/^\/\/<Enum !End!>/p' "$repo/include/test_GSLC.h" > "$build/gslc_ids.h"

# stubs/ comes first so its version.h replaces the generated include/version.h.
g++ -std=gnu++17 -O1 -g -w \
    -DDRV_DISP_TFT_ESPI \
    -I "$here/stubs" -I "$repo/include" -I "$build" -I "$here" \
    "$here/desk_test.cpp" "$here/stubs/stubs.cpp" \
    "$repo/src/main.cpp" "$repo/src/Config.cpp" "$repo/src/FaceRenderer.cpp" \
    "$repo/src/DisplayInit.cpp" "$repo/src/LdrSensor.cpp" \
    "$repo/src/MenuCallbacks.cpp" "$repo/src/UIManager.cpp" \
    -o "$build/desk_test"

"$build/desk_test" "$@"
