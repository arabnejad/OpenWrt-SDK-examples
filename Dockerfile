# Dockerfile for OpenWrt SDK 22.03 (x86/generic, musl, GCC 11.2.0)
# Place this file next to the extracted SDK directory.

FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

# Core build prereqs for OpenWrt SDK usage
# (gcc/make toolchain, ncurses for menuconfig, zlib, ssl, python3, git, svn, etc.)
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential gawk flex bison \
    libncurses5-dev zlib1g-dev libssl-dev \
    ca-certificates file xz-utils unzip rsync wget curl tree \
    python3 python3-distutils python3-venv python-is-python3 \
    git subversion ccache gettext \
    time pkg-config \
    vim nano less \
    bsdextrautils patch quilt \
    && rm -rf /var/lib/apt/lists/*

# keep ccache in a persistent mount (Optional)
ENV CCACHE_DIR=/ccache

# Make 'vi' and 'editor' use Vim; set default editor
RUN update-alternatives --set vi /usr/bin/vim.basic \
    && update-alternatives --set editor /usr/bin/vim.basic
ENV EDITOR=vim VISUAL=vim

# --- Mount point & defaults ---
ENV SDK_DIR=/openwrt-sdk
ENV STAGING_DIR=${SDK_DIR}/staging_dir

# Copy bashrc_extra file into the image
COPY bashrc_extra.sh /etc/profile.d/openwrt-sdk.sh

RUN set -eux; \
    chmod 0644 /etc/profile.d/openwrt-sdk.sh; \
    # ensure ~/.bashrc sources it (covers interactive shells)
    printf '\n# OpenWrt SDK helpers\nsource /etc/profile.d/openwrt-sdk.sh\n' >> /root/.bashrc

# Helper that wires PATH for the SDK each time the container runs.
# - Adds host tools (staging_dir/host/bin)
# - Auto-detects the toolchain-* directory and adds its bin
# - Adds libc/usr/bin if present (some SDKs expose extra target utils there)
RUN printf '%s\n' \
    '#!/usr/bin/env bash' \
    'set -e' \
    ': "${SDK_DIR:=/openwrt-sdk}"' \
    ': "${STAGING_DIR:=${SDK_DIR}/staging_dir}"' \
    'export STAGING_DIR' \
    'HOST_BIN="${STAGING_DIR}/host/bin"' \
    'TOOLCHAIN_BIN="$(find "${STAGING_DIR}" -maxdepth 1 -type d -name "toolchain-*" -printf "%p/bin\n" -quit || true)"' \
    'LIBC_BIN="$(find "${STAGING_DIR}" -maxdepth 2 -type d -path "*/libc/usr/bin" -print -quit || true)"' \
    'NEWPATH=""' \
    '[[ -d "${HOST_BIN}" ]] && NEWPATH="${HOST_BIN}"' \
    '[[ -n "${TOOLCHAIN_BIN}" && -d "${TOOLCHAIN_BIN}" ]] && NEWPATH="${NEWPATH:+${NEWPATH}:}${TOOLCHAIN_BIN}"' \
    '[[ -n "${LIBC_BIN}" && -d "${LIBC_BIN}" ]] && NEWPATH="${NEWPATH:+${NEWPATH}:}${LIBC_BIN}"' \
    'export PATH="${NEWPATH:+${NEWPATH}:}${PATH}"' \
    'exec "$@"' \
    > /usr/local/bin/sdk-env && chmod +x /usr/local/bin/sdk-env



# Default working directory inside the SDK
WORKDIR ${SDK_DIR}

# Use the env wrapper so PATH is always correct in interactive shells
ENTRYPOINT ["/usr/local/bin/sdk-env"]
CMD ["/bin/bash"]
