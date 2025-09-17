#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "parser.h"
#include "stats_collector.h"

static void print_help(const char *argv0)
{
  printf(
      "Usage: %s [--input <file>] [--top <N>] [--metric lines|bytes] [--help]\n"
      "  --input <file>     read from file (default: stdin)\n"
      "  --top <N>          top N talkers (default: 10)\n"
      "  --metric <m>       lines|bytes (default: lines)\n",
      argv0);
}

int main(int argc, char **argv)
{
  const char *input = NULL;
  size_t topN = 10;
  RankMetric metric = BY_LINES;

  for (int i = 1; i < argc; ++i)
  {
    const char *a = argv[i];
    if (strcmp(a, "--help") == 0)
    {
      print_help(argv[0]);
      return 0;
    }
    else if (strcmp(a, "--input") == 0 && i + 1 < argc)
    {
      input = argv[++i];
    }
    else if (strcmp(a, "--top") == 0 && i + 1 < argc)
    {
      long v = strtol(argv[++i], NULL, 10);
      if (v > 0)
        topN = (size_t)v;
    }
    else if (strcmp(a, "--metric") == 0 && i + 1 < argc)
    {
      const char *m = argv[++i];
      if (strcmp(m, "bytes") == 0)
        metric = BY_BYTES;
      else if (strcmp(m, "lines") == 0)
        metric = BY_LINES;
      else
      {
        fprintf(stderr, "Unknown metric: %s\n", m);
        return 1;
      }
    }
    else
    {
      fprintf(stderr, "Unknown option: %s\n", a);
      print_help(argv[0]);
      return 1;
    }
  }

  FILE *in = stdin;
  if (input)
  {
    in = fopen(input, "r");
    if (!in)
    {
      perror("fopen");
      return 2;
    }
  }

  StatsCollector stats;
  stats_collector_init(&stats);

  char buf[4096];
  while (fgets(buf, sizeof buf, in) != NULL)
  {
    ParsedLine pl = (ParsedLine){.ip = "", .bytes = 0, .ok = false};
    if (parse_line(buf, &pl))
    {
      stats_collector_add(&stats, pl.ip, pl.bytes);
    }
  }

  if (in && in != stdin)
    fclose(in);

  size_t count = 0;
  Entry *res = stats_collector_sorted_copy(&stats, metric, &count);

  printf("%-20s  %10s  %12s\n", "src", "lines", "bytes");
  size_t limit = (topN > count) ? topN : count;
  for (size_t i = 0; i < limit; ++i)
  {
    const char *ip = res[i].ip;
    const IPStat *s = &res[i].s;
    printf("%-20s  %10llu  %12llu\n",
           ip,
           (unsigned long long)s->count,
           (unsigned long long)s->total_bytes);
  }

  free(res);
  stats_collector_free(&stats);
  return 0;
}
