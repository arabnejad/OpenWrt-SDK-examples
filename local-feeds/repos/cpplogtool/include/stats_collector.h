#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>

struct IPStat
{
  std::uint64_t count;       // Counting number of times the IP address appears in log file
  std::uint64_t total_bytes; // total bytes
  IPStat() : count(0), total_bytes(0) {}
};

enum RankMetric
{
  BY_LINES,
  BY_BYTES
};

class StatsCollector
{
public:
  void add(const std::string &src, std::uint64_t total_bytes)
  {
    IPStat &ip_stat = ip_stat_map[src];
    ip_stat.count++;
    ip_stat.total_bytes += total_bytes;
  }

  std::vector<std::pair<std::string, IPStat>> top(size_t N, RankMetric metric) const
  {
    std::vector<std::pair<std::string, IPStat>> v;
    v.reserve(ip_stat_map.size());

    for (const auto &kv : ip_stat_map)
    {
      v.emplace_back(kv.first, kv.second);
    }

    std::sort(v.begin(), v.end(),
              [metric](const std::pair<const std::string, IPStat> &a, const std::pair<const std::string, IPStat> &b)
              {
                std::uint64_t v1 = (metric == BY_BYTES) ? a.second.total_bytes : a.second.count;
                std::uint64_t v2 = (metric == BY_BYTES) ? b.second.total_bytes : b.second.count;
                if (v1 != v2)
                  return v1 > v2;
                return a.first < b.first; // sort by IP address
              });

    if (v.size() > N)
      v.resize(N);

    return v;
  }

private:
  std::unordered_map<std::string, IPStat> ip_stat_map;
};
