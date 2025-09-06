#!/bin/sh

docker run --rm -it \
  -v "$PWD/openwrt-sdk-22.03.0-x86-generic_gcc-11.2.0_musl.Linux-x86_64":/openwrt-sdk \
  -v "$PWD/.ccache":/ccache \
  -w /openwrt-sdk \
  openwrt-sdk:22.03
