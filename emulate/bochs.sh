#!/bin/sh
set -e

bochs -q -f emulate/bochsrc #&
(sleep 2 && build/DebugBridge/DebugBridge)
