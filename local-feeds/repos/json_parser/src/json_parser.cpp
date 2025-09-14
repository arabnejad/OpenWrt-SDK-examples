#include "json_parser.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

nlohmann::json JSON_PARSER::parse_file(const std::string &path) const
{
  std::ifstream in(path, std::ios::in | std::ios::binary);
  if (!in)
  {
    throw std::runtime_error("cannot open: " + path);
  }
  std::ostringstream buf;
  buf << in.rdbuf();
  return nlohmann::json::parse(buf.str());
}
