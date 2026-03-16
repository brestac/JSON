#pragma once

#include <stddef.h>
#include <stdint.h>
#include "ParseResult.h"
// ---------------------------------------------------------------------------
//   JSONStream
// ---------------------------------------------------------------------------
#ifndef STRING_LENGTH
#define STRING_LENGTH
constexpr size_t str_length(const char *str) {
  size_t len = 0;
  while (str[len] != '\0') {
    len++;
    if (len >= 2048) break;
  }
  return len;
}
#endif

struct JSONStream {
  size_t size;
  char *buffer;
  size_t position;

  constexpr JSONStream() : size(0), buffer(nullptr), position(0) {}
  constexpr JSONStream(char *buffer, size_t size) : size(size), buffer(buffer), position(0) {}
  constexpr JSONStream(const char *buffer, size_t size) : size(size), buffer((char *)buffer), position(0) {}
  constexpr JSONStream(const char *buffer) : size(str_length(buffer)), buffer((char *)buffer), position(0) {
    JSON_DEBUG_INFO("JSONStream created with size=%zu\n", size);
  }
  constexpr JSONStream(std::string_view sv) : size(sv.length()), buffer((char *)sv.data()), position(0) {} 
  template <size_t N> constexpr JSONStream(char (&buffer)[N]) : size(N - 1), buffer(buffer), position(0) {}
  template <size_t N> constexpr JSONStream(const char (buffer)[N]) : size(N - 1), buffer(buffer), position(0) {}
  
  ~JSONStream() {
    if (buffer != nullptr && position > 0) {
      buffer[position] = '\0';
      //JSON_DEBUG_INFO("JSONStream destructor ended, buffer=%s\n", buffer);
    }
    size = 0;
    position = 0;
  }

  template <typename... Args>
  void constexpr write(const char *format, Args &&...args);
};

struct JSONData {
public:
  uint32_t updated = 0;

  virtual ~JSONData() = default;

  virtual JSON::ParseResult fromJSON(JSONStream stream) = 0;
JSON::ParseResult fromJSON(char *input, size_t size);

  virtual size_t toJSON(JSONStream stream, bool updates = true) = 0;
  size_t toJSON(bool updates = true);
  size_t toJSON(char *output, size_t size, bool updates = true);
  
  void clearUpdated();
};

////////////////////////////////////////////////////////////////////////////////
//  fromJSON
////////////////////////////////////////////////////////////////////////////////
JSON::ParseResult JSONData::fromJSON(char *input, size_t size) {
  JSONStream stream(input, size);
  return fromJSON(stream);
}

////////////////////////////////////////////////////////////////////////////////
//  toJSON
////////////////////////////////////////////////////////////////////////////////

size_t JSONData::toJSON(bool updates) {
  JSONStream stream;
  return toJSON(stream, updates);
}

size_t JSONData::toJSON(char *output, size_t size, bool updates) {
  JSONStream stream(output, size);
  return toJSON(stream, updates);
}

void JSONData::clearUpdated() { updated = 0; }
