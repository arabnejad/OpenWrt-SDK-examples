#pragma once
#include <string>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cerrno>

struct ParsedLine
{
    std::string src;     // SRC=...
    std::uint64_t bytes; // BYTES=... or LEN=..., default 0
    bool ok;             // true if SRC key found
    ParsedLine() : bytes(0), ok(false) {}
};

class Parser
{
public:
    // Parses a single log line. Accepts tokens like KEY=VALUE separated by spaces.
    // Valid if at least SRC= is present.
    // Bytes read from BYTES=... (preferred) then LEN=...
    bool parse(const std::string &line, ParsedLine &parsed_line) const
    {
        parsed_line = ParsedLine();
        // Fast substring extraction: find "SRC=", "BYTES=", "LEN="
        extractToken(line, "SRC=", parsed_line.src);
        std::string token_value_str;
        if (!extractToken(line, "BYTES=", token_value_str))
        {
            extractToken(line, "LEN=", token_value_str);
        }
        if (!token_value_str.empty())
        {
            std::uint64_t value = 0;
            if (parseUint(token_value_str, value))
                parsed_line.bytes = value;
        }
        parsed_line.ok = !parsed_line.src.empty();
        return parsed_line.ok;
    }

private:
    static bool parseUint(const std::string &s, std::uint64_t &value)
    {
        if (s.empty())
            return false;
        value = 0;

        char *end = nullptr;
        errno = 0;
        unsigned long long tmp = std::strtoull(s.c_str(), &end, 10);
        if (end == s.c_str() || *end != '\0' || errno == ERANGE)
            return false;
        value = static_cast<std::uint64_t>(tmp);
        // for (std::size_t i = 0; i < s.size(); ++i)
        // {
        //     char c = s[i];
        //     if (c < '0' || c > '9')
        //         return false;
        //     value = value * 10 + (std::uint64_t)(c - '0');
        // }
        return true;
    }

    static bool extractToken(const std::string &line, const char *key, std::string &token_value)
    {
        const std::size_t key_len = std::strlen(key);
        const std::size_t key_pos = line.find(key);
        if (key_pos == std::string::npos)
            return false;
        const std::size_t start_pos = key_pos + key_len;
        const std::size_t end_pos = line.find_first_of(" \t\r\n", start_pos);
        token_value = (end_pos == std::string::npos) ? line.substr(start_pos)
                                                     : line.substr(start_pos, end_pos - start_pos);
        return true;
    }
};
