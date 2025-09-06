#!/bin/sh

qemu-system-i386 \
  -m 256  -nographic \
  -drive file=openwrt_86.img,format=raw,if=ide \
  -nic user,model=e1000,hostfwd=tcp::2222-:22,hostfwd=udp::9000-:9000

