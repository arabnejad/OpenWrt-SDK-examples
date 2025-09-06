# UDP Echo (OpenWrt package)

A tiny UDP echo daemon for OpenWrt. It listens on a configurable address/port and echoes incoming UDP datagrams back to the sender. Useful for SDK/packaging exercises and quick UDP path checks.

---

## 1) Run OpenWrt in QEMU with UDP 9000 forwarded

Run this command. It forwards host **TCP 2222 → guest 22 (SSH)** and host **UDP 9000 → guest 9000**.

```bash
qemu-system-i386 \
    -m 256 -nographic \
    -drive file=openwrt.img,format=raw,if=ide \
    -nic user,model=e1000,hostfwd=tcp::2222-:22,hostfwd=udp::9000-:9000
```


> If your OpenWrt VM is 64-bit, use an **x86_64** image and matching **SDK**. If it’s 32-bit generic, use the **i386_pentium4** SDK.

---

## 2) Prepare & compile `udp_echo` in the OpenWrt SDK

From the **SDK root**:

```bash
# Ensure your local feed is wired up in feeds.conf, e.g.:
# src-link hamidrepo /absolute/path/to/local-feeds/hamidrepo
./scripts/feeds update -f hamidrepo
./scripts/feeds install -f -p hamidrepo udp_echo

# Clean + build only this package
make package/feeds/hamidrepo/udp_echo/clean V=s
make package/feeds/hamidrepo/udp_echo/compile V=s -j"$(nproc)"
```

Artifacts (example for 22.03 **i386_pentium4** SDK):
```
./bin/packages/i386_pentium4/hamidrepo/udp_echo_1.0-1_i386_pentium4.ipk
```

> Installing the wrong architecture will fail with “incompatible architectures”.

---

## 3) Copy the `.ipk` into the QEMU VM

From your **host** (Ubuntu), assuming SSH is forwarded on **2222**:

```bash
# Dropbear on OpenWrt often needs legacy SCP protocol: add -O
scp -O -P 2222 ./bin/packages/i386_pentium4/hamidrepo/udp_echo_*.ipk root@127.0.0.1:/tmp/
```

---

## 4) Install, enable, and start the service (inside the VM)

```sh
opkg install /tmp/udp_echo_*.ipk

# Enable and start the procd service
/etc/init.d/udp-echo enable
/etc/init.d/udp-echo start

# Sanity checks
ps | grep udp_echo
netstat -anu | grep 9000
```

If you plan to hit it from the host through QEMU “WAN”, allow UDP 9000 in the firewall:

```sh
uci add firewall rule
uci set firewall.@rule[-1].name='Allow-udp-echo'
uci set firewall.@rule[-1].src='wan'
uci set firewall.@rule[-1].proto='udp'
uci set firewall.@rule[-1].dest_port='9000'
uci set firewall.@rule[-1].target='ACCEPT'
uci commit firewall
/etc/init.d/firewall restart
```

---

## 5) Test from another machine (host)

Use **nc** (OpenBSD variant), **ncat**, or **socat**. For **nc**/**ncat**, keep stdin open briefly so the client waits to read the echo.

### 5.1 `nc` (OpenBSD netcat)
```bash
# Ensure the OpenBSD variant on Ubuntu
sudo apt-get update && sudo apt-get install netcat-openbsd

# Piped test – keep stdin open briefly (sleep 1) so nc reads the reply
( printf 'hello\n'; sleep 1 ) | nc -u -v -w3 -q1 127.0.0.1 9000
# Expected output:
# hello

# Interactive (type then see echo)
# nc -u -v -w3 127.0.0.1 9000
# hello<Enter>
```

### 5.2 `ncat` (the Nmap netcat)
```bash
sudo apt-get update && sudo apt-get install ncat
( printf 'hello\n'; sleep 1 ) | ncat -u --idle-timeout 3 127.0.0.1 9000
# Expected output:
# hello
```
> Some `ncat` builds also support:
> ```bash
> printf 'hello\n' | ncat -u --no-shutdown --idle-timeout 3 127.0.0.1 9000
> ```

### 5.3 `socat` (very reliable)
```bash
sudo apt-get update && sudo apt-get install socat
echo -n 'hello' | socat -t 3 - UDP:127.0.0.1:9000
# Expected output:
# hello
```

---

## 6) Optional: test inside the VM

BusyBox `nc` on OpenWrt is TCP-only/minimal. Use `socat`:

```sh
opkg update && opkg install socat
echo -n 'hello' | socat -t 3 - UDP:127.0.0.1:9000
```

---

## 7) Troubleshooting

- **No output on host but tcpdump shows packets both ways:** it’s just client timing. Use the `( printf ... ; sleep 1 ) | nc/ncat` form, or use `socat`.
- **No packets reach the VM:** confirm QEMU has `hostfwd=udp::9000-:9000`, and that the daemon listens on `0.0.0.0:9000`.
- **Firewall drops:** add the UCI rule above (WAN→router UDP 9000) and restart the firewall.
- **“bad udp cksum” in tcpdump:** normal in VMs due to checksum offload; not a functional error.
- **Arch mismatch on install:** ensure the `.ipk` arch matches the VM (e.g., `i386_pentium4` vs `x86_64`).

Handy live trace in the VM:
```sh
opkg update && opkg install tcpdump
tcpdump -ni any udp port 9000 -vv
```
