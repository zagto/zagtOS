#!/bin/sh
set -e

bochs -q -f emulate/x86_64/bochsrc #&
(sleep 2 && build/DebugBridge/DebugBridge)
