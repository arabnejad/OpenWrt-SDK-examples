# OpenWrt SDK Local Development (Docker + QEMU)

This tutorial documents a clean, repeatable workflow for local OpenWrt package development using:
- A **Docker** container that hosts the OpenWrt **SDK** (so your host stays clean).
- A **QEMU** VM running OpenWrt **x86/generic** (so you can install and run your `.ipk`s quickly).

It captures the **exact versions** and steps used in this setup (OpenWrt **22.03.0**, **x86/generic**) and includes battle‑tested tips from troubleshooting along the way.

---

## 1) Versions & Downloads (exact artifacts)

### 1.1 OpenWrt SDK (x86/generic, 22.03.0)
- **URL**:
  `https://downloads.openwrt.org/releases/22.03.0/targets/x86/generic/openwrt-sdk-22.03.0-x86-generic_gcc-11.2.0_musl.Linux-x86_64.tar.xz`

### 1.2 OpenWrt VM disk image (x86/generic, ext4 combined)
- **URL**:
  `https://downloads.openwrt.org/releases/22.03.0/targets/x86/generic/openwrt-22.03.0-x86-generic-generic-ext4-combined.img.gz`

### 1.3 Prepare the image
```bash
gunzip openwrt-22.03.0-x86-generic-generic-ext4-combined.img.gz
mv openwrt-22.03.0-x86-generic-generic-ext4-combined.img openwrt_86.img

# (optional) give yourself more space; extroot auto-expands on first boot
qemu-img resize openwrt_86.img 256M
```

> Keep SDK + VM image in the same project directory for convenience.

Example project layout:
```
~/openwrt-dev/
├── openwrt-sdk-22.03.0-x86-generic_gcc-11.2.0_musl.Linux-x86_64/
├── openwrt_86.img
├── Dockerfile
├── bashrc_extra.sh
├── run.sh
└── qemu_start.sh
```

---

## 2) Prerequisites

- **Docker** installed and working:
  ```bash
  docker run hello-world
  ```
- **QEMU** for x86:
  ```bash
  sudo apt-get update && sudo apt-get install -y qemu-system-x86
  ```
- Tools you’ll likely want on the host for testing:
  ```bash
  sudo apt-get install -y netcat-openbsd ncat socat
  ```

---

## 3) Dockerized OpenWrt SDK

You have the following helper files in the project root:
- **Dockerfile** – builds a dev‑friendly image with SDK deps, editors (vim/vi/nano), toned‑down prompt, and helper functions appended to `~/.bashrc`.
- **bashrc_extra.sh** – sets a colored prompt, exports `nproc=4`, aliases `quilt` to the system binary, and adds a `cd_home` shortcut
- **run.sh** – starts an interactive container and **bind‑mounts** the SDK root to `/openwrt-sdk` inside the container.
- **qemu_start.sh** – launches the OpenWrt VM with the port forwards you need for development.

### 3.1 Build the container
```bash
docker build -t openwrt-sdk:22.03 .
```

### 3.2 Enter the container
```bash
./run.sh
# You are now inside the container, working under /openwrt-sdk
```

### 3.3 Verify the environment
Inside the container:
```bash
echo "$STAGING_DIR"    # should be set by the SDK
command -v i486-openwrt-linux-musl-gcc || true
echo "$PATH"           # should include staging_dir/host/bin and toolchain bins
```

---

## 4) QEMU VM (OpenWrt x86/generic)

**qemu_start.sh** (what we use):
```sh
#!/bin/sh
qemu-system-i386 \
  -m 256  -nographic \
  -drive file=openwrt_86.img,format=raw,if=ide \
  -nic user,model=e1000,hostfwd=tcp::2222-:22,hostfwd=udp::9000-:9000
```
This maps:
- **Host 2222/TCP → VM 22/TCP** (SSH)
- **Host 9000/UDP → VM 9000/UDP** (handy for testing datagram servers)

Run it:
```bash
./qemu_start.sh
```

On first boot inside the VM:
```sh
passwd                       # set root password
uci set network.lan.proto='dhcp'
uci commit network
/etc/init.d/network restart

uci set system.@system[0].hostname='openwrt-sdk-vm'
uci commit system
/etc/init.d/dropbear restart # SSH server (Dropbear) restart for good measure
```

SSH from host:
```bash
ssh -p 2222 root@127.0.0.1
```

> If you plan to send UDP from the host to the VM (e.g., port 9000), allow it in the firewall:
```sh
uci add firewall rule
uci set firewall.@rule[-1].name='Allow-udp-9000'
uci set firewall.@rule[-1].src='wan'
uci set firewall.@rule[-1].proto='udp'
uci set firewall.@rule[-1].dest_port='9000'
uci set firewall.@rule[-1].target='ACCEPT'
uci commit firewall
/etc/init.d/firewall restart
```

---

## 5) Wire a local feed (absolute path)

Using **absolute** paths avoids empty indices or odd symlinks inside `package/feeds/...`.

Inside the container under `/openwrt-sdk`:
```bash
# Let's assume your local feed lives here:
#   /openwrt-sdk/local-feeds/hamidrepo
# and you copied feeds.conf into /openwrt-sdk

# Update the feed and index it
./scripts/feeds update -f hamidrepo
```

Install package stubs from your feed (this creates `package/feeds/<feed>/<pkg>` symlinks):
```bash
./scripts/feeds install -p hamidrepo <pkg>
```

> If you see “Not overriding core package ‘X’”, add `-f` to force for your own packages:
> `./scripts/feeds install -f -p hamidrepo <pkg>`

---

## 6) Build only what you need

General form:
```bash
make package/feeds/<feed>/<pkg>/{clean,compile} V=s -j"$(nproc)"
```

Examples:
```bash
# Discover packages
./scripts/feeds list hamidrepo | grep -i <pkg>

# Build example package
./scripts/feeds install -p hamidrepo myapp
make package/feeds/hamidrepo/myapp/{clean,compile} V=s -j"$(nproc)"
```

Artifacts land in:
```
./bin/packages/<arch>/<feed>/<pkg>_*.ipk
```
Common arch values for x86 targets:
- `i386_pentium4` → x86 **generic** 32‑bit
- `x86_64` → x86 **64‑bit**

> Installing a mismatched `.ipk` in the VM will fail with “incompatible architectures”.

---

## 7) Copy `.ipk` into the VM and install

**Dropbear** prefers legacy scp protocol; use `-O`:
```bash
# From the host
scp -O -P 2222 ./bin/packages/*/*/<yourpkg>_*.ipk root@127.0.0.1:/tmp/
```

Inside the VM:
```sh
opkg update || true
opkg install /tmp/<yourpkg>_*.ipk
```

---

## 8) Patches with `quilt`

Two ways to integrate patches:

### 8.1 Static (preferred for feeds)
Place patches under the package’s `patches/` directory (alongside the package `Makefile`). They are applied automatically during `Build/Prepare` via `$(Build/Patch)`.

```
local-feeds/hamidrepo/<pkg>/
├── Makefile
├── patches/
│   ├── 0001-...
│   └── 0002-...
├── src/...
└── include/...
```

### 8.2 Interactive in the build tree
After a first compile, go to:
```
build_dir/target-*/<pkg>-<ver>/
```

Run quilt with a known patch store:
```bash
export QUILT_PATCHES="$(CURDIR)/patches"
quilt new 0001-some-change.patch
quilt add src/file.c
# edit src/file.c
quilt refresh
```

Rebuild:
```bash
make package/feeds/<feed>/<pkg>/{clean,compile} V=s -j"$(nproc)"
```

> If the SDK ships a wrapper quilt with minimal `--help`, call the system `/usr/bin/quilt` (on your host) to prepare patches, then drop them into your package’s `patches/` directory.

---

## 9) Known pitfalls (and fixes)

- **Empty `feeds/*.index` after `feeds update`**
  Ensure `feeds.conf` uses an **absolute** path for `src-link`:
  ```
  src-link hamidrepo /openwrt-sdk/local-feeds/hamidrepo
  ```

- **`No rule to make target 'package/feeds/.../<pkg>/compile'`**
  You forgot `./scripts/feeds install -p <feed> <pkg>` (creates the symlink under `package/feeds/<feed>/<pkg>`).

- **BusyBox `nc` lacks options**
  Use **OpenBSD netcat** on your host; inside the VM prefer `socat` for UDP tests.

- **SCP fails with “not found /usr/libexec/sftp-server”**
  Use legacy scp protocol with Dropbear: `scp -O -P 2222 ...`

- **Arch mismatch on install**
  Build with the SDK matching your VM target (e.g., `i386_pentium4` for x86/generic 32‑bit).

- **Missing C++ runtime at execution**
  Install `libstdcpp` in the VM: `opkg update && opkg install libstdcpp`.

---
