# ctmon-proc

A tiny OpenWrt userspace tool that tails `/proc/net/nf_conntrack` and prints
new connection-tracking entries as they appear. This variant intentionally
avoids libmnl/libnetfilter-conntrack to keep the build light and SDK‑friendly;
it only requires the kernel conntrack modules to be present on the target.


---

## Add the feed and index it

From the SDK root (`/openwrt-sdk` inside the container):

```sh
# Re-index just the feed
./scripts/feeds update -f local_feeds

# Verify the package is visible in the feed index
./scripts/feeds list local_feeds | grep -E '^ctmon-proc\b' || echo "ctmon-proc not indexed"
```

---

## Build

This package copies sources during **Build/Prepare**. Run `prepare` explicitly,
or remove the `.prepared_*` stamp before `compile` to ensure fresh sources are
copied.

### Recommended (reliable)

```sh
make package/feeds/local_feeds/ctmon-proc/{clean,prepare,compile} V=s -j1
```

### Alternative: remove the prepare stamp

```sh
rm -f build_dir/target-*/ctmon-proc-1.0/.prepared_*
make package/feeds/local_feeds/ctmon-proc/compile V=s -j1
```

If the build succeeds, there should be an ipk like:

```
bin/packages/i386_pentium4/local_feeds/ctmon-proc_1.0-1_i386_pentium4.ipk
```

> (Exact arch path depends on the SDK target.)

---

## Install on the OpenWrt VM (QEMU)

Assuming the QEMU forwards SSH on host port **2222** (→ guest 22):

```sh
# From the host
scp -P 2222 bin/packages/*/local_feeds/ctmon-proc_1.0-1_*.ipk root@127.0.0.1:/tmp/

# Inside the VM
opkg update
opkg install kmod-nf-conntrack  # ensure /proc/net/nf_conntrack exists
opkg install /tmp/ctmon-proc_1.0-1_*.ipk
```

Verify the proc file exists:

```sh
test -r /proc/net/nf_conntrack && echo "conntrack present" || echo "missing!"
```

---

## Run

Foreground (prints lines as they appear):

```sh
# watch poll every 2 seconds
/usr/bin/ctmon-proc -w 2
```

Background with logging:

```sh
nohup /usr/bin/ctmon-proc -w 2 >/tmp/ctmon.log 2>&1 &
tail -f /tmp/ctmon.log
```

---

## Generate traffic (see entries appear)

Run any of the following **inside the VM**:

```sh
ping -c2 1.1.1.1                     # ICMP entries
nslookup openwrt.org                 # UDP/53
wget -qO- http://example.com | head  # TCP/80
```

Or send traffic **from the host to the VM** using the UDP forward (e.g., 9000):

**socat (host):**
```sh
echo -n 'hello' | socat -t 3 - UDP:127.0.0.1:9000
```

**OpenBSD netcat (host):**
```sh
( printf 'hello\n'; sleep 1 ) | nc -u -w3 127.0.0.1 9000
```

**ncat (host):**
```sh
( printf 'hello\n'; sleep 1 ) | ncat -u --idle-timeout 3 127.0.0.1 9000
```

You should see new lines in `ctmon-proc` output as flows are created.

---

## Troubleshooting

- **`/proc/net/nf_conntrack` missing**
  Install & load: `opkg install kmod-nf-conntrack` then `modprobe nf_conntrack`.

- **No new lines appear**
  Generate traffic (e.g., `ping`, `nslookup`, or host→VM UDP). Check the file
  has content: `test -s /proc/net/nf_conntrack && tail -n2 /proc/net/nf_conntrack`.

- **`clean,compile` fails but `clean,prepare,compile` works**
  The SDK’s `.prepared_*` stamp makes `compile` skip the copy step. Either run
  `prepare` or delete the stamp (`rm -f build_dir/.../.prepared_*`).

---

