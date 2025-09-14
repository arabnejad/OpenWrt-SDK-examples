# iperf3 — Local Feed Override with Patch (OpenWrt SDK)

This document shows how to **override** the stock `iperf3` package with a **local feed** copy,
apply a **quilt-style patch**, rebuild it in the OpenWrt **SDK**, and verify the patched binary in a **QEMU** VM.

> Target used in examples: **OpenWrt 22.03.0 x86/generic SDK** (but the flow is the same for other targets).

---

## 1) What you’ll build

- A local feed package `local_feeds/repos/iperf3/` that **shadows** the default `feeds/packages/net/iperf3`.
- A small patch that **prints a banner** when `IPERF3_PATCH_BANNER=1` is set (proof the patched binary runs).
- Rebuild **only** this package and produce a `.ipk`.
- Install and **verify** inside the QEMU OpenWrt VM.

---

## 2) Folder structure (local feed)

Your feed directory (absolute path is recommended in `feeds.conf` via `src-link`):

```
/work/local-feeds/repos/
└── iperf3/
    ├── Makefile
    ├── patches/
    │   └── 0001-Add-optional-patched-banner-via-env.patch
    └── README.md
```

> Copy `Makefile` **from the SDK** `feeds/packages/net/iperf3/Makefile` into your local feed above.
> Keep the original `PKG_SOURCE`, `PKG_VERSION`, `PKG_HASH`, etc., so the build “just works”.

---

## 3) The patch

Create `patches/0001-Add-optional-patched-banner-via-env.patch` with the following idea:
- Touch `src/main.c` (file containing `main()`).
- On startup, if env var `IPERF3_PATCH_BANNER` is set and not `"0"`, print a one-line banner to **stderr**.

> The exact hunk offsets may differ slightly across iperf3 versions; if needed, adjust the target file/line.
> For interactive creation with `quilt`, see section **7**.

**Patch concept (pseudo-unified-diff):**
```diff
--- a/src/main.c
+++ b/src/main.c
@@ -1,3 +1,5 @@
+#include <stdlib.h>
+#include <stdio.h>
 #include "iperf.h"
 #include "iperf_api.h"

@@ int main(int argc, char **argv)
+    const char *banner = getenv("IPERF3_PATCH_BANNER");
+    if (banner && *banner && *banner != '0') {
+        fprintf(stderr, "iperf3: [patched] banner enabled via IPERF3_PATCH_BANNER\n");
+    }
```

Place the finished patch under `local-feeds/local_feeds/repos/iperf3/patches/`.

---

## 4) Wire the local feed

Ensure `feeds.conf` uses an **absolute** path for your local feed (prevents empty indices/symlink issues):

```
src-link local_feeds /work/local-feeds/repos/
```

Update and force-install the local override:
```bash
cd /openwrt-sdk
./scripts/feeds update -f local_feeds
./scripts/feeds install -f -p local_feeds iperf3
```

---

## 5) Build only iperf3

```bash
# Inside the SDK container
cd /openwrt-sdk

# Clean + compile only this package
make package/feeds/local_feeds/iperf3/{clean,prepare,compile} V=s -j"$(nproc)"
```

Artifacts appear under:
```
./bin/packages/<arch>/local_feeds/iperf3_*.ipk
```
(For x86/generic 32-bit SDK, `<arch>` is typically `i386_pentium4`. For x86/64 SDK it’s `x86_64`.)

---

## 6) Install in the QEMU VM & verify

Copy the ipk (Dropbear prefers legacy scp protocol, hence `-O`):
```bash
# From host
scp -O -P 2222 ./bin/packages/*/local_feeds/iperf3_*.ipk root@127.0.0.1:/tmp/
```

Install and run (inside the VM):
```bash
opkg update || true
opkg install /tmp/iperf3_*.ipk

# Verify banner is printed on stderr (does not break JSON/stdout flows)
IPERF3_PATCH_BANNER=1 iperf3 --version 2>&1 | head -n1
```

Expected output includes:
```
iperf3: [patched] banner enabled via IPERF3_PATCH_BANNER
```

You can also run a quick server/client test:
```bash
# In VM shell:
IPERF3_PATCH_BANNER=1 iperf3 -s -D             # start server in daemon mode
iperf3 -c 127.0.0.1 -t 2                       # simple local client test
```

---

## 7) (Alternative) Create the patch interactively with quilt

Sometimes it’s easier to create/refresh patches from the **build tree** after `prepare`:

```bash
# 1) Prepare iperf3 so sources are unpacked under build_dir
make package/feeds/local_feeds/iperf3/prepare V=s

# 2) Enter the build dir (adjust the actual versioned path)
cd build_dir/target-*/iperf-*/

# 3) Tell quilt to store patches back into your local feed
export QUILT_PATCHES="/work/local-feeds/repos/iperf3/patches"

# 4) Start a new patch and add the file you’ll edit
quilt new 0001-Add-optional-patched-banner-via-env.patch
quilt add src/main.c

# 5) Edit src/main.c and insert the banner snippet, then refresh
quilt refresh

# 6) Rebuild the package
cd /openwrt-sdk
make package/feeds/local_feeds/iperf3/{clean,prepare,compile} V=s -j"$(nproc)"
```

Your refreshed patch will live in `local-feeds/local_feeds/iperf3/patches/` and be applied on each build.

---

