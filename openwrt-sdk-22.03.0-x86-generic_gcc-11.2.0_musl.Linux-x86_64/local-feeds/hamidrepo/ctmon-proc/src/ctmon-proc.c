/*
 * ctmon-proc: minimal conntrack monitor (procfs)
 *
 * Reads /proc/net/nf_conntrack and prints lines.
 * Options:
 *   -w <secs>   : watch mode; re-read every <secs>
 *   -q <substr> : only print lines containing <substr> (can repeat)
 *   -n <count>  : limit lines per snapshot (helpful to avoid floods)
 *
 * Examples:
 *   ctmon-proc
 *   ctmon-proc -w 2
 *   ctmon-proc -w 1 -q tcp -q dport=22
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define PROC_CT "/proc/net/nf_conntrack"
#define MAX_LINE 4096
#define MAX_FILTERS 16

struct filters
{
  int count;
  const char *q[MAX_FILTERS];
};

static int match_filters(const char *line, const struct filters *f)
{
  if (f->count == 0)
    return 1; /* no filter -> print all */
  for (int i = 0; i < f->count; i++)
  {
    if (strstr(line, f->q[i]) == NULL)
    {
      return 0; /* all filters must match (AND) */
    }
  }
  return 1;
}

static int dump_once(const struct filters *f, long limit)
{
  FILE *fp = fopen(PROC_CT, "r");
  if (!fp)
  {
    fprintf(stderr, "ctmon-proc: cannot open %s: %s\n", PROC_CT, strerror(errno));
    return -1;
  }

  char buf[MAX_LINE];
  long printed = 0;
  while (fgets(buf, sizeof(buf), fp) != NULL)
  {
    if (match_filters(buf, f))
    {
      fputs(buf, stdout);
      printed++;
      if (limit > 0 && printed >= limit)
        break;
    }
  }
  fclose(fp);
  fflush(stdout);
  return 0;
}

static void usage(const char *argv0)
{
  fprintf(stderr,
          "Usage: %s [-w secs] [-q substr]... [-n count]\n"
          "  -w secs    Re-read /proc/net/nf_conntrack every N seconds\n"
          "  -q substr  Require substring match (can be repeated; AND logic)\n"
          "  -n count   Limit printed lines per snapshot\n"
          "\nExamples:\n"
          "  %s\n"
          "  %s -w 2\n"
          "  %s -w 1 -q tcp -q dport=22\n",
          argv0, argv0, argv0, argv0);
}

int main(int argc, char **argv)
{
  int opt;
  int watch_secs = 0;
  long limit = 0;
  struct filters F = {.count = 0};

  while ((opt = getopt(argc, argv, "w:q:n:h")) != -1)
  {
    switch (opt)
    {
    case 'w':
      watch_secs = atoi(optarg);
      if (watch_secs < 0)
        watch_secs = 0;
      break;
    case 'q':
      if (F.count < MAX_FILTERS)
      {
        F.q[F.count++] = optarg;
      }
      else
      {
        fprintf(stderr, "Too many -q filters (max %d)\n", MAX_FILTERS);
        return 1;
      }
      break;
    case 'n':
      limit = strtol(optarg, NULL, 10);
      if (limit < 0)
        limit = 0;
      break;
    case 'h':
    default:
      usage(argv[0]);
      return (opt == 'h') ? 0 : 2;
    }
  }

  if (watch_secs <= 0)
  {
    /* One-shot */
    return dump_once(&F, limit) == 0 ? 0 : 1;
  }

  /* Watch mode */
  for (;;)
  {
    if (dump_once(&F, limit) != 0)
    {
      /* If procfs entry is not present, don't spin hard */
      sleep(watch_secs > 5 ? watch_secs : 5);
    }
    else
    {
      sleep(watch_secs);
    }
  }
  return 0;
}
