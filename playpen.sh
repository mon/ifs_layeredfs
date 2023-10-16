#!/usr/bin/env bash

meson compile -C build64 playpen && ./build64/playpen.exe --layered-verbose
