FROM montymintypie/llvm-mingw-xp:22

RUN set -eux; \
    dpkg --add-architecture i386; \
    apt-get update -qq; \
    DEBIAN_FRONTEND="noninteractive" apt-get install -qqy --no-install-recommends \
        meson jq python3-pefile python3-requests wine wine32 wine64 xz-utils; \
    apt-get clean -y; \
    rm -rf /var/lib/apt/lists/*;

# Run wineboot just to setup the default WINEPREFIX so we don't do it every
# container run
RUN wine wineboot --init

RUN set -eux; \
    curl -fsSL "https://api.github.com/repos/mon/windows-dll-compat-checker/releases/latest" \
        | jq -r '.assets[] | select(.name | contains("linux-x86_64")) | .browser_download_url' \
        | xargs curl -fsSL -o /tmp/dll_compat_checker.tar.xz; \
    tar -xJf /tmp/dll_compat_checker.tar.xz -C /usr/local/bin/; \
    chmod +x /usr/local/bin/windows_dll_compat_checker; \
    rm /tmp/dll_compat_checker.tar.xz;

ENV CROSS_32=/opt/llvm-mingw/toolchain-files/meson/i686-mingw32-clang.ini
ENV CROSS_64=/opt/llvm-mingw/toolchain-files/meson/x86_64-mingw32-clang.ini

WORKDIR /work
