#pragma once

#include "macro.h"
#include "constants.h"

template <size_t N>
constexpr bool scan_keyword(const char *&position, const char (&keyword)[N],
                            bool include = true) {
  if (position == nullptr) return false;
  
  for (size_t i = 0; i < N; i++) {
    if (position[i] != keyword[i]) {
      return false;
    }      
  }

  if (include)
    position += N;

  return true;
}

// BE CAREFUL: even if include is true, by default position will be set to the delimiter position, not after it.
constexpr bool scan_until(const char *&position, char c, size_t max_length = 0,
                          bool include = true, bool include_delimiter = false) {
  if (position == nullptr) return false;
  
  const char *current = position;

  while (*current != c) {
    if (max_length > 0 && (current - position) > max_length) {
      if (include)
        position = current;
      return false;
    }
    current++;
  }

  bool result = current > position;

  if (include) {
    position = current;

    if (include_delimiter) {
      position++;
    }
  }
  
  return result;
}

constexpr bool scan_char(const char *&position, char c, bool include = true) {
  if (position == nullptr) return false;
  
  if (*position == c) {
    if (include)
      position++;

    return true;
  }

  return false;
}

template <size_t N>
constexpr bool scan_chars_once(const char *&position, const char (&chars)[N],
                               bool include = true) {
  if (position == nullptr) return false;
  
  for (size_t i = 0; i < N; i++) {
    if (*position == chars[i]) {
      if (include)
        position++;
      return true;
    }
  }

  return false;
}

template <size_t N>
constexpr bool scan_chars(const char *&position, const char (&chars)[N],
                          size_t max_length = 0, bool include = true) {
  if (position == nullptr) return false;
  
  const char *current = position;

  while (*position != '\0' &&
         (max_length == 0 || (current - position) <= max_length)) {
    bool ok = scan_chars_once(current, chars, true);
    if (!ok)
      break;
  }

  bool result = current > position;

  if (include)
    position = current;

  return result;
}

constexpr bool scan_range_once(const char *&position, char range[2], bool include = true) {
  if (position == nullptr) return false;
  
  const char *current = position;
  
  if (*current >= range[0] && *current <= range[1]) {
    if (include)
      position++;

     return true;
  }

  return false;
}

template <size_t N>
constexpr bool scan_ranges_once(const char *&position, char (&ranges)[N][2], bool include = true) {
  const char *current = position;

  for (size_t i = 0; i < N; i++) {
    bool ok = scan_range_once(current, ranges[i], true);
    if (ok) {
      break;
    }
  }
  
  bool result = current > position;

  if (include)
    position = current;

  return result;
}

constexpr bool scan_range(const char *&position, char (&ranges)[2], size_t max_length = 0, bool include = true) {
  if (position == nullptr) return false;
  
  const char *current = position;

  while (*position != '\0' && (max_length == 0 || (current - position) <= max_length)) {
    bool ok = scan_range_once(current, ranges, true);
    if (!ok) {
      break;
    }
  }

  bool result = current > position;

  if (include)
    position = current;

  return result;
}

template <size_t N>
constexpr bool scan_ranges(const char *&position, char (&ranges)[N][2], size_t max_length = 0, bool include = true) {
  if (position == nullptr) return false;
  
  const char *current = position;

  while (*position != '\0' && (max_length == 0 || (current - position) <= max_length)) {
    if (!scan_ranges_once(current, ranges, true)) {
      break;
    }
  }

  bool result = current > position;

  if (include)
    position = current;

  return result;
}

bool _skip_spaces(const char *&position, size_t max_length = JSON::MAX_JSON_LENGTH) {
  return scan_chars(position, JSON_SPACE_CHARACTERS, max_length, true);
}

// size_t check_is_comma_separated_integer_array(const char *&position, size_t max_length = 0) {
//   const char *current = (char *)position;
//   size_t n = 0;
  
//   if (scan_char(current, '[', true)) {
//     _skip_spaces(current, max_length);

//     size_t iterations = 0;
//     while (iterations < MAX_ITERATIONS ) {
//       iterations++;
      
//       if (!scan_range(current, JSON_DIGIT_CHARACTERS_RANGE, max_length, true)) {
//         _skip_spaces(current, max_length);
//         if (!scan_char(current, ',', true)) break;
//         _skip_spaces(current, max_length);
//       } else {
//         n++;
//       }
//     }

//     _skip_spaces(current, max_length);
    
//     if (scan_char(current, ']', true)) {
//       return n;
//     }
//   }

//   return 0;
// }
