#!/bin/bash
# BGShellBB10 -> BB10 armle-v7 cross build (RELEASE)
#
# Every .bar shipped up to now came from CONFIG+=debug and "make debug":
# -g, no -O2.  bar-descriptor.xml already has a Device-Release configuration
# pointing at arm/o.le-v7/, so only the qmake/make target changes.
#
# Back to debug:   bash build-debug.sh
set -o pipefail
export QNX_HOST=/opt/bbndk/host_10_3_1_12/linux/x86
export QNX_TARGET=/opt/bbndk/target_10_3_1_995/qnx6
export PATH="$QNX_HOST/usr/bin:$PATH"
export QMAKESPEC=unsupported/blackberry-armv7le-qcc

PROJ=/workspace/src/BGShellBB10
cd "$PROJ" || exit 1

echo "=== 1. qmake (release) ==="
mkdir -p arm "$PROJ/arm/o.le-v7"
qmake -spec unsupported/blackberry-armv7le-qcc -o arm/Makefile _BGShellBB10.pro CONFIG+=device CONFIG+=release 2>&1 | tail -20
echo "qmake rc=$?"

echo
echo "=== 2. make -j4 (release) ==="
make -C arm -f Makefile release -j4 2>&1 | tail -80
echo "make rc=${PIPESTATUS[0]}"

echo
echo "=== 3. artifact ==="
ls -l "$PROJ/arm/o.le-v7/" 2>&1 | head
