#pragma once

#include <stddef.h>
#define _MAX_ARRAY_LENGTH 1 << 16

namespace JSON {
    inline uint8_t PARSER_ID = 0;
    inline uint8_t VERSION = 1;
    inline bool PRINT_BUFFER_AS_HEX = true;
    inline size_t DEBUG_COLUMN_WIDTH = 80;

    inline size_t MAX_JSON_LENGTH = 1 << 24; // 16777216 bytes = 16MB
    inline size_t MAX_KEY_LENGTH = 255;
    inline size_t MAX_VALUE_LENGTH = 255;
    inline size_t MAX_ARRAY_LENGTH = 1 << 16; // 65536
    inline size_t MAX_KEY_VALUE_COUNT = 255;
    inline size_t MAX_ITERATIONS = 1 << 16; // 65536
}

static char JSON_SPACE_CHARACTERS[4] = {' ', '\t', '\n', '\r'};
static char JSON_HEX_CHARACTERS[3][2] = {{'a', 'f'}, {'A', 'F'}, {'0', '9'}};
static char JSON_KEY_CHARACTERS[5][2] = {
    {'a', 'z'}, {'A', 'Z'}, {'0', '9'}, {'_', '_'}, {'$', '$'}};
static char JSON_DIGIT_CHARACTERS_RANGE[2] = {'0', '9'};

static constexpr char JSON_START_CHARACTER = '{';
static constexpr char JSON_END_CHARACTER = '}';
static constexpr char JSON_ARRAY_START_CHARACTER = '[';
static constexpr char JSON_ARRAY_END_CHARACTER = ']';
static constexpr char JSON_COLON_CHARACTER = ':';
static constexpr char JSON_COMMA_CHARACTER = ',';
static constexpr char JSON_QUOTE_CHARACTER = '"';
static constexpr char JSON_ESCAPE_CHARACTER = '\\';
static constexpr char JSON_TRUE[4] = {'t', 'r', 'u', 'e'};
static constexpr char JSON_FALSE[5] = {'f', 'a', 'l', 's', 'e'};
static constexpr char JSON_NULL[4] = {'n', 'u', 'l', 'l'};
static constexpr char JSON_NAN[3] = {'N', 'a', 'N'};
static constexpr char JSON_INFINITY[8] = {'I', 'n', 'f', 'i', 'n', 'i', 't', 'y'};
