#!/usr/bin/env bash

set -euxo pipefail

rm -rf dist/

CROSS_32="${CROSS_32:-cross-clang-mingw-32.ini}"
CROSS_64="${CROSS_64:-cross-clang-mingw-64.ini}"

# x86
meson setup --cross-file "$CROSS_32" build32
# without `--tags runtime`, the .a files are also installed
meson install  -C build32 --destdir ../dist/32bit --tags runtime,doc

# x86_64
meson setup --cross-file "$CROSS_64" build64
meson install  -C build64 --destdir ../dist/64bit --tags runtime,doc

# docs
cp README.md dist/
# only copy MOD_README, I might have something in data_mods for testing
mkdir -p dist/data_mods
cp data_mods/MOD_README.txt dist/data_mods/

# while we wait for mesonbuild issue #4019 / PR #11954 to be solved, need to
# rename the special builds ourselves
shopt -s globstar
for i in dist/**/special_builds/**/*.dll; do
    mv "$i" "$(echo "$i" | sed 's/ifs_hook.*.dll$/ifs_hook.dll/')"
done

VERSION=$(meson introspect --projectinfo build32 | jq -r .version)

cd dist
zip -9r "ifs_layeredfs_$VERSION.zip" ./*
