#!/bin/sh
set -e
rm -f out/disk.vdi
VBoxManage convertfromraw out/disk.img out/disk.vdi --format VDI --uuid "da082bea-b016-4d9d-a011-93d8992f04a4"
VBoxManage startvm --putenv VBOX_GUI_DBG_ENABLED=true Test &
build/DebugBridge/DebugBridge
