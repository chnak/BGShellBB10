#!/bin/bash
# Build the QR scanner helper (bgqrcam).
#
# The camera on BlackBerry 10 can only be opened through the Cascades
# multimedia API, and Cascades controls only exist inside a Cascades
# application.  BGShellBB10 is a plain Qt Widgets program, so the camera
# runs in this helper instead: a Cascades application with no window that
# pulls the preview frames into buffers and lets the barcode detector read
# them.  It prints "QRCODE:<text>" on stdout and exits.
set -o pipefail

export QNX_HOST=${QNX_HOST:-/opt/bbndk/host_10_3_1_12/linux/x86}
export QNX_TARGET=${QNX_TARGET:-/opt/bbndk/target_10_3_1_995/qnx6}
export PATH="$QNX_HOST/usr/bin:$PATH"

PROJ=${PROJ:-/workspace/src/BGShellBB10}
OUT="$PROJ/arm/o.le-v7"
T="$QNX_TARGET"

cd "$PROJ" || exit 1
mkdir -p "$OUT"

QCC="$QNX_HOST/usr/bin/qcc"
MOC="$QNX_HOST/usr/bin/moc"

echo "=== 1. moc ==="
$MOC "$PROJ/helper/bgqrcam.cpp" -o "$PROJ/helper/bgqrcam.moc" || exit 1

echo "=== 2. qcc ==="
$QCC -Vgcc_ntoarmv7le -lang-c++ -o "$OUT/bgqrcam" "$PROJ/helper/bgqrcam.cpp" \
  -I"$T/usr/include/qt4" \
  -I"$T/usr/include/qt4/QtCore" \
  -I"$T/usr/include/qt4/QtGui" \
  -I"$T/usr/include/qt4/QtDeclarative" \
  -I"$T/usr/include/qt4/QtNetwork" \
  -I"$T/usr/include" \
  -I"$PROJ" \
  -I"$PROJ/helper" \
  -L"$T/armle-v7/usr/lib/qt4/lib" \
  -L"$T/armle-v7/usr/lib" \
  -L"$T/armle-v7/lib" \
  -lbbcascadesmultimedia \
  -lzxing \
  -lQtCore -lQtGui -lQtDeclarative -lQtScript -lQtSql \
  -lQtXml -lQtXmlPatterns -lQtNetwork -lQtSvg \
  -lbb -lbbcascades -lbbsystem -lbbplatform -lscreen \
  -Wl,-rpath-link,"$T/armle-v7/lib" \
  -Wl,-rpath-link,"$T/armle-v7/usr/lib" || exit 1

echo "=== 3. artifact ==="
ls -l "$OUT/bgqrcam"
file "$OUT/bgqrcam"
