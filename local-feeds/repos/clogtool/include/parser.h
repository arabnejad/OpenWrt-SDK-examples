#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

typedef struct
{
    char ip[64];    // SRC=...
    uint64_t bytes; // BYTES=... or LEN=..., default 0
    bool ok;        // true if SRC key found
} ParsedLine;

static inline bool extract_token_into(const char *line, const char *key, char *token_str, size_t token_str_size)
{
    if (token_str_size == 0)
        return false;

    const char *pos = strstr(line, key);
    if (!pos)
        return false;
    pos += strlen(key);

    const char *end = pos;
    while (*end && *end != ' ' && *end != '\t' && *end != '\r' && *end != '\n')
        ++end;

    size_t n = (size_t)(end - pos);
    if (n >= token_str_size)
        n = token_str_size - 1;
    memcpy(token_str, pos, n);

    token_str[n] = '\0';
    return true;
}

static inline bool parse_u64_token(const char *line, const char *key, uint64_t *value)
{
    const char *pos = strstr(line, key);
    if (!pos)
        return false;
    pos += strlen(key);

    const char *end = pos;
    while (*end && *end != ' ' && *end != '\t' && *end != '\r' && *end != '\n')
        ++end;

    size_t n = (size_t)(end - pos);
    if (n == 0)
        return false;

    // set buffer to copy the number
    char buf[32];
    if (n >= sizeof(buf))
        n = sizeof(buf) - 1;
    memcpy(buf, pos, n);
    buf[n] = '\0';

    // convert using strtoull base-10
    char *endp = NULL;
    uint64_t v = 0;
    errno = 0; // clear errno
    v = (uint64_t)strtoull(buf, &endp, 10);
    if (endp == buf)
    {
        // can’t read even a single digit (no conversion)
        // returns 0, and sets endp == nptr
        return false;
    }
    if (errno == ERANGE)
    {
        // overflow detected during conversion.
        // value was limited to ULLONG_MAX.
        return false;
    }
    *value = v;
    return true;
}

static inline bool parse_line(const char *line, ParsedLine *parsed_line)
{
    // SRC= should be exists in line
    if (!extract_token_into(line, "SRC=", parsed_line->ip, sizeof(parsed_line->ip)))
    {
        parsed_line->ok = false;
        return false;
    }
    parsed_line->ok = true;

    // Prefer BYTES=, if not exists, look for LEN=
    uint64_t v = 0;
    if (parse_u64_token(line, "BYTES=", &v))
    {
        parsed_line->bytes = v;
    }
    else if (parse_u64_token(line, "LEN=", &v))
    {
        parsed_line->bytes = v;
    }
    else
    {
        parsed_line->bytes = 0;
    }
    return true;
}

#endif /* PARSER_H */
