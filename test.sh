#!/usr/bin/env bash

meson test -C build64 --print-errorlogs "$@"
