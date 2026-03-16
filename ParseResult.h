#pragma once

#include <stdint.h>
#include <stddef.h>
#include "macro.h"

namespace JSON {
  struct ParseResult {
    size_t length;
    size_t nKeys;
    size_t nParsed;
    size_t nConverted;
    size_t nUpdated;
    bool error;
    uint64_t elapsed;
    bool stopped;

    ParseResult(size_t length, size_t nKeys, size_t nParsed, size_t nConverted, size_t nUpdated, bool error, uint64_t elapsed, bool stopped = false) : length(length), nKeys(nKeys), nParsed(nParsed), nConverted(nConverted), nUpdated(nUpdated), error(error), elapsed(elapsed), stopped(stopped) {}
    operator size_t() const { return length; }

    void print();
  };

  void ParseResult::print() {
    PRINT_FUNC("{\"length\":%zu,\"nKeys\":%zu,\"nParsed\":%zu,\"nMatched\":%zu,\"nUpdated\":%zu,\"error\":%s,\"elapsed\":%lu, \"stopped\":%d}\n", length, nKeys, nParsed, nConverted, nUpdated, error ? "true" : "false", elapsed, stopped);
  }
}
