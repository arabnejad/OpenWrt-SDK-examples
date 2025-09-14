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
RUN mkdir -p /ccache && chmod 777 /ccache

# Make 'vi' and 'editor' use Vim; set default editor
RUN update-alternatives --set vi /usr/bin/vim.basic \
    && update-alternatives --set editor /usr/bin/vim.basic
ENV EDITOR=vim VISUAL=vim

# --- Mount point & defaults ---
# Workspace layout where we'll mount both SDKs
#   /work/sdk-x86     -> openwrt-sdk-22.03.0-x86-generic_...
#   /work/sdk-mt7621  -> openwrt-sdk-22.03.0-ramips-mt7621_...
RUN mkdir -p /work
WORKDIR /work


# Copy bashrc_extra file into the image
COPY bashrc_extra.sh /etc/profile.d/openwrt-sdk.sh

RUN set -eux; \
    chmod 0644 /etc/profile.d/openwrt-sdk.sh; \
    # ensure ~/.bashrc sources it (covers interactive shells)
    printf '\n# OpenWrt SDK helpers\nsource /etc/profile.d/openwrt-sdk.sh\n' >> /root/.bashrc

# Use the env wrapper so PATH is always correct in interactive shells
ENTRYPOINT ["/bin/bash", "-lc"]
CMD ["/bin/bash"]
