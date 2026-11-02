#!/bin/bash
# Package the RELEASE binary (arm/o.le-v7) into BGShellBB10.bar.
set -o pipefail
export QNX_HOST=/opt/bbndk/host_10_3_1_12/linux/x86
export QNX_TARGET=/opt/bbndk/target_10_3_1_995/qnx6
export PATH="$QNX_HOST/usr/bin:$PATH"
# JDK 21 dropped the Security Manager and hid the internal Xerces parser the
# BB10 packager uses.  Both have to be re-opened explicitly.
export JAVA_TOOL_OPTIONS="-Djava.security.manager=allow \
 --add-exports=java.xml/com.sun.org.apache.xerces.internal.parsers=ALL-UNNAMED \
 --add-exports=java.xml/com.sun.org.apache.xerces.internal.dom=ALL-UNNAMED \
 --add-exports=java.xml/com.sun.org.apache.xerces.internal.jaxp=ALL-UNNAMED \
 --add-exports=java.xml/com.sun.org.apache.xerces.internal.util=ALL-UNNAMED \
 --add-opens=java.xml/com.sun.org.apache.xerces.internal.parsers=ALL-UNNAMED \
 --add-opens=java.xml/com.sun.org.apache.xerces.internal.dom=ALL-UNNAMED"

PROJ=/workspace/src/BGShellBB10
cd "$PROJ" || exit 1
BIN=arm/o.le-v7/BGShellBB10

echo "=== file ==="
file "$BIN"
echo "=== size / md5 ==="
ls -l "$BIN"; md5sum "$BIN"
echo "=== optimisation / debug leftovers ==="
readelf -S "$BIN" 2>/dev/null | grep -E "debug_info|debug_abbrev" | head -5

echo "=== pack .bar (devMode, Device-Release) ==="
rm -f BGShellBB10.bar
blackberry-nativepackager -devMode -package BGShellBB10.bar bar-descriptor.xml -configuration Device-Release 2>&1 | tail -20
echo "packager rc=$?"
ls -l BGShellBB10.bar 2>&1
echo "=== native dir ==="
unzip -l BGShellBB10.bar 2>/dev/null | grep -E "native/|MANIFEST" | head -12
echo "=== MANIFEST ==="
unzip -p BGShellBB10.bar META-INF/MANIFEST.MF 2>/dev/null | head -12
