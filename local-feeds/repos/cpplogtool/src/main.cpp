#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>

#include "parser.h"
#include "stats_collector.h"

static void print_help(const char *argv0)
{
  std::cout
      << "Usage: " << argv0 << " [--input <file>] [--top <N>] [--metric lines|bytes] [--help]\n"
      << "  --input <file>     read from file (default: stdin)\n"
      << "  --top <N>          top N talkers (default: 10)\n"
      << "  --metric <m>       lines|bytes (default: lines)\n";
}

int main(int argc, char **argv)
{
  std::string input;
  std::size_t topN = 10;
  RankMetric metric = BY_LINES;

  for (int i = 1; i < argc; ++i)
  {
    std::string a = argv[i];
    if (a == "--help")
    {
      print_help(argv[0]);
      return 0;
    }
    else if (a == "--input" && i + 1 < argc)
    {
      input = argv[++i];
    }
    else if (a == "--top" && i + 1 < argc)
    {
      int v = ::atoi(argv[++i]);
      if (v > 0)
        topN = (std::size_t)v;
    }
    else if (a == "--metric" && i + 1 < argc)
    {
      std::string m = argv[++i];
      if (m == "bytes")
        metric = BY_BYTES;
      else if (m == "lines")
        metric = BY_LINES;
      else
      {
        std::cerr << "Unknown metric: " << m << "\n";
        return 1;
      }
    }
    else
    {
      std::cerr << "Unknown option: " << a << "\n";
      print_help(argv[0]);
      return 1;
    }
  }

  std::istream *in = 0;
  std::ifstream file;
  if (!input.empty())
  {
    file.open(input.c_str());
    if (!file)
    {
      std::cerr << "Failed to open: " << input << "\n";
      return 2;
    }
    in = &file;
  }
  else
  {
    in = &std::cin;
  }

  Parser parser;
  StatsCollector stats; // was Aggregator

  std::string line;
  while (std::getline(*in, line))
  {
    ParsedLine pl;
    if (parser.parse(line, pl))
    {
      stats.add(pl.src, pl.bytes);
    }
  }

  std::vector<std::pair<std::string, IPStat>> res = stats.top(topN, metric);

  std::cout << std::left
            << std::setw(20) << "src" << "  "
            << std::setw(10) << "lines" << "  "
            << std::setw(12) << "bytes" << '\n';
  for (std::size_t i = 0; i < res.size(); ++i)
  {
    const std::string &src = res[i].first;
    const IPStat &ip_stat = res[i].second;
    std::cout << std::left
              << std::setw(20) << src << "  "
              << std::setw(10) << ip_stat.count << "  "
              << std::setw(12) << ip_stat.total_bytes << '\n';
  }
  return 0;
}
