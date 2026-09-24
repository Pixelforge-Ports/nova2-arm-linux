#!/bin/bash
set -u
ulimit -c 0
export SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy LIBGL_ALWAYS_SOFTWARE=1
export NOVA2_DESKTOP_GL=1 NOVA2_TEST_FRAMES="${NOVA2_TEST_FRAMES:-120}" LOADER_TRACE=1
timeout 90 qemu-arm -L /usr/arm-linux-gnueabihf build/nova2 build/data/donor >build/startup.log 2>&1
status=$?
tail -n 65 build/startup.log
exit "$status"
