#!/bin/bash
# Incremental release build (keeps existing .o, only recompiles what changed).
export QNX_HOST=/opt/bbndk/host_10_3_1_12/linux/x86
export QNX_TARGET=/opt/bbndk/target_10_3_1_995/qnx6
export PATH="$QNX_HOST/usr/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
export QMAKESPEC=unsupported/blackberry-armv7le-qcc
cd /workspace/src/BGShellBB10 || exit 1
make -C arm -f Makefile release -j6 > build3.log 2>&1
echo "make_rc=$?" >> build3.log
ls -l arm/o.le-v7/BGShellBB10 >> build3.log
md5sum arm/o.le-v7/BGShellBB10 >> build3.log
echo "=== DONE ===" >> build3.log
