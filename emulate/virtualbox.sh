#!/bin/sh
set -e
rm out/disk.vdi
VBoxManage convertfromraw out/disk.img out/disk.vdi --format VDI --uuid "da082bea-b016-4d9d-a011-93d8992f04a4"
netcat -l -p 30000 localhost &
VBoxManage startvm Test
