#!/usr/bin/env bash

set -euxo pipefail

# x86
meson setup --cross-file cross-i686-w64-mingw32.txt build32
meson compile -C build32

# x86_64
meson setup --cross-file cross-x86_64-w64-mingw32.txt build64
meson compile -C build64
