#!/bin/bash
# Force a full release rebuild: the flags in _BGShellBB10.pro changed
# (-fno-strict-aliasing), and make only looks at timestamps.
set -o pipefail
PROJ=/workspace/src/BGShellBB10
find "$PROJ/arm/o.le-v7" -name '*.o' -delete 2>/dev/null
bash /workspace/local/bbndk-build/build-release.sh
