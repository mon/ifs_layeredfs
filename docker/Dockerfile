FROM montymintypie/llvm-mingw-xp:21

RUN set -eux; \
    dpkg --add-architecture i386; \
    apt-get update -qq; \
    DEBIAN_FRONTEND="noninteractive" apt-get install -qqy --no-install-recommends \
        meson jq wine wine32; \
    apt-get clean -y; \
    rm -rf /var/lib/apt/lists/*;

# Run wineboot just to setup the default WINEPREFIX so we don't do it every
# container run
RUN wine wineboot --init

ENV CROSS_32=/opt/llvm-mingw/toolchain-files/meson/i686-mingw32-clang.ini
ENV CROSS_64=/opt/llvm-mingw/toolchain-files/meson/x86_64-mingw32-clang.ini

WORKDIR /work
