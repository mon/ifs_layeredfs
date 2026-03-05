#!/bin/bash

# highly recommend using podman-docker instead of docker, no "the build folder is owned by root" issues

set -eu

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

docker build --pull "$SCRIPT_DIR/docker" -t layeredfs/deps --platform linux/x86_64

docker run -it --rm -v "$SCRIPT_DIR:/work" layeredfs/deps ./build.sh "$@"
docker run -it --rm -v "$SCRIPT_DIR:/work" layeredfs/deps ./test.sh "$@"
