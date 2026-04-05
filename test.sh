#!/usr/bin/env bash

set -eu

meson test -C build32 --print-errorlogs "$@"
meson test -C build64 --print-errorlogs "$@"
