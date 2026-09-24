#!/bin/bash
set -u
mkdir -p build
if make -j2 >build/compile.log 2>&1; then
  tail -n 3 build/compile.log
else
  tail -n 45 build/compile.log
  exit 1
fi
