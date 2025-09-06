#include <iostream>
#include "json_parser.h"

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::cerr << "Usage: json_parser <file.json>\n";
    return 1;
  }
  try
  {
    JSON_PARSER p;
    auto j = p.parse_file(argv[1]);
    std::cout << j.dump(2) << std::endl;
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error: " << e.what() << "\n";
    return 2;
  }
}
