#!/bin/bash
export QNX_HOST=/opt/bbndk/host_10_3_1_12/linux/x86
export QNX_TARGET=/opt/bbndk/target_10_3_1_995/qnx6
export PATH="$QNX_HOST/usr/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
export JAVA_TOOL_OPTIONS="-Djava.security.manager=allow --add-exports=java.xml/com.sun.org.apache.xerces.internal.parsers=ALL-UNNAMED --add-exports=java.xml/com.sun.org.apache.xerces.internal.dom=ALL-UNNAMED --add-exports=java.xml/com.sun.org.apache.xerces.internal.jaxp=ALL-UNNAMED --add-exports=java.xml/com.sun.org.apache.xerces.internal.util=ALL-UNNAMED --add-opens=java.xml/com.sun.org.apache.xerces.internal.parsers=ALL-UNNAMED --add-opens=java.xml/com.sun.org.apache.xerces.internal.dom=ALL-UNNAMED"
cd /workspace/src/BGShellBB10 || exit 1
rm -f BGShellBB10.bar
blackberry-nativepackager -devMode -package BGShellBB10.bar bar-descriptor.xml -configuration Device-Release > pack2.log 2>&1
echo "pack_rc=$?" >> pack2.log
ls -l BGShellBB10.bar >> pack2.log 2>&1
md5sum BGShellBB10.bar >> pack2.log 2>&1
unzip -p BGShellBB10.bar native/BGShellBB10 > /tmp/pk_bin
md5sum /tmp/pk_bin >> pack2.log 2>&1
md5sum arm/o.le-v7/BGShellBB10 >> pack2.log 2>&1
echo "=== DONE ===" >> pack2.log
