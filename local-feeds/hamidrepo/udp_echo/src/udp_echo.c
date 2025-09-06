/*
 * udp_echo.c — a minimal UDP echo daemon using epoll(7)
 *
 * Build: via OpenWrt package Makefile (TARGET_CC + flags)
 * Run:   /usr/bin/udp_echo [-f] [-a addr] [-p port] [-b bytes] [-t]
 *
 * Flags:
 *   -f          run in foreground (default; procd keeps it in foreground)
 *   -a addr     listen address (default 0.0.0.0)
 *   -p port     listen port    (default 9000)
 *   -b bytes    receive buffer size (default 2048)
 *   -t          prefix replies with a UTC ISO-8601 timestamp + space
 *
 * Notes:
 * - Uses non-blocking socket + epoll for robustness.
 * - Designed to have zero dynamic library deps beyond libc (no C++).
 * - Intended for OpenWrt procd integration; logging kept simple.
 */

#include <arpa/inet.h>  // inet_aton(), htons()
#include <errno.h>      // errno
#include <fcntl.h>      // fcntl()
#include <netinet/in.h> // sockaddr_in
#include <signal.h>     // signal handling
#include <stdbool.h>    // bool
#include <stdio.h>      // printf, fprintf, perror
#include <stdlib.h>     // atoi, malloc, free
#include <string.h>     // memset, memcpy, strlen
#include <sys/epoll.h>  // epoll API
#include <sys/socket.h> // socket, bind, sendto, recvfrom
#include <syslog.h>     // syslog if daemonized
#include <time.h>       // time, gmtime_r, strftime
#include <unistd.h>     // getopt, close, daemon, write

// Global stop flag toggled by signals (SIGINT/SIGTERM)
static volatile sig_atomic_t g_stop = 0;

// Whether to add timestamp prefixes to replies (set by -t flag)
static bool g_timestamp = false;

// Signal handler: set stop flag so the main loop can exit gracefully
static void on_signal(int sig)
{
    (void)sig;
    g_stop = 1;
}

// Print CLI usage
static void usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [-f] [-a addr] [-p port] [-b bufsize] [-t]\n"
            "  -f           run in foreground (default)\n"
            "  -a <addr>    listen address (default 0.0.0.0)\n"
            "  -p <port>    listen port (default 9000)\n"
            "  -b <n>       receive buffer size (default 2048)\n"
            "  -t           prefix replies with UTC timestamp\n",
            prog);
}

// Helper: set O_NONBLOCK on a file descriptor
static int set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return -1;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        return -1;
    return 0;
}

int main(int argc, char **argv)
{
    // Defaults overridable by CLI flags or UCI via init script
    const char *addr_str = "0.0.0.0";
    int port = 9000;
    size_t bufsize = 2048;
    bool foreground = true; // procd prefers foreground

    // Parse CLI flags
    int opt;
    while ((opt = getopt(argc, argv, "fa:p:b:th")) != -1)
    {
        switch (opt)
        {
        case 'f':
            foreground = true;
            break;
        case 'a':
            addr_str = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'b':
            bufsize = (size_t)atoi(optarg);
            break;
        case 't':
            g_timestamp = true;
            break;
        case 'h':
        default:
            usage(argv[0]);
            return (opt == 'h') ? 0 : 1;
        }
    }

    // Setup signal handlers so CTRL+C or procd stop will exit cleanly
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // If we ever wanted to detach (not needed with procd), support daemon()
    if (!foreground)
    {
        if (daemon(0, 0) < 0)
        {
            perror("daemon");
            return 1;
        }
        openlog("udp_echo", LOG_PID | LOG_NDELAY, LOG_DAEMON);
    }

    // Create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    // Allow quick rebinding after restart
    int yes = 1;
    (void)setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // Build bind address (IPv4 only for simplicity)
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_aton(addr_str, &addr.sin_addr) == 0)
    {
        fprintf(stderr, "Invalid address: %s\n", addr_str);
        return 1;
    }

    // Bind UDP socket to chosen addr:port
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        return 1;
    }

    // Non-blocking socket to use with epoll
    if (set_nonblock(sock) < 0)
    {
        perror("nonblock");
        return 1;
    }

    // epoll(7) setup: watch the socket for readability
    int ep = epoll_create1(EPOLL_CLOEXEC);
    if (ep < 0)
    {
        perror("epoll_create1");
        return 1;
    }
    struct epoll_event ev = {.events = EPOLLIN, .data.fd = sock};
    (void)epoll_ctl(ep, EPOLL_CTL_ADD, sock, &ev);

    // Allocate receive buffer
    char *buf = malloc(bufsize);
    if (!buf)
    {
        fprintf(stderr, "oom\n");
        return 1;
    }

    // Initial status message (stderr or syslog depending on foreground)
    if (foreground)
        fprintf(stderr, "udp_echo listening on %s:%d (buf=%zu, ts=%d)\n",
                addr_str, port, bufsize, g_timestamp ? 1 : 0);
    else
        syslog(LOG_INFO, "udp_echo listening on %s:%d (buf=%zu, ts=%d)",
               addr_str, port, bufsize, g_timestamp ? 1 : 0);

    // Main loop: wait for events and echo back datagrams
    while (!g_stop)
    {
        struct epoll_event events[4];
        int n = epoll_wait(ep, events, 4, 1000 /* ms timeout */);
        if (n < 0)
        {
            if (errno == EINTR)
                continue; // interrupted by signal, retry
            perror("epoll_wait");
            break;
        }
        if (n == 0)
            continue; // timeout (1s), loop again

        for (int i = 0; i < n; ++i)
        {
            if (!(events[i].events & EPOLLIN))
                continue;

            // Receive a UDP datagram
            struct sockaddr_in src;
            socklen_t slen = sizeof(src);
            ssize_t r = recvfrom(sock, buf, bufsize, 0,
                                 (struct sockaddr *)&src, &slen);
            if (r < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    continue;
                perror("recvfrom");
                continue;
            }

            // If timestamps requested (-t), prefix ISO-8601 UTC + space
            if (g_timestamp)
            {
                char ts[32];
                time_t now = time(NULL);
                struct tm tm;
                gmtime_r(&now, &tm);
                strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ ", &tm);

                // Compose ts + payload into a stack buffer (alloca) or small heap temp
                size_t tsl = strlen(ts);
                // Stack-friendly: small messages; for large, fallback to heap if you prefer
                char *out = alloca(tsl + (size_t)r);
                memcpy(out, ts, tsl);
                memcpy(out + tsl, buf, (size_t)r);
                (void)sendto(sock, out, tsl + (size_t)r, 0,
                             (struct sockaddr *)&src, slen);
            }
            else
            {
                // Plain echo
                (void)sendto(sock, buf, (size_t)r, 0,
                             (struct sockaddr *)&src, slen);
            }
        }
    }

    if (!foreground)
        closelog();
    free(buf);
    close(ep);
    close(sock);
    return 0;
}
