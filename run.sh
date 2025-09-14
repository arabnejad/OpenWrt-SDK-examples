#!/bin/sh
set -e

X86_SDK="./openwrt-sdk-22.03.0-x86-generic_gcc-11.2.0_musl.Linux-x86_64"

mkdir -p .ccache

# docker build -t openwrt-sdk:22.03 .

docker run --rm -it \
  -v "$PWD/.ccache":/ccache \
  -v "$PWD/$X86_SDK":/work/sdk-x86 \
  -v "$PWD/local-feeds":/work/local-feeds \
  -w /work \
  openwrt-sdk:22.03
