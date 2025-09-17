#ifndef STATS_COLLECTOR_H
#define STATS_COLLECTOR_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define SC_INITIAL_CAP 16
#define SC_GROW_FACTOR 2

typedef struct
{
  uint64_t count;       // Counting number of times the IP address appears in log file
  uint64_t total_bytes; // total bytes
} IPStat;

typedef enum
{
  BY_LINES,
  BY_BYTES
} RankMetric;

static RankMetric g_rank_metric;

typedef struct
{
  char ip[64];
  IPStat s;
} Entry;

typedef struct
{
  Entry *items;
  size_t size; // number of items in StatsCollector
  size_t cap;  // maximum current capacity of StatsCollector
} StatsCollector;

static inline void stats_collector_init(StatsCollector *sc)
{
  sc->items = NULL;
  sc->size = 0;
  sc->cap = 0;
}

static inline void stats_collector_free(StatsCollector *sc)
{
  free(sc->items);
  sc->items = NULL;
  sc->size = 0;
  sc->cap = 0;
}

static inline void stats_collector_grow(StatsCollector *sc)
{
  // https://en.wikipedia.org/wiki/Dynamic_array
  // growing a dynamic array by a constant factor (e.g., doubling) yields amortized O(1) per append
  size_t ncap = (sc->cap == 0)                          ? SC_INITIAL_CAP
                : sc->cap > (SIZE_MAX / SC_GROW_FACTOR) ? SIZE_MAX
                                                        : sc->cap * SC_GROW_FACTOR;

  Entry *nitems = (Entry *)realloc(sc->items, ncap * sizeof(Entry));
  if (!nitems)
  {
    /* If realloc fails, keep old state. In production you might abort. */
    return;
  }
  sc->items = nitems;
  sc->cap = ncap;
}

static inline void stats_collector_add(StatsCollector *sc, const char *ip, uint64_t bytes)
{
  // Linear search for existing IP
  for (size_t i = 0; i < sc->size; ++i)
  {
    if (strcmp(sc->items[i].ip, ip) == 0)
    {
      sc->items[i].s.count += 1;
      sc->items[i].s.total_bytes += bytes;
      return;
    }
  }
  // Insert new entry
  if (sc->size == sc->cap)
    stats_collector_grow(sc);
  if (sc->size == sc->cap)
  {
    // realloc failed; drop sample ip
    return;
  }
  Entry *e = &sc->items[sc->size];
  sc->size++;
  // Safe copy into fixed buffer
  size_t n = strlen(ip);
  if (n >= sizeof(e->ip))
    n = sizeof(e->ip) - 1;
  memcpy(e->ip, ip, n);
  e->ip[n] = '\0';
  e->s.count = 1;
  e->s.total_bytes = bytes;
}

static int entry_cmp_desc(const void *a, const void *b)
{
  const Entry *entry_a = (const Entry *)a;
  const Entry *entry_b = (const Entry *)b;
  uint64_t value_a = (g_rank_metric == BY_BYTES) ? entry_a->s.total_bytes : entry_a->s.count;
  uint64_t value_b = (g_rank_metric == BY_BYTES) ? entry_b->s.total_bytes : entry_b->s.count;
  if (value_a > value_b)
    return -1;
  if (value_a < value_b)
    return 1;
  return strcmp(entry_a->ip, entry_b->ip);
}

static inline void sort_entries_by_metric(Entry *arr, size_t n, RankMetric metric)
{
  g_rank_metric = metric;
  qsort(arr, n, sizeof(Entry), entry_cmp_desc);
}

static inline Entry *stats_collector_sorted_copy(const StatsCollector *sc, RankMetric metric, size_t *out_count)
{
  *out_count = sc->size;
  if (sc->size == 0)
    return NULL;
  Entry *copy = (Entry *)malloc(sc->size * sizeof(Entry));
  if (!copy)
  {
    *out_count = 0;
    return NULL;
  }
  memcpy(copy, sc->items, sc->size * sizeof(Entry));
  sort_entries_by_metric(copy, sc->size, metric);
  return copy;
}

#endif /* STATS_COLLECTOR_H */
