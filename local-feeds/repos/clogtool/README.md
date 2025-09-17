# clogtool (OpenWrt package)

A tiny **C** CLI that parses netfilter/iptables-style log lines and reports top talkers by source IP. It ranks by lines or bytes (prefers `BYTES=...`, falls back to `LEN=...`).

---

## 1) Run OpenWrt in QEMU (SSH port forward only)

Run this command. It forwards host **TCP 2222 → guest 22 (SSH)**.

```bash
qemu-system-i386 \
  -m 256 -nographic \
  -drive file=openwrt.img,format=raw,if=ide \
  -nic user,model=e1000,hostfwd=tcp::2222-:22
```

> 🔎 If your VM is 64-bit, use an **x86_64** image and matching **SDK**. If it’s 32-bit generic, use the **i386_pentium4** SDK.

---

## 2) Prepare & compile `clogtool` in the OpenWrt SDK

From the **SDK root**:

```bash
# Ensure your local feed is wired up in feeds.conf, e.g.:
# src-link local_feeds /work/local-feeds/repos/

# 1) Update feeds
./scripts/feeds update -f local_feeds packages

# 2) Install build dependencies and your package metadata
./scripts/feeds install -f -p local_feeds clogtool

# 3) Clean + build only this package
make package/feeds/local_feeds/clogtool/{clean,prepare,compile} V=s -j"$(nproc)"

```

Artifacts (example for 22.03 **i386_pentium4** SDK):
```
./bin/packages/i386_pentium4/local_feeds/clogtool_1_i386_pentium4.ipk
```

---

## 3) Copy the `.ipk`(s) into the QEMU VM

From your **host** (Ubuntu), assuming SSH is forwarded on **2222**:

```bash
# Dropbear on OpenWrt often needs legacy SCP protocol: add -O
# Copy clogtool
scp -O -P 2222 ./bin/packages/i386_pentium4/local_feeds/clogtool_*.ipk root@127.0.0.1:/tmp/
```

---

## 4) Install and run (inside the VM)

```sh

# SSH into the VM
ssh -p 2222 root@127.0.0.1

# Install dependencies (if your package does not pull them automatically)
opkg update || true

# Install clogtool package
opkg install /tmp/clogtool_*.ipk

# Quick usage
/usr/bin/clogtool --help

# Create a sample log
cat > /tmp/sample.log <<'EOF'
<date> kernel: IN=eth0 OUT= SRC=192.168.1.10 DST=8.8.8.8     LEN=120 PROTO=UDP SPT=56789 DPT=53
<date> kernel: IN=eth0 OUT= SRC=192.168.1.10 DST=1.1.1.1     BYTES=2048 PROTO=TCP SPT=55000 DPT=443
<date> kernel: IN=eth0 OUT= SRC=192.168.1.20 DST=93.184.216.34 LEN=150 PROTO=UDP SPT=40000 DPT=53
<date> kernel: IN=eth0 OUT= SRC=192.168.1.10 DST=93.184.216.34 LEN=220 PROTO=TCP SPT=55001 DPT=443
<date> kernel: IN=eth0 OUT= SRC=192.168.1.30 DST=142.250.74.78 BYTES=4096 PROTO=TCP SPT=50123 DPT=80
<date> kernel: IN=eth0 OUT= SRC=192.168.1.30 DST=142.250.74.78 LEN=600 PROTO=TCP SPT=50123 DPT=80
<date> kernel: IN=eth0 OUT= SRC=192.168.1.20 DST=151.101.1.69 BYTES=512 PROTO=TCP SPT=40001 DPT=80
<date> kernel: IN=eth0 OUT= SRC=192.168.1.10 DST=8.8.4.4     BYTES=1024 PROTO=UDP SPT=56790 DPT=53
<date> kernel: IN=eth0 OUT= SRC=192.168.1.40 DST=52.216.0.0  LEN=90  PROTO=TCP SPT=51000 DPT=443
<date> kernel: IN=eth0 OUT= SRC=192.168.1.20 DST=52.216.0.0  LEN=200 PROTO=TCP SPT=40002 DPT=443
<date> kernel: IN=eth0 OUT= SRC=fe80::1        DST=2606:4700:4700::1111 BYTES=300 PROTO=UDP SPT=5353 DPT=53
<date> kernel: IN=eth0 OUT= SRC=192.168.1.10 DST=93.184.216.34 BYTES=3500 PROTO=TCP SPT=55002 DPT=443
EOF
```

### run clogtool app
```sh
clogtool --input /tmp/sample.log --top 5 --metric bytes

```
Expected output:
```bash
src                        lines         bytes
192.168.1.10                   5          6912
192.168.1.30                   2          4696
192.168.1.20                   3           862
fe80::1                        1           300
192.168.1.40                   1            90
```

```sh
clogtool --input /tmp/sample.log --top 5 --metric lines

```
Expected output:
```bash
src                        lines         bytes
192.168.1.10                   5          6912
192.168.1.20                   3           862
192.168.1.30                   2          4696
192.168.1.40                   1            90
fe80::1                        1           300
```

---

## 5) Troubleshooting

- **No .ipk produced**: Run the packaging step :  `make package/feeds/local_feeds/clogtool/install V=sc -j"$(nproc)"`.
- **Arch mismatch on install**: On the VM `opkg print-architecture`. Ensure your `.ipk` arch matches (e.g., `i386_pentium4` vs `x86_64`).
- **Command not found after install**: Verify path: `opkg files clogtool`. This should list `/usr/bin/clogtool`
- **Uninstall**: remove it using `opkg remove clogtool`.


