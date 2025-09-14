# json_parser (OpenWrt package)

A tiny **C++11** CLI that reads a JSON file, parses it using **nlohmann/json**, and pretty-prints the result. Built as an OpenWrt package for SDK practice.

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

## 2) Prepare & compile `json_parser` in the OpenWrt SDK

From the **SDK root**:

```bash
# Ensure your local feed is wired up in feeds.conf, e.g.:
# src-link local_feeds /work/local-feeds/repos/

# 1) Update feeds (your local feed + packages feed for nlohmannjson)
./scripts/feeds update -f local_feeds packages

# 2) Install build dependencies and your package metadata
./scripts/feeds install -f -p packages nlohmannjson
./scripts/feeds install -f -p local_feeds json_parser

# 3) Clean + build only this package
make package/feeds/local_feeds/json_parser/{clean,prepare,compile} V=s  -j"$(nproc)"
```

Artifacts (example for 22.03 **i386_pentium4** SDK):
```
./bin/packages/i386_pentium4/local_feeds/json_parser_1.0-1_i386_pentium4.ipk
# (optional) If your SDK emits an nlohmannjson .ipk, it may be under:
./bin/packages/i386_pentium4/packages/nlohmannjson_*.ipk
```

> ℹ️ `nlohmannjson` is header-only. It’s needed at **build-time**. At **runtime** only `libstdc++` is required.
> If your `json_parser` package lists `+nlohmannjson` in `DEPENDS`, you’ll need to install that .ipk too (or drop that dependency for runtime).

---

## 3) Copy the `.ipk`(s) into the QEMU VM

From your **host** (Ubuntu), assuming SSH is forwarded on **2222**:

```bash
# Dropbear on OpenWrt often needs legacy SCP protocol: add -O
# Copy json_parser (and nlohmannjson if it exists)
scp -O -P 2222 ./bin/packages/i386_pentium4/local_feeds/json_parser_*.ipk root@127.0.0.1:/tmp/
# If you have an nlohmannjson ipk:
# scp -O -P 2222 ./bin/packages/i386_pentium4/packages/nlohmannjson_*.ipk root@127.0.0.1:/tmp/
```

---

## 4) Install and run (inside the VM)

```sh
# Install dependencies (if your package does not pull them automatically)
opkg update || true
opkg install libstdcpp || true

# Install your package (and nlohmannjson if it is listed as a runtime dep)
opkg install /tmp/json_parser_*.ipk
# opkg install /tmp/nlohmannjson_*.ipk   # only if required by your package

# Quick usage
/usr/bin/json_parser -h 2>/dev/null || true

# Create a sample JSON and parse it
cat > /tmp/sample.json << 'EOF'
{
  "name": "Hamid",
  "numbers": [1, 2, 3],
  "nested": { "ok": true }
}
EOF

/usr/bin/json_parser /tmp/sample.json
```

Expected output (pretty-printed):
```json
{
    "name": "Hamid",
    "nested": {
        "ok": true
    },
    "numbers": [
        1,
        2,
        3
    ]
}
```

---

## 5) Troubleshooting

- **Arch mismatch on install**: ensure the `.ipk` arch matches the VM (e.g., `i386_pentium4` vs `x86_64`).
- **`libstdc++.so.6` missing at runtime**: `opkg update && opkg install libstdcpp`.
- **Changing sources doesn’t rebuild**: run `make package/feeds/local_feeds/json_parser/{clean,compile} V=s`.
- **Patches**: put `0001-...patch` under `patches/` and rebuild, or use quilt in `build_dir/.../json_parser-1.0` (`QUILT_PATCHES=$(CURDIR)/patches quilt push -a`).

---

## 7) Handy commands

```bash
# From SDK root – rebuild just this package
make package/feeds/local_feeds/json_parser/{clean,compile} V=s -j"$(nproc)"

# Inspect the produced ipk
tar -tf ./bin/packages/*/local_feeds/json_parser_*.ipk
```
