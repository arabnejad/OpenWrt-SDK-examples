#pragma once
#include <string>
#include <nlohmann/json.hpp>

class JSON_PARSER
{
public:
  // Parse the JSON file at 'path' and return the parsed object.
  nlohmann::json parse_file(const std::string &path) const;
};
